#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
# Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
"""Snapshot regression check for refactors (canon E19, v3.54).

Records what the compiler says about every program in the repository (tests,
examples, stdlib): the LLVM IR of `--emit-llvm`, the types of `--dump-types`,
their diagnostics and exit codes. A refactor that must not change behavior is
checked by recording with the compiler before the change and comparing with
the compiler after it; every difference must be listed as intended.

Usage
  tools/ir_snapshot.py record  <inox> <snapshot-dir>
  tools/ir_snapshot.py compare <inox> <snapshot-dir> [--allow PATH ...] [--expect FILE ...]
  tools/ir_snapshot.py expect  <inox> <snapshot-dir> PATH ...  > FILE

`--allow` names a program (repository-relative path) that the change adds
or removes: one missing from a snapshot, or (when the base compiler was
recorded on the head's programs, as CI does) one whose base output is not
compared. It must name a program of the snapshots, and it is not how an
intended difference is declared: CI derives it from the files the pull
request adds or removes, never from a list someone writes.

An intended difference in an existing program is declared exactly in an
expectation file (JSON), written by `expect` from the snapshot and the new
compiler and reviewed with the change (change ir-snapshot-exact, r3.115):
for each program and each mode that changes, the exit codes and diagnostics
before and after, and a SHA-256 of the whole output before and after.
`compare --expect FILE` accepts a difference only when it is exactly the
one declared; it fails on any other difference of that program, on a mode
declared but unchanged, and on a program declared that no longer differs
(an exception no longer needed).

`compare` exits 1 on any difference not declared this way.

The snapshot sets INOX_STDLIB to the repository's stdlib/, so a compiler
binary copied elsewhere still finds Std.* modules (a copied binary otherwise
reports "could not resolve module" and pollutes the comparison).
"""

from __future__ import annotations

import hashlib
import json
import os
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
MODES = ("--emit-llvm", "--dump-types")
SOURCES = ("tests", "examples", "stdlib")
TIMEOUT_SECONDS = 30


def programs() -> list[Path]:
    found: list[Path] = []
    for folder in SOURCES:
        found.extend(sorted((ROOT / folder).rglob("*.inox")))
    return found


def run(compiler: str, program: Path) -> dict[str, dict[str, object]]:
    environment = dict(os.environ, INOX_STDLIB=str(ROOT / "stdlib"))
    outputs: dict[str, dict[str, object]] = {}
    for mode in MODES:
        try:
            result = subprocess.run([compiler, mode, program.name], cwd=program.parent,
                                    capture_output=True, timeout=TIMEOUT_SECONDS, env=environment)
            outputs[mode] = {
                "exit": result.returncode,
                "stdout": result.stdout.decode("utf-8", "replace"),
                "stderr": result.stderr.decode("utf-8", "replace"),
            }
        except subprocess.TimeoutExpired:
            outputs[mode] = {"exit": None, "stdout": "", "stderr": "timeout"}
    return outputs


def snapshot(compiler: str) -> dict[str, dict[str, dict[str, object]]]:
    compiler = str(Path(compiler).resolve())
    sources = programs()
    with ThreadPoolExecutor(max_workers=os.cpu_count() or 4) as pool:
        results = pool.map(lambda program: run(compiler, program), sources)
    return {program.relative_to(ROOT).as_posix(): outputs
            for program, outputs in zip(sources, results)}


def command_record(compiler: str, directory: Path) -> int:
    directory.mkdir(parents=True, exist_ok=True)
    data = snapshot(compiler)
    (directory / "snapshot.json").write_text(json.dumps(data, indent=1, sort_keys=True),
                                             encoding="utf-8")
    print(f"recorded {len(data)} programs in {directory}")
    return 0


def digest(output: dict[str, object]) -> str:
    """The SHA-256 of one mode's whole output (exit code, stdout, stderr)."""
    text = json.dumps(output, sort_keys=True)
    return hashlib.sha256(text.encode("utf-8")).hexdigest()


def difference(old: dict[str, object], new: dict[str, object]) -> dict[str, object]:
    """One mode's difference as an expectation file records it."""
    return {
        "exit": [old["exit"], new["exit"]],
        "stderr": [old["stderr"], new["stderr"]],
        "sha256": [digest(old), digest(new)],
    }


def changed_modes(before: dict[str, object], after: dict[str, object]) -> list[str]:
    return [mode for mode in MODES if before[mode] != after[mode]]


