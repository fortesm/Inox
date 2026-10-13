#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
# Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
"""Traceability of docs/INOX_CANONICAL.md during its migration to OpenSpec.

The canon is split into units so that every line belongs to exactly one unit.
openspec/migration/CANON_MAP.md records, for each unit, a content hash, a
status and a destination. The canon can be retired only when no unit is left
"planned" (approved plan, Marcelo Fortes, 2026-10-12).

Statuses
  planned     destination chosen, nothing written yet
  mirrored    a spec or document restates the unit; the canon is still the law
  copied      the unit's text is in the destination verbatim (checked here)
  approved    Marcelo Fortes approved the destination as the law (G7)
  duplicate   every line also exists in the unit named as destination
              (written `unit:ID`; checked here)
  structural  a heading with no other content (checked here)

Commands
  update   regenerate the map from the canon, keeping statuses and destinations
  check    fail if the map is stale, a destination is missing, a copied unit
           is not verbatim in its destination, or a mirrored/approved unit
           changed in the canon after its destination was written
  report   counts by status and lines still planned
  accept ID [ID...]
           record the current canon text of mirrored/approved units after
           their destination was brought up to date
"""

from __future__ import annotations

import hashlib
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CANON = ROOT / "docs" / "INOX_CANONICAL.md"
MAP = ROOT / "openspec" / "migration" / "CANON_MAP.md"
STATUSES = ("planned", "mirrored", "copied", "approved", "duplicate", "structural")
NEEDS_DESTINATION_FILE = ("mirrored", "copied", "approved")

SECTION_RE = re.compile(r"^# SECTION (\d+) - (.+)$")
HEADING_RE = re.compile(r"^(##|###) (.+)$")
LOG_RE = re.compile(r"^# ([vr]\d+\.\d+[a-z]?)\b")  # v3.xx up to v3.57, r3.NN from r3.58
GOVERNANCE_RE = re.compile(r"^# (G\d+)\. ")
GROUP_RE = re.compile(r"^# GROUP: (.+)$")
BANNER_RE = re.compile(r"^# [=#]{20,}\s*$")
BANNER_TITLE_RE = re.compile(r"^# [#\s]*([A-Z][A-Z0-9_.-]{2,}(?:\s.*)?)$")


@dataclass
class Unit:
    ident: str
    title: str
    start: int  # 1-based, inclusive
    lines: list[str] = field(default_factory=list)

    @property
    def end(self) -> int:
        return self.start + len(self.lines) - 1

    @property
    def digest(self) -> str:
        text = "\n".join(line.rstrip() for line in self.lines).strip()
        return hashlib.sha256(text.encode("utf-8")).hexdigest()[:12]


@dataclass
class Entry:
    ident: str
    lines: str
    digest: str
    status: str
    destination: str


def slug(text: str, limit: int = 40) -> str:
    text = re.sub(r"\(.*?\)|\[.*?\]", "", text)
    value = re.sub(r"[^a-z0-9]+", "-", text.lower()).strip("-")
    return value[:limit].rstrip("-") or "untitled"


def heading_ident(title: str) -> str:
    for pattern in (r"^(CANON-\d+)", r"^(ADR-\d{4})", r"^(DEV-\d+)", r"^(FUTURE-\d+)",
                    r"^(B-[A-Z]+)", r"^(E\d+)\."):
        match = re.match(pattern, title)
        if match:
            return match.group(1)
    return slug(title)


def split_units(text: str) -> list[Unit]:
    units: list[Unit] = [Unit("PREAMBLE", "file header and document map", 1)]
    parent = ""
    in_log = False
    in_governance = False
    seen: dict[str, int] = {}

    def start(ident: str, title: str, line_number: int) -> None:
        count = seen.get(ident, 0) + 1
        seen[ident] = count
        if count > 1:
            ident = f"{ident}~{count}"
        units.append(Unit(ident, title, line_number))

    previous = ""
    for number, line in enumerate(text.splitlines(), start=1):
        after_banner = BANNER_RE.match(previous) is not None
        previous = line
        section = SECTION_RE.match(line)
        group = GROUP_RE.match(line)
        heading = HEADING_RE.match(line)
        banner_title = (BANNER_TITLE_RE.match(line)
                        if after_banner and not BANNER_RE.match(line) else None)
        log = LOG_RE.match(line) if in_log else None
        governance = GOVERNANCE_RE.match(line) if in_governance else None
        if section:
            parent = f"SECTION-{section.group(1)}"
            in_log = in_governance = False
            start(parent, section.group(2).strip(), number)
        elif group:
            parent = "GROUP-" + slug(group.group(1), 20)
            in_log = in_governance = False
            start(parent, group.group(1).strip(), number)
        elif banner_title:
            title = banner_title.group(1).strip(" #")
            parent = "BANNER-" + slug(title, 30)
            in_log = in_governance = False
            start(parent, title, number)
        elif heading:
            level, title = heading.group(1), heading.group(2).strip()
            ident = heading_ident(title)
            if level == "##":
                parent = ident
                in_log = title.startswith("CHANGE LOG")
                in_governance = title.startswith("GOVERNANCE")
                start(ident, title, number)
            else:
                start(ident if re.match(r"^E\d+$", ident) else f"{parent}/{ident}", title, number)
        elif log:
            start(f"LOG/{log.group(1)}", line[2:].strip(), number)
        elif governance:
            start(governance.group(1), line[2:].strip(), number)
        else:
            units[-1].lines.append(line)
            continue
        units[-1].lines.append(line)
    return units


