# SPDX-License-Identifier: MPL-2.0
# Copyright (c) 2026 Marcelo Fortes and Inox contributors. All rights reserved.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.
"""Mutation fuzzing of the Inox compiler front end and backend.

Takes every .inox file under tests/ and examples/ as a seed, applies small
random edits (delete, insert a token, duplicate a span, truncate, insert a
random character, swap two lines) and runs ``inox --emit-llvm`` on the result.

Every mutant must end in one of two ways:

  exit 0   the program was accepted and lowered
  exit 1   an ordinary diagnostic (lexer, parser, semantic, or
           "not yet implemented in the LLVM backend")

Anything else is reported as a finding and makes the tool exit with 1:

  CRASH    any other exit status, a signal, or sanitizer output
  TIMEOUT  the compiler did not finish within --timeout seconds
  CGERR    a codegen error after semantic acceptance that is not reported as
           "not yet implemented": the backend found a problem semantic
           analysis should have found (CANON E11)

Best run against a build with AddressSanitizer and UndefinedBehaviorSanitizer
(cmake -DINOX_ENABLE_ASAN=ON -DINOX_ENABLE_UBSAN=ON), which turns silent memory
errors into crashes. Runs on any OS with Python 3.8+.

Usage:
  python tools/mutation_fuzz.py <path-to-inox> [--count 4000] [--seed 1]
                                [--jobs 4] [--timeout 60] [--save-dir DIR]

The same --seed reproduces the same mutants. With --save-dir, every finding is
written there as a .inox file so it can become a regression test.
"""

import argparse
import concurrent.futures
import collections
import os
import random
import subprocess
import sys
import tempfile

REPOSITORY = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

TOKENS = [
    "(", ")", ";", ":", "..", "=", ":=", "+", "-", "*", "div", "mod", "^",
    "shl", "shr", "not", "and", "or", "if", "elif", "else", "while", "repeat",
    "until", "for", "in", "try", "except", "finally", "On", "Raise", "Retry",
    "break", "continue", "Return", "Exit", "case", "otherwise", "with", "Type",
    "Struct", "Const", "State", "Main", "Module", "Use", "Var",
    "9223372036854775807", "-9223372036854775808", "9223372036854775808",
    "$FFFFFFFFFFFFFFFF", "0", "1.5", "\"s\"", "\n", "    ", "==", "X", "I",
    "Self", ".", ",", "true",
]


def load_seeds():
    seeds = []
    for folder in ("tests", "examples"):
        for root, _directories, files in os.walk(os.path.join(REPOSITORY, folder)):
            for name in sorted(files):
                if name.endswith(".inox"):
                    with open(os.path.join(root, name), encoding="utf-8", errors="replace") as stream:
                        seeds.append(stream.read())
    return seeds


def mutate(text, rng):
    for _ in range(rng.randint(1, 4)):
        length = len(text)
        if length == 0:
            text = "Module M\n"
            continue
        position = rng.randrange(length)
        operation = rng.randrange(6)
        if operation == 0:
            text = text[:position] + text[position + rng.randint(1, 20):]
        elif operation == 1:
            text = text[:position] + rng.choice(TOKENS) + " " + text[position:]
        elif operation == 2:
            start = rng.randrange(length)
            end = min(length, start + rng.randint(1, 80))
            text = text[:position] + text[start:end] + text[position:]
        elif operation == 3:
            text = text[:position]
        elif operation == 4:
            text = text[:position] + chr(rng.randint(0, 0x2FF)) + text[position:]
        else:
            lines = text.split("\n")
            first, second = rng.randrange(len(lines)), rng.randrange(len(lines))
            lines[first], lines[second] = lines[second], lines[first]
            text = "\n".join(lines)
    return text


def run_one(compiler, seeds, seed, index, timeout):
    rng = random.Random(seed * 1000003 + index)
    source = mutate(rng.choice(seeds), rng)
    handle, path = tempfile.mkstemp(suffix=".inox")
    try:
        with os.fdopen(handle, "wb") as stream:
            stream.write(source.encode("utf-8", errors="replace"))
        environment = dict(os.environ)
        environment.setdefault("ASAN_OPTIONS", "detect_leaks=1")
        environment.setdefault("UBSAN_OPTIONS", "halt_on_error=1:print_stacktrace=1")
        try:
            result = subprocess.run([compiler, "--emit-llvm", path], capture_output=True,
                                    timeout=timeout, env=environment)
        except subprocess.TimeoutExpired:
            return "TIMEOUT", index, "", source
        errors = result.stderr.decode("utf-8", errors="replace")
        if result.returncode not in (0, 1) or "Sanitizer" in errors or "runtime error:" in errors:
            return "CRASH", index, "exit %d: %s" % (result.returncode, errors[:400]), source
        if result.returncode == 1 and errors.startswith("codegen error:") and \
                "not yet implemented in the LLVM backend" not in errors:
            return "CGERR", index, errors.strip()[:300], source
        return ("OK" if result.returncode == 0 else "REJECTED"), index, "", ""
    finally:
        os.unlink(path)


def main():
    parser = argparse.ArgumentParser(description="Mutation fuzzing of the Inox compiler.")
    parser.add_argument("compiler", help="path to the inox executable")
    parser.add_argument("--count", type=int, default=4000, help="number of mutants")
    parser.add_argument("--seed", type=int, default=1, help="random seed (reproducible)")
    parser.add_argument("--jobs", type=int, default=max(1, (os.cpu_count() or 2)), help="parallel runs")
    parser.add_argument("--timeout", type=float, default=60.0, help="seconds per compiler run")
    parser.add_argument("--save-dir", default=None, help="write every finding here as .inox")
    arguments = parser.parse_args()

    seeds = load_seeds()
    if not seeds:
        print("no .inox seeds found under tests/ or examples/")
        return 2

    counts = collections.Counter()
    findings = []
    with concurrent.futures.ThreadPoolExecutor(arguments.jobs) as pool:
        jobs = [pool.submit(run_one, arguments.compiler, seeds, arguments.seed, index, arguments.timeout)
                for index in range(arguments.count)]
        for job in concurrent.futures.as_completed(jobs):
            kind, index, detail, source = job.result()
            counts[kind] += 1
            if kind in ("CRASH", "TIMEOUT", "CGERR"):
                findings.append((kind, index, detail, source))

    findings.sort(key=lambda finding: finding[1])
    for kind, index, detail, source in findings[:20]:
        print("==== %s (seed %d, mutant %d) %s" % (kind, arguments.seed, index, detail))
        print(source[:800])
    if arguments.save_dir and findings:
        os.makedirs(arguments.save_dir, exist_ok=True)
        for kind, index, _detail, source in findings:
            name = "%s-seed%d-%d.inox" % (kind.lower(), arguments.seed, index)
            with open(os.path.join(arguments.save_dir, name), "w", encoding="utf-8", newline="\n") as stream:
                stream.write(source)

    print("Summary: " + ", ".join("%s=%d" % item for item in sorted(counts.items())) +
          " (seed %d, %d mutants)" % (arguments.seed, arguments.count))
    return 1 if findings else 0


if __name__ == "__main__":
    sys.exit(main())