def command_expect(compiler: str, directory: Path, names: list[str]) -> int:
    before = json.loads((directory / "snapshot.json").read_text(encoding="utf-8"))
    after = snapshot(compiler)
    expected: dict[str, dict[str, object]] = {}
    for name in names:
        if name not in before or name not in after:
            print(f"ir snapshot: {name} is not in both snapshots (an added program needs --allow)", file=sys.stderr)
            return 2
        modes = changed_modes(before[name], after[name])
        if not modes:
            print(f"ir snapshot: {name} does not differ", file=sys.stderr)
            return 2
        expected[name] = {mode: difference(before[name][mode], after[name][mode]) for mode in modes}
    print(json.dumps(expected, indent=1, sort_keys=True, ensure_ascii=False))
    return 0


def command_compare(compiler: str, directory: Path, allowed: set[str],
                    expected: dict[str, dict[str, object]]) -> int:
    before = json.loads((directory / "snapshot.json").read_text(encoding="utf-8"))
    after = snapshot(compiler)
    unexpected = 0
    for name in sorted(set(before) | set(after)):
        if name in allowed and name in before and name in after:
            # CI records the base compiler on the head's programs, so a
            # program the change adds is in both snapshots; what the base
            # compiler said about it means nothing (r3.116).
            print(f"added (intended, not compared): {name}")
            continue
        if name not in before or name not in after:
            # A program named by --allow may appear or disappear (a test the
            # change adds or removes); any other is unexpected.
            mark = " (intended)" if name in allowed else ""
            print(f"{'added' if name not in before else 'removed'}{mark}: {name}")
            if name not in allowed:
                unexpected += 1
            continue
        changed = changed_modes(before[name], after[name])
        declared = expected.get(name, {})
        if not changed and not declared:
            continue
        if not changed:
            print(f"UNEXPECTED: {name}: declared in an expectation file, but it no longer differs")
            unexpected += 1
            continue
        actual = {mode: difference(before[name][mode], after[name][mode]) for mode in changed}
        exact = actual == declared
        mark = "intended (exact)" if exact else "UNEXPECTED"
        print(f"{mark}: {name} ({', '.join(changed)})")
        if exact:
            continue
        unexpected += 1
        if declared:
            for mode in sorted(set(declared) - set(actual)):
                print(f"    {mode}: declared, but unchanged")
            for mode in sorted(set(actual) - set(declared)):
                print(f"    {mode}: changed, but not declared")
            for mode in sorted(set(actual) & set(declared)):
                if actual[mode] != declared[mode]:
                    print(f"    {mode}: differs from the declared difference")
        for mode in changed:
            old, new = before[name][mode], after[name][mode]
            if old["exit"] != new["exit"] or old["stderr"] != new["stderr"]:
                print(f"    {mode}: exit {old['exit']} -> {new['exit']}")
                print(f"    before: {str(old['stderr']).strip()[:160]}")
                print(f"    after:  {str(new['stderr']).strip()[:160]}")
    for name in sorted(allowed - (set(after) | set(before))):
        print(f"UNEXPECTED: --allow names a program in neither snapshot: {name}")
        unexpected += 1
    for name in sorted(set(expected) - (set(after) & set(before))):
        print(f"UNEXPECTED: an expectation names a program not in both snapshots: {name}")
        unexpected += 1
    print(f"ir snapshot: {len(after)} programs, {unexpected} unexpected differences")
    return 1 if unexpected else 0


def main(argv: list[str]) -> int:
    if len(argv) < 4 or argv[1] not in ("record", "compare", "expect"):
        print(__doc__)
        return 2
    compiler, directory = argv[2], Path(argv[3])
    if argv[1] == "record":
        return command_record(compiler, directory)
    if argv[1] == "expect":
        if len(argv) < 5:
            print(__doc__)
            return 2
        return command_expect(compiler, directory, argv[4:])
    allowed: set[str] = set()
    expected: dict[str, dict[str, object]] = {}
    rest = argv[4:]
    while rest:
        if rest[0] not in ("--allow", "--expect") or len(rest) < 2:
            print(__doc__)
            return 2
        if rest[0] == "--allow":
            allowed.add(rest[1])
        else:
            for name, modes in json.loads(Path(rest[1]).read_text(encoding="utf-8")).items():
                if name in expected:
                    print(f"ir snapshot: {name} is declared in two expectation files")
                    return 2
                expected[name] = modes
        rest = rest[2:]
    return command_compare(compiler, directory, allowed, expected)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