# Default destinations for units the map does not know yet. The map, once
# written, is the source of truth; these rules only seed new units.
DEFAULT_RULES: list[tuple[str, str]] = [
    (r"^PREAMBLE$", "openspec/README.md (file header and document map)"),
    (r"^SECTION-\d+$|^GROUP-|^BANNER-", "heading only"),
    (r"^how-to-read|^G\d+$|^GOVERNANCE|^NON-NEGOTIABLE|^AGENT-RULES|^agent-rules|^non-negotiable",
     "AGENTS.md (governance)"),
    (r"^FIRST-ORDER|^first-order|^E\d+$|^backend-strategy|^compiler-module|^BACKEND-STRATEGY",
     "docs/ENGINEERING.md"),
    (r"^CHANGE-LOG|^change-log|^LOG/", "docs/history/canon-changelog.md (frozen at retirement)"),
    (r"^ADR-(\d{4})", "openspec/changes/archive (ADR)"),
    (r"^B-PROPOSALS|^B-OPEN", "openspec/changes (one change per open item)"),
    (r"^B-|^current-backend|^current-observed|^temporary-console|/current-v3-17", "docs/STATUS.md"),
    (r"^FUTURE-|^CANON-22|^roadmap", "docs/ROADMAP.md"),
    (r"^CANON-1(/|$)", "docs/VISION.md"),
    (r"^DEV-|^release-|^portability|^SECTION-29/", "docs/RELEASE.md"),
    (r"^CANON-21|^required-regression|^examples-policy|^documentation-generation",
     "docs/ENGINEERING.md"),
]


def default_destination(ident: str) -> str:
    for pattern, destination in DEFAULT_RULES:
        if re.search(pattern, ident):
            return destination
    return "openspec/specs/language (capability to choose)"


def read_map() -> dict[str, Entry]:
    entries: dict[str, Entry] = {}
    if not MAP.exists():
        return entries
    for line in MAP.read_text(encoding="utf-8").splitlines():
        if not line.startswith("| ") or line.startswith("| Unit") or line.startswith("|--"):
            continue
        cells = [cell.strip() for cell in line.strip().strip("|").split("|")]
        if len(cells) != 5:
            raise SystemExit(f"malformed map row: {line}")
        ident, lines, digest, status, destination = cells
        ident = ident.strip("`")
        entries[ident] = Entry(ident, lines, digest.strip("`"), status, destination)
    return entries


def render_map(units: list[Unit], entries: dict[str, Entry]) -> str:
    out = [
        "<!-- SPDX-License-Identifier: MPL-2.0 -->",
        "# Canon migration map",
        "",
        "Every line of `docs/INOX_CANONICAL.md` belongs to exactly one unit below.",
        "Generated and checked by `tools/canon_trace.py` (run `update` after editing",
        "the canon; CI runs `check`). Statuses: planned, mirrored, copied (verbatim,",
        "checked), approved (law in OpenSpec, G7), duplicate. The canon is retired",
        "when no unit is planned.",
        "",
        "| Unit | Lines | Hash | Status | Destination |",
        "|------|-------|------|--------|-------------|",
    ]
    for unit in units:
        entry = entries[unit.ident]
        out.append(f"| `{unit.ident}` | {unit.start}-{unit.end} | `{entry.digest}` | "
                   f"{entry.status} | {entry.destination} |")
    return "\n".join(out) + "\n"


def destination_paths(destination: str) -> list[Path]:
    return [ROOT / match for match in re.findall(r"`([^`]+)`", destination)]


def normalize(line: str) -> str:
    return line.strip().lstrip("# ").strip()


def content_lines(unit: Unit) -> list[str]:
    result = []
    for line in unit.lines:
        value = normalize(line)
        if value and not re.fullmatch(r"[=#\s]+", value):
            result.append(value)
    return result


def command_update() -> int:
    units = split_units(CANON.read_text(encoding="utf-8"))
    old = read_map()
    entries: dict[str, Entry] = {}
    for unit in units:
        previous = old.get(unit.ident)
        if previous is None:
            if len(content_lines(unit)) <= 1:
                entries[unit.ident] = Entry(unit.ident, "", unit.digest, "structural",
                                            "heading only")
            else:
                entries[unit.ident] = Entry(unit.ident, "", unit.digest, "planned",
                                            default_destination(unit.ident))
            continue
        digest = previous.digest
        if previous.status in ("planned", "structural", "duplicate") or previous.digest == unit.digest:
            digest = unit.digest
        entries[unit.ident] = Entry(unit.ident, "", digest, previous.status, previous.destination)
    removed = sorted(set(old) - {unit.ident for unit in units})
    MAP.parent.mkdir(parents=True, exist_ok=True)
    MAP.write_text(render_map(units, entries), encoding="utf-8")
    for ident in removed:
        print(f"removed from the canon: {ident}")
    print(f"{len(units)} units written to {MAP.relative_to(ROOT)}")
    return 0


