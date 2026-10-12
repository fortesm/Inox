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
  tools/ir_snapshot.py compare <inox> <snapshot-dir> [--allow PATH ...]

`--allow` names a program (repository-relative path) whose outputs may
differ; the report still lists it, marked "intended". `compare` exits 1 when
an unlisted program differs or a program appears or disappears.

The snapshot sets INOX_STDLIB to the repository's stdlib/, so a compiler
binary copied elsewhere still finds Std.* modules (a copied binary otherwise
reports "could not resolve module" and pollutes the comparison).
"""

from __future__ import annotations

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


def command_compare(compiler: str, directory: Path, allowed: set[str]) -> int:
    before = json.loads((directory / "snapshot.json").read_text(encoding="utf-8"))
    after = snapshot(compiler)
    unexpected = 0
    for name in sorted(set(before) | set(after)):
        if name not in before or name not in after:
            print(f"{'added' if name not in before else 'removed'}: {name}")
            unexpected += 1
            continue
        changed = [mode for mode in MODES if before[name][mode] != after[name][mode]]
        if not changed:
            continue
        mark = "intended" if name in allowed else "UNEXPECTED"
        print(f"{mark}: {name} ({', '.join(changed)})")
        for mode in changed:
            old, new = before[name][mode], after[name][mode]
            if old["exit"] != new["exit"] or old["stderr"] != new["stderr"]:
                print(f"    {mode}: exit {old['exit']} -> {new['exit']}")
                print(f"    before: {str(old['stderr']).strip()[:160]}")
                print(f"    after:  {str(new['stderr']).strip()[:160]}")
        if name not in allowed:
            unexpected += 1
    for name in sorted(allowed - set(after)):
        print(f"note: --allow names an unknown program: {name}")
    print(f"ir snapshot: {len(after)} programs, {unexpected} unexpected differences")
    return 1 if unexpected else 0


def main(argv: list[str]) -> int:
    if len(argv) < 4 or argv[1] not in ("record", "compare"):
        print(__doc__)
        return 2
    compiler, directory = argv[2], Path(argv[3])
    if argv[1] == "record":
        return command_record(compiler, directory)
    allowed: set[str] = set()
    rest = argv[4:]
    while rest:
        if rest[0] != "--allow" or len(rest) < 2:
            print(__doc__)
            return 2
        allowed.add(rest[1])
        rest = rest[2:]
    return command_compare(compiler, directory, allowed)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
