#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
# Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.
"""Generates src/runtime/UnicodeTables.inc, the Unicode data of Std.Strings.

OpenSpec change std-strings, design section 4: case mappings that take one
code point to one (so a length in characters never changes), simple case
folding for SameText and CompareText, and the White_Space property for
Trim. The data comes from the Unicode Character Database that Python ships
(`unicodedata`); its version is written in the file. A mapping is kept only
when Python's mapping of the character is one code point: characters whose
full mapping has several (`ß` -> `SS`) stay unchanged, as full mappings come
in a later change.

r3.89 (change input-chars-exponents) adds the classes of a Char: Letter
(general category L*), Decimal_Number (Nd), Uppercase_Letter (Lu),
Lowercase_Letter (Ll) and Punctuation (P*), each a sorted list of ranges
`{first, last}`.

Each table is a list of runs `{first, last, delta, stride}`: every `stride`-th
code point from `first` to `last` maps to itself plus `delta`. The runtime
finds a run by binary search on `first`.

Usage: python3 tools/gen_unicode_tables.py [output]   (default src/runtime/UnicodeTables.inc)
"""

from __future__ import annotations

import sys
import unicodedata
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

# PropList.txt, White_Space (25 code points; unchanged since Unicode 6.3).
WHITE_SPACE = [
    0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x0020, 0x0085, 0x00A0, 0x1680,
    0x2000, 0x2001, 0x2002, 0x2003, 0x2004, 0x2005, 0x2006, 0x2007, 0x2008,
    0x2009, 0x200A, 0x2028, 0x2029, 0x202F, 0x205F, 0x3000,
]


def mapping(transform) -> dict[int, int]:
    result: dict[int, int] = {}
    for code in range(0x110000):
        if 0xD800 <= code <= 0xDFFF:
            continue
        mapped = transform(chr(code))
        if len(mapped) == 1 and ord(mapped) != code:
            result[code] = ord(mapped)
    return result


def runs(table: dict[int, int]) -> list[tuple[int, int, int, int]]:
    """Groups the mapping into runs of equal delta and stride 1 or 2."""
    codes = sorted(table)
    result: list[tuple[int, int, int, int]] = []
    index = 0
    while index < len(codes):
        first = codes[index]
        delta = table[first] - first
        best = (first, first, delta, 1)
        for stride in (1, 2):
            last = first
            while last + stride in table and table[last + stride] - (last + stride) == delta:
                last += stride
            if (last - first) // stride > (best[1] - best[0]) // best[3]:
                best = (first, last, delta, stride)
        result.append(best)
        _, last, _, stride = best
        covered = set(range(first, last + 1, stride))
        while index < len(codes) and codes[index] in covered:
            index += 1
    return sorted(result)


def lookup(items: list[tuple[int, int, int, int]], code: int) -> int:
    """What the runtime does: the last run whose first is <= code."""
    low, high, found = 0, len(items) - 1, -1
    while low <= high:
        middle = (low + high) // 2
        if items[middle][0] <= code:
            found, low = middle, middle + 1
        else:
            high = middle - 1
    if found < 0:
        return code
    first, last, delta, stride = items[found]
    return code + delta if code <= last and (code - first) % stride == 0 else code


def verify(name: str, table: dict[int, int], items: list[tuple[int, int, int, int]]) -> None:
    for code in range(0x110000):
        if lookup(items, code) != table.get(code, code):
            raise SystemExit(f"{name}: the runs map U+{code:04X} wrongly; fix the generator")


def emit_runs(name: str, items: list[tuple[int, int, int, int]]) -> str:
    lines = [f"constexpr CaseRun {name}[] = {{"]
    for first, last, delta, stride in items:
        lines.append(f"    {{0x{first:05X}, 0x{last:05X}, {delta}, {stride}}},")
    lines.append("};")
    return "\n".join(lines)


def category_ranges(accept) -> list[tuple[int, int]]:
    """The ranges of code points whose general category `accept` takes."""
    result: list[tuple[int, int]] = []
    for code in range(0x110000):
        if accept(unicodedata.category(chr(code))):
            if result and result[-1][1] == code - 1:
                result[-1] = (result[-1][0], code)
            else:
                result.append((code, code))
    return result


CLASSES = (
    ("kLetterRanges", lambda category: category.startswith("L")),
    ("kDigitRanges", lambda category: category == "Nd"),
    ("kUpperRanges", lambda category: category == "Lu"),
    ("kLowerRanges", lambda category: category == "Ll"),
    ("kPunctuationRanges", lambda category: category.startswith("P")),
)


def emit_ranges(name: str, items: list[tuple[int, int]]) -> str:
    lines = [f"constexpr CodeRange {name}[] = {{"]
    for first, last in items:
        lines.append(f"    {{0x{first:05X}, 0x{last:05X}}},")
    lines.append("};")
    return "\n".join(lines)


def main(argv: list[str]) -> int:
    output = Path(argv[1]) if len(argv) > 1 else ROOT / "src" / "runtime" / "UnicodeTables.inc"
    tables = {name: mapping(transform) for name, transform in
              (("kUpperRuns", str.upper), ("kLowerRuns", str.lower), ("kFoldRuns", str.casefold))}
    upper, lower, fold = (runs(tables[name]) for name in ("kUpperRuns", "kLowerRuns", "kFoldRuns"))
    for name, items in (("kUpperRuns", upper), ("kLowerRuns", lower), ("kFoldRuns", fold)):
        verify(name, tables[name], items)
    text = "\n".join([
        "// SPDX-License-Identifier: MPL-2.0",
        "// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.",
        "//",
        "// Generated by tools/gen_unicode_tables.py from the Unicode Character",
        f"// Database {unicodedata.unidata_version} (Python unicodedata). Do not edit: run the tool.",
        "// Included by src/runtime/StringFunctions.cpp, which defines CaseRun and CodeRange.",
        "",
        f"constexpr const char* kUnicodeVersion = \"{unicodedata.unidata_version}\";",
        "",
        emit_runs("kUpperRuns", upper),
        "",
        emit_runs("kLowerRuns", lower),
        "",
        emit_runs("kFoldRuns", fold),
        "",
        "constexpr std::uint32_t kWhiteSpace[] = {",
        "    " + ", ".join(f"0x{code:04X}" for code in WHITE_SPACE) + ",",
        "};",
        "",
    ] + [emit_ranges(name, category_ranges(accept)) + "\n" for name, accept in CLASSES])
    output.write_text(text, encoding="utf-8")
    print(f"{output}: Unicode {unicodedata.unidata_version}, runs: upper {len(upper)}, "
          f"lower {len(lower)}, fold {len(fold)}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