def command_check() -> int:
    units = split_units(CANON.read_text(encoding="utf-8"))
    entries = read_map()
    errors: list[str] = []
    total_lines = sum(len(unit.lines) for unit in units)
    if total_lines != len(CANON.read_text(encoding="utf-8").splitlines()):
        errors.append("internal: units do not cover every canon line")
    idents = [unit.ident for unit in units]
    for ident in sorted(set(idents) - set(entries)):
        errors.append(f"unit not in the map (run tools/canon_trace.py update): {ident}")
    for ident in sorted(set(entries) - set(idents)):
        errors.append(f"map lists a unit the canon no longer has (run update): {ident}")
    for unit in units:
        entry = entries.get(unit.ident)
        if entry is None:
            continue
        if entry.status not in STATUSES:
            errors.append(f"{unit.ident}: unknown status {entry.status}")
            continue
        if entry.status == "planned" and entry.digest != unit.digest:
            errors.append(f"{unit.ident}: map is stale (run tools/canon_trace.py update)")
        if entry.status in ("mirrored", "approved") and entry.digest != unit.digest:
            errors.append(f"{unit.ident}: changed in the canon after its destination was "
                          f"written; update {entry.destination}, then run tools/canon_trace.py accept {unit.ident}")
        if entry.status in NEEDS_DESTINATION_FILE:
            paths = destination_paths(entry.destination)
            if not paths:
                errors.append(f"{unit.ident}: status {entry.status} needs a `path` destination")
            for path in paths:
                if not path.exists():
                    errors.append(f"{unit.ident}: destination missing: {path.relative_to(ROOT)}")
        if entry.status == "structural" and len(content_lines(unit)) > 1:
            errors.append(f"{unit.ident}: marked structural but has content beyond its heading")
        if entry.status == "duplicate":
            match = re.search(r"unit:([^\s`]+)", entry.destination)
            other = next((u for u in units if match and u.ident == match.group(1)), None)
            if other is None:
                errors.append(f"{unit.ident}: duplicate needs a destination `unit:ID` that exists")
            else:
                present = set(content_lines(other))
                missing = [v for v in content_lines(unit)[1:] if v not in present]
                if missing:
                    errors.append(f"{unit.ident}: not a duplicate of {other.ident}: {missing[0][:60]}")
        if entry.status == "copied":
            target = ""
            for path in destination_paths(entry.destination):
                if path.is_dir():
                    target += "\n".join(p.read_text(encoding="utf-8")
                                        for p in sorted(path.rglob("*.md"))) + "\n"
                elif path.exists():
                    target += path.read_text(encoding="utf-8") + "\n"
            present = {normalize(line) for line in target.splitlines()}
            for value in content_lines(unit):
                if value not in present:
                    errors.append(f"{unit.ident}: line not copied verbatim: {value[:70]}")
                    break
    for error in errors:
        print(f"error: {error}")
    print(f"canon trace: {len(units)} units, {len(errors)} errors")
    return 1 if errors else 0


def command_report() -> int:
    units = split_units(CANON.read_text(encoding="utf-8"))
    entries = read_map()
    by_status: dict[str, list[int]] = {status: [0, 0] for status in STATUSES}
    for unit in units:
        status = entries[unit.ident].status if unit.ident in entries else "planned"
        by_status.setdefault(status, [0, 0])
        by_status[status][0] += 1
        by_status[status][1] += len(unit.lines)
    total = sum(len(unit.lines) for unit in units)
    for status, (count, lines) in by_status.items():
        print(f"{status:10} {count:4} units {lines:5} lines ({100 * lines / total:5.1f}%)")
    print(f"{'total':10} {len(units):4} units {total:5} lines")
    return 0


def command_accept(idents: list[str]) -> int:
    units = {unit.ident: unit for unit in split_units(CANON.read_text(encoding="utf-8"))}
    entries = read_map()
    for ident in idents:
        if ident not in units or ident not in entries:
            print(f"unknown unit: {ident}")
            return 1
        entries[ident].digest = units[ident].digest
    MAP.write_text(render_map(list(units.values()), entries), encoding="utf-8")
    print(f"accepted {len(idents)} unit(s)")
    return 0


def main(argv: list[str]) -> int:
    if len(argv) >= 3 and argv[1] == "accept":
        return command_accept(argv[2:])
    commands = {"update": command_update, "check": command_check, "report": command_report}
    if len(argv) != 2 or argv[1] not in commands:
        print(__doc__)
        return 2
    return commands[argv[1]]()


if __name__ == "__main__":
    sys.exit(main(sys.argv))
