#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
# Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.
"""The lock of the Inox constitution (G8) and the test references of every spec (G5).

docs/openspec/specs/constitution/spec.md and engineering.md are the law. They are
split into units (the text before `## Requirements` and each `### Requirement:`
of spec.md; the text before the first `## ` and each `## ` section of
engineering.md). constitution.lock records, per unit, a hash of its text and
the OpenSpec change that approved that text. A change to the law is accepted
only when the lock was updated through an approved change (one whose
proposal.md records Marcelo Fortes's approval).

Commands
  check    fail if a unit differs from its hash, is missing from the lock or
           was removed without relocking, if an approval names no approved
           change, or if a spec under docs/openspec/specs names a tests/,
           examples/ or tools/ file that does not exist
  relock --approval CHANGE [UNIT ...]
           record the current text of the given units (all changed, new and
           removed units when none is given) as approved by CHANGE
  list     print the units and their state

Usage: python3 tools/constitution_check.py check|list|relock ...
"""

from __future__ import annotations

import hashlib
import re
import sys
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CONSTITUTION = ROOT / "docs" / "openspec" / "specs" / "constitution"
LOCK = CONSTITUTION / "constitution.lock"
SPECS = ROOT / "docs" / "openspec" / "specs"
CHANGES = ROOT / "docs" / "openspec" / "changes"
LOCKED_FILES = ("spec.md", "engineering.md")
REFERENCE_RE = re.compile(r"(?<![\w/.-])((?:tests|examples|tools)/[\w./-]*\w\.\w+)")
LOCK_HEADER = (
    "# constitution.lock - written by tools/constitution_check.py relock; never edit by hand.\n"
    "# One line per locked unit: unit <TAB> sha256 prefix <TAB> approving OpenSpec change.\n"
    "# A unit changes only through relock --approval <change>, where <change> records\n"
    "# Marcelo Fortes's approval (constitution G2, G8).\n"
)


@dataclass
class Unit:
    name: str
    text: str

    @property
    def digest(self) -> str:
        normalized = "\n".join(line.rstrip() for line in self.text.splitlines()).strip()
        return hashlib.sha256(normalized.encode("utf-8")).hexdigest()[:16]


def split_sections(file_name: str, text: str, marker: str) -> list[Unit]:
    """Split at every line starting with `marker`; the text before the first is the preamble."""
    units: list[Unit] = []
    current_name = f"{file_name}#preamble"
    current: list[str] = []
    for line in text.splitlines():
        if line.startswith(marker):
            units.append(Unit(current_name, "\n".join(current)))
            current_name = f"{file_name}#{line[len(marker):].strip()}"
            current = []
        current.append(line)
    units.append(Unit(current_name, "\n".join(current)))
    return units


def constitution_units() -> list[Unit]:
    units: list[Unit] = []
    spec = (CONSTITUTION / "spec.md").read_text(encoding="utf-8")
    head, separator, requirements = spec.partition("\n## Requirements")
    units.append(Unit("spec.md#preamble", head))
    for unit in split_sections("spec.md", separator + requirements, "### Requirement: "):
        if unit.name == "spec.md#preamble":
            continue  # the "## Requirements" heading itself
        units.append(unit)
    engineering = (CONSTITUTION / "engineering.md").read_text(encoding="utf-8")
    units.extend(split_sections("engineering.md", engineering, "## "))
    names = [unit.name for unit in units]
    duplicates = sorted({name for name in names if names.count(name) > 1})
    if duplicates:
        raise SystemExit(f"constitution_check: duplicate unit names: {duplicates}")
    return units


def read_lock() -> dict[str, tuple[str, str]]:
    entries: dict[str, tuple[str, str]] = {}
    if not LOCK.exists():
        return entries
    for number, line in enumerate(LOCK.read_text(encoding="utf-8").splitlines(), 1):
        if not line.strip() or line.startswith("#"):
            continue
        parts = line.split("\t")
        if len(parts) != 3:
            raise SystemExit(f"constitution_check: {LOCK.name}:{number}: expected 3 tab-separated fields")
        entries[parts[0]] = (parts[1], parts[2])
    return entries


