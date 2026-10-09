# SPDX-License-Identifier: MPL-2.0
# Copyright (c) 2026 Marcelo Fortes and Inox contributors. All rights reserved.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.
"""Check that every caught exception state is released exactly once.

For each tests/eh-lifetime/NAME.inox the tool emits LLVM IR with inox, links it
against libinoxrt and tools/eh_state_counter.cpp with GNU ld --wrap, runs it,
and requires (1) the program output to equal NAME.out and (2) the number of
exception states captured to equal the number released or consumed by a
rethrow. A leaked state is invisible to the regular suite, which only compares
output.

Usage: python tools/eh_state_balance.py path/to/inox [path/to/libinoxrt.a]

Linux/GNU ld only (it relies on --wrap and the Itanium EH path); elsewhere it
prints SKIP and exits 0.
"""

import os
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BALANCE = re.compile(r"\[eh-state\] captures=(\d+) releases=(\d+) live=(\d+)")


def main(argv):
    if len(argv) < 2:
        print(__doc__)
        return 2
    inox = os.path.abspath(argv[1])
    runtime = os.path.abspath(argv[2]) if len(argv) > 2 else os.path.join(os.path.dirname(inox), "libinoxrt.a")
    clang = shutil.which("clang")
    cxx = shutil.which("clang++") or shutil.which("g++")
    if not sys.platform.startswith("linux") or clang is None or cxx is None:
        print("SKIP: eh_state_balance needs Linux, clang and a C++ compiler")
        return 0
    if not os.path.exists(runtime):
        print("error: runtime library not found: " + runtime)
        return 2

    fixtures_dir = os.path.join(ROOT, "tests", "eh-lifetime")
    fixtures = sorted(name for name in os.listdir(fixtures_dir) if name.endswith(".inox"))
    failures = 0
    with tempfile.TemporaryDirectory() as work:
        counter = os.path.join(work, "counter.o")
        subprocess.run([cxx, "-std=c++20", "-c", os.path.join(ROOT, "tools", "eh_state_counter.cpp"), "-o", counter],
                       check=True)
        for name in fixtures:
            source = os.path.join(fixtures_dir, name)
            base = os.path.join(work, name[:-5])
            with open(source[:-5] + ".out", encoding="utf-8") as stream:
                expected = stream.read()
            emitted = subprocess.run([inox, "--emit-llvm", source], capture_output=True)
            if emitted.returncode != 0:
                print("[FAIL] %s: emission failed: %s" % (name, emitted.stderr.decode(errors="replace").strip()))
                failures += 1
                continue
            with open(base + ".ll", "wb") as stream:
                stream.write(emitted.stdout)
            subprocess.run([clang, "-c", "-w", base + ".ll", "-o", base + ".o"], check=True)
            subprocess.run([cxx, base + ".o", counter, runtime,
                            "-Wl,--wrap=__inox_exception_capture,--wrap=__inox_exception_release,"
                            "--wrap=__inox_exception_rethrow",
                            "-o", base + ".exe"], check=True)
            result = subprocess.run([base + ".exe"], capture_output=True, timeout=60)
            output = result.stdout.decode("utf-8", errors="replace")
            match = BALANCE.search(result.stderr.decode("utf-8", errors="replace"))
            problems = []
            if result.returncode != 0:
                problems.append("exit status %d" % result.returncode)
            if output != expected:
                problems.append("output differs from %s" % (name[:-5] + ".out"))
            if match is None:
                problems.append("no exception state was captured")
            elif match.group(3) != "0":
                problems.append("%s of %s exception states leaked" % (match.group(3), match.group(1)))
            if problems:
                print("[FAIL] %s: %s" % (name, "; ".join(problems)))
                failures += 1
            else:
                print("[PASS] %s: %s exception states captured and released" % (name, match.group(1)))
    print("\nSummary: %d passed, %d failed" % (len(fixtures) - failures, failures))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
