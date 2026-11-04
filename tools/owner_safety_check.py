#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
# Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
"""No access to a freed object in the programs Inox builds (CANON-25).

The release libinoxrt keeps freed blocks on free lists and hands them out
again (Links compare generations), so a program that reaches a freed object
reads or writes a block another object may own, and AddressSanitizer sees
nothing. This tool builds libinoxrt in its checking mode
(`INOX_OWN_QUARANTINE`: a freed block is never reused, and its payload is
poisoned), instruments the generated code with AddressSanitizer (each
function marked `sanitize_address`, which the sanitizer requires), runs every
runtime test that uses owners, Links or `New`, and fails on any sanitizer
report, on a changed output, or on a trap that does not happen.

Usage:
  python3 tools/owner_safety_check.py <inox>

Linux only. Needs clang++ with its AddressSanitizer runtime, or clang and
g++ (the generated code instrumented by clang, the runtime and the link by
g++). Leaks are not checked: a quarantined block is never freed.
"""

import glob
import os
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RUNTIME = sorted(glob.glob(os.path.join(ROOT, "src", "runtime", "*.cpp")))
OWNERS = re.compile(r"\b(Own|Link)<|\bNew\(")
MARKER = "AddressSanitizer"


def instrumented(ir):
    """The IR with every defined function marked `sanitize_address`: the
    sanitizer's pass instruments only the functions that carry it, and the
    compiler emits none."""
    lines = []
    for line in ir.split("\n"):
        if line.startswith("define ") and " sanitize_address" not in line:
            start = line.find("(", line.find("@"))
            depth = 0
            for index in range(start, len(line)):
                if line[index] == "(":
                    depth += 1
                elif line[index] == ")":
                    depth -= 1
                    if depth == 0:
                        line = line[:index + 1] + " sanitize_address" + line[index + 1:]
                        break
        lines.append(line)
    return "\n".join(lines)


def run(command, **kwargs):
    return subprocess.run(command, capture_output=True, text=True, **kwargs)


def toolchains():
    """(name, runtime compiler, program compile, link), best first."""
    found = []
    if shutil.which("clang++"):
        found.append(("clang", "clang++", None, "clang++"))
    if shutil.which("clang") and shutil.which("g++"):
        found.append(("clang-instrument-gcc-link", "g++", "clang", "g++"))
    return found


def build_runtime(compiler, work):
    objects = []
    for source in RUNTIME:
        obj = os.path.join(work, os.path.basename(source) + ".o")
        result = run([compiler, "-std=c++20", "-O1", "-g", "-fsanitize=address", "-DINOX_OWN_QUARANTINE",
                      "-c", source, "-o", obj])
        if result.returncode != 0:
            return None, result.stdout + result.stderr
        objects.append(obj)
    return objects, None


def link(chain, ll, objects, work, name):
    exe = os.path.join(work, name)
    if chain[2] is None:
        commands = [[chain[3], "-fsanitize=address", "-Wno-override-module", ll] + objects + ["-o", exe]]
    else:
        obj = os.path.join(work, name + ".o")
        commands = [[chain[2], "-c", "-fsanitize=address", "-Wno-override-module", ll, "-o", obj],
                    [chain[3], "-fsanitize=address", obj] + objects + ["-o", exe]]
    for command in commands:
        result = run(command)
        if result.returncode != 0:
            return None, result.stdout + result.stderr
    return exe, None


def programs():
    found = []
    for program in sorted(glob.glob(os.path.join(ROOT, "tests", "runtime", "*.inox"))):
        base = os.path.splitext(program)[0]
        if not (os.path.exists(base + ".out") or os.path.exists(base + ".trap")):
            continue
        with open(program, encoding="utf-8") as handle:
            if OWNERS.search(handle.read()):
                found.append(program)
    return found


def expected_stdout(base):
    if not os.path.exists(base + ".out"):
        return None
    with open(base + ".out", encoding="utf-8") as handle:
        lines = handle.read().replace("\r\n", "\n").split("\n")
    return "\n".join(line for line in lines
                     if not line.startswith("Inox runtime error:") and not line.startswith("inox:"))


def main(argv):
    if len(argv) < 2:
        print(__doc__)
        return 2
    if not sys.platform.startswith("linux"):
        print("SKIP owner_safety_check: Linux only")
        return 0
    inox = os.path.abspath(argv[1])
    chains = toolchains()
    if not chains:
        print("SKIP owner_safety_check: needs clang++, or clang and g++")
        return 0
    environment = dict(os.environ, INOX_STDLIB=os.path.join(ROOT, "stdlib"), ASAN_OPTIONS="detect_leaks=0")
    failures = 0
    checked = 0
    with tempfile.TemporaryDirectory(prefix="inox-owner-safety-") as work:
        chain = None
        objects = None
        error = None
        for candidate in chains:
            built = os.path.join(work, candidate[0])
            os.makedirs(built, exist_ok=True)
            objects, error = build_runtime(candidate[1], built)
            if objects is None:
                continue
            probe, error = link(candidate, *probe_program(work, inox, environment), objects, work, "probe")
            if probe is not None:
                chain = candidate
                break
        if chain is None:
            print("FAIL owner_safety_check: no AddressSanitizer toolchain works\n" + (error or ""))
            return 1
        for program in programs():
            name = os.path.splitext(os.path.basename(program))[0]
            base = os.path.splitext(program)[0]
            emitted = run([inox, "--emit-llvm", program], cwd=os.path.dirname(program), env=environment)
            if emitted.returncode != 0:
                continue  # a program that does not compile is the suite's concern
            ll = os.path.join(work, name + ".ll")
            with open(ll, "w", encoding="utf-8") as handle:
                handle.write(instrumented(emitted.stdout))
            exe, error = link(chain, ll, objects, work, name)
            if exe is None:
                failures += 1
                print(f"FAIL {name}: link\n{error}")
                continue
            input_path = base + ".in"
            with open(input_path, "rb") if os.path.exists(input_path) else open(os.devnull, "rb") as stdin:
                result = subprocess.run([exe], stdin=stdin, capture_output=True, env=environment,
                                        cwd=work, timeout=120)
            stdout = result.stdout.decode("utf-8", "replace").replace("\r\n", "\n")
            stderr = result.stderr.decode("utf-8", "replace")
            problems = []
            if MARKER in stderr:
                problems.append("sanitizer report:\n" + stderr)
            trap = base + ".trap"
            if os.path.exists(trap):
                with open(trap, encoding="utf-8") as handle:
                    message = handle.readline().strip()
                if result.returncode == 0 or message not in stdout + stderr:
                    problems.append(f"expected the program to stop with: {message}")
            elif result.returncode != 0:
                problems.append(f"exit code {result.returncode}")
            expected = expected_stdout(base)
            if expected is not None and stdout.rstrip("\n") != expected.rstrip("\n"):
                problems.append(f"output differs\nexpected:\n{expected}\nactual:\n{stdout}")
            checked += 1
            if problems:
                failures += 1
                print(f"FAIL {name}: " + "\n".join(problems))
    print(f"owner_safety_check: {checked - failures} passed, {failures} failed ({chain[0]})")
    return 1 if failures else 0


def probe_program(work, inox, environment):
    source = os.path.join(work, "probe.inox")
    with open(source, "w", encoding="utf-8") as handle:
        handle.write("Main :\n    PutLn(1)\n;\n")
    emitted = run([inox, "--emit-llvm", source], cwd=work, env=environment)
    ll = os.path.join(work, "probe.ll")
    with open(ll, "w", encoding="utf-8") as handle:
        handle.write(instrumented(emitted.stdout))
    return (ll,)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