def write_lock(entries: dict[str, tuple[str, str]], order: list[str]) -> None:
    lines = [LOCK_HEADER]
    for name in order:
        digest, approval = entries[name]
        lines.append(f"{name}\t{digest}\t{approval}\n")
    LOCK.write_text("".join(lines), encoding="utf-8")


def approval_problem(change: str) -> str | None:
    """None when `change` is an OpenSpec change whose proposal records Marcelo Fortes's approval."""
    candidates = [CHANGES / change] + sorted((CHANGES / "archive").glob(f"*-{change}"))
    for folder in candidates:
        proposal = folder / "proposal.md"
        if proposal.is_file():
            if "Marcelo Fortes" in proposal.read_text(encoding="utf-8"):
                return None
            return f"the proposal of change {change} does not record Marcelo Fortes's approval"
    return f"no OpenSpec change named {change}"


def reference_problems() -> list[str]:
    problems = []
    for spec in sorted(SPECS.rglob("*.md")):
        for number, line in enumerate(spec.read_text(encoding="utf-8").splitlines(), 1):
            for reference in REFERENCE_RE.findall(line):
                if not (ROOT / reference).exists():
                    problems.append(f"{spec.relative_to(ROOT)}:{number}: names a missing file: {reference}")
    return problems


def check() -> int:
    units = constitution_units()
    lock = read_lock()
    problems = []
    present = set()
    for unit in units:
        present.add(unit.name)
        if unit.name not in lock:
            problems.append(f"not locked (new law needs relock --approval): {unit.name}")
            continue
        digest, approval = lock[unit.name]
        if digest != unit.digest:
            problems.append(f"changed without approval (relock --approval <change>): {unit.name}")
        problem = approval_problem(approval)
        if problem:
            problems.append(f"{unit.name}: {problem}")
    for name in sorted(set(lock) - present):
        problems.append(f"removed without approval (relock --approval <change>): {name}")
    problems.extend(reference_problems())
    for problem in problems:
        print(f"FAIL {problem}")
    print(f"constitution_check: {len(units)} locked units, {len(problems)} problem(s)")
    return 1 if problems else 0


def relock(arguments: list[str]) -> int:
    if len(arguments) < 2 or arguments[0] != "--approval":
        print(__doc__)
        return 2
    approval = arguments[1]
    wanted = set(arguments[2:])
    problem = approval_problem(approval)
    if problem:
        print(f"constitution_check: cannot relock: {problem}")
        return 1
    units = constitution_units()
    lock = read_lock()
    names = {unit.name for unit in units}
    unknown = wanted - names - set(lock)
    if unknown:
        print(f"constitution_check: unknown units: {sorted(unknown)}")
        return 1
    changed = []
    for unit in units:
        stale = lock.get(unit.name, ("", ""))[0] != unit.digest
        if (wanted and unit.name in wanted) or (not wanted and stale):
            lock[unit.name] = (unit.digest, approval)
            changed.append(unit.name)
    for name in sorted(set(lock) - names):
        if not wanted or name in wanted:
            del lock[name]
            changed.append(f"{name} (removed)")
    write_lock(lock, [unit.name for unit in units if unit.name in lock])
    for name in changed:
        print(f"relocked {name} ({approval})")
    print(f"constitution_check: {len(changed)} unit(s) relocked")
    return 0


def list_units() -> int:
    lock = read_lock()
    for unit in constitution_units():
        entry = lock.get(unit.name)
        state = "unlocked" if entry is None else ("ok" if entry[0] == unit.digest else "CHANGED")
        approval = entry[1] if entry else "-"
        print(f"{state:8} {unit.digest} {approval:24} {unit.name}")
    return 0


def main(argv: list[str]) -> int:
    if len(argv) < 2:
        print(__doc__)
        return 2
    command = argv[1]
    if command == "check":
        return check()
    if command == "relock":
        return relock(argv[2:])
    if command == "list":
        return list_units()
    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv))
