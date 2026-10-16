# SPDX-License-Identifier: MPL-2.0
# Copyright (c) 2026 Marcelo Fortes and Inox contributors. All rights reserved.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.
"""Run the String and File test programs under AddressSanitizer with leak detection.

String and File values are reference counted by code the compiler inserts
(OpenSpec changes string-runtime and file-io). A missing release leaks (and
leaves a file open); an extra one frees twice or reads freed memory. The
regular suite only compares output, so it sees neither. For each
tests/runtime/string*.inox, files-*.inox, contracts*.inox and pointers*.inox (and some examples) this tool emits LLVM IR with
inox, links it with AddressSanitizer against libinoxrt, runs it with the
NAME.in input, and requires:

  - the output to equal NAME.out (NAME.trap: the program stops with that
    runtime error; leak detection is off there, because a trap ends the
    program while Strings are still owned);
  - no AddressSanitizer or LeakSanitizer report (leak, double free, use after
    free).

Usage: python tools/string_leak_check.py path/to/inox [path/to/libinoxrt.a]

The program is instrumented with clang's AddressSanitizer when clang has its
runtime; otherwise it is compiled plainly and linked with g++ -fsanitize=address,
which still catches leaks and double frees through the allocator. Linux only;
elsewhere it prints SKIP and exits 0.
"""

import glob
import os
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SANITIZER_MARKERS = ("AddressSanitizer", "LeakSanitizer")


def run(command, **kwargs):
    return subprocess.run(command, capture_output=True, text=True, **kwargs)


def link_modes(clang, gxx):
    """Ways to build an instrumented executable, best first."""
    modes = []
    if shutil.which("clang++"):
        modes.append(("clang-asan", lambda ll, obj, runtime, exe: [
            ["clang++", "-fsanitize=address", "-Wno-override-module", ll, runtime, "-o", exe]]))
    if clang and gxx:
        modes.append(("gcc-asan-link", lambda ll, obj, runtime, exe: [
            [clang, "-c", "-Wno-override-module", ll, "-o", obj],
            [gxx, "-fsanitize=address", obj, runtime, "-o", exe]]))
    return modes


def build(mode, ll, obj, runtime, exe):
    for command in mode[1](ll, obj, runtime, exe):
        result = run(command)
        if result.returncode != 0:
            return result.stdout + result.stderr
    return None


def main(argv):
    if len(argv) < 2:
        print(__doc__)
        return 2
    if not sys.platform.startswith("linux"):
        print("SKIP string_leak_check: Linux only")
        return 0
    inox = os.path.abspath(argv[1])
    runtime = os.path.abspath(argv[2]) if len(argv) > 2 else os.path.join(os.path.dirname(inox), "libinoxrt.a")
    clang = shutil.which("clang")
    gxx = shutil.which("g++")
    modes = link_modes(clang, gxx)
    if not modes:
        print("SKIP string_leak_check: needs clang++, or clang and g++")
        return 0

    programs = sorted(glob.glob(os.path.join(ROOT, "tests", "runtime", "string*.inox")) +
                      glob.glob(os.path.join(ROOT, "tests", "runtime", "files-*.inox")) +
                      glob.glob(os.path.join(ROOT, "tests", "runtime", "contracts*.inox")) +
                      glob.glob(os.path.join(ROOT, "tests", "runtime", "pointers*.inox")) +
                      glob.glob(os.path.join(ROOT, "tests", "runtime", "with-*.inox")) +
                      [os.path.join(ROOT, "examples", name)
                       for name in ("file_io.inox", "bank-atm.inox", "contracts.inox",
                                    "pointers-tree.inox", "pointers-navigator.inox")])
    if not programs:
        print("FAIL string_leak_check: no String test programs found")
        return 1

    failures = 0
    chosen = None
    with tempfile.TemporaryDirectory(prefix="inox-string-leak-") as work:
        for program in programs:
            name = os.path.splitext(os.path.basename(program))[0]
            base = os.path.splitext(program)[0]
            if os.path.dirname(program) == os.path.join(ROOT, "examples"):
                # An example's expected output lives in tests/examples.
                base = os.path.join(ROOT, "tests", "examples", os.path.basename(base))
            ll = os.path.join(work, name + ".ll")
            obj = os.path.join(work, name + ".o")
            exe = os.path.join(work, name)
            emitted = run([inox, "--emit-llvm", program], cwd=os.path.dirname(program))
            if emitted.returncode != 0:
                print(f"FAIL {name}: inox --emit-llvm\n{emitted.stderr}")
                failures += 1
                continue
            with open(ll, "w", encoding="utf-8") as handle:
                handle.write(emitted.stdout)

            error = None
            for mode in ([chosen] if chosen else modes):
                error = build(mode, ll, obj, runtime, exe)
                if error is None:
                    chosen = mode
                    break
            if error is not None:
                print(f"FAIL {name}: link with AddressSanitizer\n{error}")
                failures += 1
                continue

            trap_file = base + ".trap"
            is_trap = os.path.exists(trap_file)
            environment = dict(os.environ)
            environment["ASAN_OPTIONS"] = "detect_leaks=0" if is_trap else "detect_leaks=1"
            input_path = base + ".in"
            with open(input_path, "rb") if os.path.exists(input_path) else open(os.devnull, "rb") as stdin:
                result = subprocess.run([exe], stdin=stdin, capture_output=True, env=environment)
            stdout = result.stdout.decode("utf-8", "replace").replace("\r\n", "\n")
            stderr = result.stderr.decode("utf-8", "replace")

            problems = []
            if any(marker in stderr for marker in SANITIZER_MARKERS):
                problems.append("sanitizer report:\n" + stderr)
            if is_trap:
                with open(trap_file, encoding="utf-8") as handle:
                    expected_message = handle.readline().strip()
                if result.returncode == 0 or expected_message not in stdout + stderr:
                    problems.append(f"expected the program to stop with: {expected_message}")
            else:
                with open(base + ".out", encoding="utf-8") as handle:
                    expected = handle.read().replace("\r\n", "\n")
                if result.returncode != 0:
                    problems.append(f"exit code {result.returncode}")
                if stdout != expected:
                    problems.append(f"output differs\nexpected:\n{expected}\nactual:\n{stdout}")
            if problems:
                failures += 1
                print(f"FAIL {name}: " + "\n".join(problems))
            else:
                print(f"PASS {name} ({chosen[0]}{', trap' if is_trap else ', no leaks'})")

    print(f"string_leak_check: {len(programs) - failures} passed, {failures} failed")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
