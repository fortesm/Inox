#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
# Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
"""The decisions code generation still takes or looks up (F6, step 0).

Foundation step F6 (change typed-ir) moves every semantic decision out of
`src/compiler/codegen/` into the typed IR. This tool lists, from the
sources, what codegen still does that the TIR is to carry:

  names     a comparison of a name with a literal (`equalsIgnoreCase(x,
            "Low")`, `name() == "__member"`): codegen deciding what a call is
  lookups   a question to the analysis by AST node (`typeOf(expression)`,
            `semantics_.symbolOf(...)`, `semantics_.declaredType(...)`, ...)
  shapes    a test of an AST node's kind (`AstNodeKind::CallExpression`)

Usage:
  python3 tools/codegen_decisions.py            the summary, one line per category
  python3 tools/codegen_decisions.py --list     every occurrence, with its file and line
  python3 tools/codegen_decisions.py --markdown the inventory table of docs (step 0)
  python3 tools/codegen_decisions.py --zero     fail unless every category is empty
                                                (the exit criterion of step 5)

Deterministic: files and lines in order, no addresses.
"""

import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
CODEGEN = ROOT / "src" / "compiler" / "codegen"

NAME = re.compile(r'(?:equalsIgnoreCase\([^;"]*?,\s*"(?P<a>[A-Za-z_][A-Za-z0-9_]*)"\)'
                  r'|name\(\)\s*==\s*"(?P<b>[A-Za-z_][A-Za-z0-9_]*)"'
                  r'|normalized\s*==\s*"(?P<c>[A-Za-z_][A-Za-z0-9_]*)")')
# Questions to the analysis by AST node: the SemanticResult accessors and
# the emitter's wrappers over them. `semantics_.types()` is the type
# context (types themselves), not a decision, and is not listed.
LOOKUP = re.compile(r'\b(?:semantics_?\.)?(?P<m>typeOf|symbolOf|declaredType|signatureOf|constructedStruct|'
                    r'stringCall|invariantChecks|newValueInvariant|constantValueOf|rangeCheck|needsNaturalCheck|'
                    r'fillUnit|instances|structInstances|resolvedConstant|intrinsicCallOf)\(')
SHAPE = re.compile(r'AstNodeKind::(?P<k>[A-Za-z]+)')

# Where each decision goes in the TIR (design.md of typed-ir).
TIR_FOR_NAME = {
    "__member": "Field place",
    "__index": "Element place",
    "__slice": "Slice expression",
    "__set": "set Literal",
    "__check": "ContractCheck",
    "__pure": "pure read (Read of a place)",
    "true": "Literal (a Bool's value)",
    "Main": "tir::Program entry routine",
    "Self": "Receiver place",
}
TIR_FOR_LOOKUP = {
    "typeOf": "the node's type",
    "symbolOf": "Read(place) / Call(routine)",
    "declaredType": "the declaration's type",
    "signatureOf": "tir::Program routine",
    "constructedStruct": "Construct",
    "stringCall": "Builtin(op)",
    "invariantChecks": "InvariantCheck",
    "newValueInvariant": "InvariantCheck",
    "constantValueOf": "Literal",
    "resolvedConstant": "Literal",
    "rangeCheck": "RangeCheck / Convert",
    "needsNaturalCheck": "NaturalCheck",
    "fillUnit": "Builtin(Fill) operand type",
    "instances": "tir::Program routines",
    "structInstances": "tir::Program struct types",
    "intrinsicCallOf": "Builtin(op) (module intrinsic)",
}


def occurrences():
    found = {"names": [], "lookups": [], "shapes": []}
    for path in sorted(CODEGEN.glob("*.cpp")) + sorted(CODEGEN.glob("*.h")):
        relative = path.relative_to(ROOT).as_posix()
        for number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
            stripped = line.strip()
            if stripped.startswith("//"):
                continue
            for match in NAME.finditer(line):
                name = match.group("a") or match.group("b") or match.group("c")
                found["names"].append((relative, number, name))
            for match in LOOKUP.finditer(line):
                if re.search(r"\b(?:const\s+)?[A-Za-z_:<>*&\s]+\s+FunctionEmitter::" + match.group("m") + r"\(", line):
                    continue  # the definition of a wrapper
                found["lookups"].append((relative, number, match.group("m")))
            for match in SHAPE.finditer(line):
                found["shapes"].append((relative, number, match.group("k")))
    return found


def tir_for(category, what):
    if category == "names":
        return TIR_FOR_NAME.get(what, "Builtin(op)")
    if category == "lookups":
        return TIR_FOR_LOOKUP.get(what, "?")
    return "the TIR node of that kind"


def main(argv):
    found = occurrences()
    mode = argv[1] if len(argv) > 1 else ""
    if mode == "--list":
        for category in ("names", "lookups", "shapes"):
            for relative, number, what in found[category]:
                print("%s:%d: %s %s -> %s" % (relative, number, category, what, tir_for(category, what)))
    elif mode == "--markdown":
        for category, title in (("names", "Name comparisons"), ("lookups", "Lookups by AST node"),
                                ("shapes", "AST shape tests")):
            counts = {}
            files = {}
            for relative, _, what in found[category]:
                counts[what] = counts.get(what, 0) + 1
                files.setdefault(what, set()).add(pathlib.Path(relative).name)
            print("### %s (%d)\n" % (title, len(found[category])))
            print("| What | Count | Files | In the TIR |")
            print("|---|---|---|---|")
            for what in sorted(counts, key=lambda key: (-counts[key], key)):
                print("| `%s` | %d | %s | %s |" % (what, counts[what], ", ".join(sorted(files[what])),
                                                tir_for(category, what)))
            print()
    print("codegen_decisions: %d name comparisons, %d lookups by AST node, %d AST shape tests" %
          (len(found["names"]), len(found["lookups"]), len(found["shapes"])),
          file=sys.stderr if mode == "--markdown" else sys.stdout)
    if mode == "--zero":
        return 0 if not any(found.values()) else 1
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
