#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
# Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
"""Classifies the differences between the LLVM IR two compilers emit.

A change that reorders evaluation on purpose changes the IR of many
programs; `tools/ir_snapshot.py` shows that they differ, this tool says how.
Per function, the instructions of both are compared as multisets, with
temporaries, labels and string constants numbered abstractly (`%tmp#`):

  reordering          the same instructions, in another order
  redundant-removed   instructions the base had and the head lacks, all of
                      them parts of an address or of a value computed twice
                      (getelementptr, load, sub, range check, extension,
                      extractvalue, the checks of the pointers on its
                      path, a method call in it); `redundant-removed:call`
                      marks a removed call of a routine, which ran twice
  validity-check      instructions the head adds, all of them parts of the
                      check of an object's generation (CANON-25: the header,
                      its generation, the comparison, the stop of fault 16)
  compound-operation  a compound assignment of a String: the concatenation
                      that rebuilt L replaced by the append of R in place
                      (CANON-14), R evaluated first
  unexpected          anything else; the tool fails

The classification shows where the code changed, not that it is right; the
behavioral tests do that.

Usage:
  python3 tools/ir_difference_kinds.py <base-inox> <head-inox> [program.inox ...]

Without programs, every program of the IR snapshot (tests, examples,
stdlib) is compared. Exit 1 when a difference is unexpected.
"""

import collections
import os
import pathlib
import re
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
SOURCES = ("tests", "examples", "stdlib")

LOCAL = re.compile(r"%([A-Za-z_.]*?[A-Za-z_.])\d+\b")
LABEL = re.compile(r"^([A-Za-z_.]*?[A-Za-z_.])\d+:")
STRING = re.compile(r"@\.inox\.str\.\d+")

# The second evaluation of a target: its address, its loads, the checks of
# the pointers on its path (an owner moved, fault 17; a Link dead, fault
# 16), and a call in its path (a method without arguments).
REDUNDANT = re.compile(r"^(%tmp# = )?(getelementptr|load|sub|zext|sext|trunc|extractvalue|"
                       r"call i64 @__inox_range_i64|icmp eq ptr %tmp#, null|icmp eq i64 %tmp#, %tmp#|"
                       r"call i1 @llvm\.expect\.i1\(.*\)|phi i1 .*|(call|invoke) .*@inox_[^ ]*\(.*)"
                       r"|^(br .*|unreachable|call void @__inox_arith_fault\(i32 1[67]\)|"
                       r"(own|link|eh\.invoke)\.[a-z]+#:|[a-z]+\.[a-z.]+#:)$")
CALL = re.compile(r"(call|invoke) .*@inox_")
# CANON-25: the check of an object's generation.
CHECK = re.compile(r"^(%tmp# = (getelementptr i8, ptr .*, i64 -16|load i64, ptr %tmp#|icmp eq ptr %tmp#, null|"
                   r"icmp eq i64 %tmp#, %tmp#|phi i1 .*|call i1 @llvm\.expect\.i1\(.*\))|"
                   r"br .*|link\.[a-z]+#:|call void @__inox_arith_fault\(i32 16\)|unreachable)$")
# A compound assignment of a String: the concatenation `L := L + R` that
# rebuilt L, and the append of R in place that replaces it (CANON-14).
COMPOUND = re.compile(r"(%strparts#|%strtmp#|%strstatus#|@__inox_str_(append|concat|retain|release|fault)|"
                      r"str\.(fail|ok)#|inox\.entry-init|^%tmp# = load (ptr|i32), ptr|^store ptr|"
                      r"^%tmp# = icmp ne i32|^br )")


def abstract(line):
    line = line.strip()
    line = STRING.sub("@.inox.str.#", line)
    line = LOCAL.sub(lambda match: "%" + match.group(1) + "#", line)
    line = LABEL.sub(lambda match: match.group(1) + "#:", line)
    return line


def functions(ir):
    found = collections.OrderedDict()
    name = "<module>"
    found[name] = []
    for line in ir.splitlines():
        if line.startswith("define "):
            match = re.search(r"@([^\s(]+)\(", line)
            name = match.group(1) if match else line
            found[name] = []
            continue
        if line.startswith("}"):
            name = "<module>"
            continue
        text = abstract(line)
        if text and not text.startswith(";"):
            found.setdefault(name, []).append(text)
    return found


def emit(compiler, program):
    environment = dict(os.environ, INOX_STDLIB=str(ROOT / "stdlib"))
    result = subprocess.run([compiler, "--emit-llvm", str(program)], capture_output=True, text=True,
                            env=environment, timeout=60, cwd=ROOT)
    return result.returncode, result.stdout


def classify(base, head):
    kinds = set()
    for name in sorted(set(base) | set(head)):
        before = base.get(name, [])
        after = head.get(name, [])
        if before == after:
            continue
        removed = collections.Counter(before) - collections.Counter(after)
        added = collections.Counter(after) - collections.Counter(before)
        if not removed and not added:
            kinds.add("reordering")
            continue
        strings = any("@__inox_str_append" in line or "@__inox_str_concat" in line
                      for line in list(removed) + list(added))
        if strings and all(COMPOUND.search(line) or REDUNDANT.match(line) for line in removed) and \
                all(COMPOUND.search(line) or CHECK.match(line) for line in added):
            kinds.add("compound-operation")
            continue
        if not all(REDUNDANT.match(line) for line in removed) or not all(CHECK.match(line) for line in added):
            kinds.add("unexpected")
            continue
        if removed:
            kinds.add("redundant-removed")
            if any(CALL.search(line) for line in removed):
                kinds.add("redundant-removed:call")
        if added:
            kinds.add("validity-check")
    return kinds


def programs(arguments):
    if arguments:
        return [pathlib.Path(argument) for argument in arguments]
    found = []
    for folder in SOURCES:
        found.extend(sorted((ROOT / folder).rglob("*.inox")))
    return found


def main(argv):
    if len(argv) < 3:
        print(__doc__)
        return 2
    base_compiler, head_compiler = argv[1], argv[2]
    totals = collections.Counter()
    failed = False
    for program in programs(argv[3:]):
        base_code, base_ir = emit(base_compiler, program)
        head_code, head_ir = emit(head_compiler, program)
        if base_code != 0 and head_code != 0:
            continue
        if base_ir == head_ir:
            continue
        relative = program.resolve().relative_to(ROOT).as_posix()
        if base_code != head_code:
            kinds = {"unexpected"}
        else:
            kinds = classify(functions(base_ir), functions(head_ir))
        if not kinds:
            kinds = {"reordering"}
        for kind in kinds:
            totals[kind] += 1
        failed = failed or "unexpected" in kinds
        print("%s: %s" % (relative, ", ".join(sorted(kinds))))
    print("ir_difference_kinds: " + ", ".join("%s %d" % (kind, totals[kind]) for kind in sorted(totals)) if totals
          else "ir_difference_kinds: no difference")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
