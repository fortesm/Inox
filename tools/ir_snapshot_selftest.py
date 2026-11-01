#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
# Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
"""Checks the rules of `tools/ir_snapshot.py compare` without a compiler.

Change ir-snapshot-exact (r3.115): an intended difference passes only when
it is exactly the declared one; anything else fails. The snapshots here are
made up; `snapshot` is replaced so no program is compiled.

Usage: python3 tools/ir_snapshot_selftest.py   (from anywhere)
"""

import contextlib
import io
import json
import pathlib
import sys
import tempfile

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import ir_snapshot  # noqa: E402


def output(exit_code, stderr="", stdout=""):
    return {"exit": exit_code, "stderr": stderr, "stdout": stdout}


def program(llvm, types):
    return {"--emit-llvm": llvm, "--dump-types": types}


BEFORE = {
    "a.inox": program(output(0, stdout="ir a"), output(0, stdout="types a")),
    "b.inox": program(output(1, "b:1:1: semantic error: x\n"), output(1, "b:1:1: semantic error: x\n")),
}


def run(after, allowed=(), expected=None):
    """compare's exit code and report for these snapshots."""
    with tempfile.TemporaryDirectory() as folder:
        pathlib.Path(folder, "snapshot.json").write_text(json.dumps(BEFORE), encoding="utf-8")
        ir_snapshot.snapshot = lambda compiler: after
        report = io.StringIO()
        with contextlib.redirect_stdout(report):
            code = ir_snapshot.command_compare("inox", pathlib.Path(folder), set(allowed), expected or {})
        return code, report.getvalue()


def declared(name, after):
    """What `expect` would write for one program."""
    return {mode: ir_snapshot.difference(BEFORE[name][mode], after[name][mode])
            for mode in ir_snapshot.changed_modes(BEFORE[name], after[name])}


def main():
    failures = []
    count = [0]

    def check(what, result, wanted):
        count[0] += 1
        if (result[0] == 0) != wanted:
            failures.append(what + "\n" + result[1])

    new_b = program(output(1, "b:1:6: parse error: x\n"), output(1, "b:1:6: parse error: x\n"))
    after = dict(BEFORE, **{"b.inox": new_b})
    exact = {"b.inox": declared("b.inox", after)}

    check("no difference passes", run(BEFORE), True)
    check("an undeclared difference fails", run(after), False)
    check("an existing program is not excused without --allow", run(after, allowed={"a.inox"}), False)
    check("the exact declared difference passes", run(after, expected=exact), True)

    other = dict(BEFORE, **{"b.inox": program(output(1, "b:1:7: parse error: x\n"), new_b["--dump-types"])})
    check("a different difference fails", run(other, expected=exact), False)

    one_mode = dict(BEFORE, **{"b.inox": program(new_b["--emit-llvm"], BEFORE["b.inox"]["--dump-types"])})
    check("a declared mode that does not change fails", run(one_mode, expected=exact), False)

    check("an expectation no longer needed fails", run(BEFORE, expected=exact), False)

    ir_changed = dict(BEFORE, **{"a.inox": program(output(0, stdout="ir a2"), BEFORE["a.inox"]["--dump-types"])})
    ir_exact = {"a.inox": declared("a.inox", ir_changed)}
    check("an IR difference is declared by its digest", run(ir_changed, expected=ir_exact), True)
    other_ir = dict(BEFORE, **{"a.inox": program(output(0, stdout="ir a3"), BEFORE["a.inox"]["--dump-types"])})
    check("another IR difference fails", run(other_ir, expected=ir_exact), False)

    added = dict(BEFORE, **{"c.inox": program(output(0), output(0))})
    check("an added program passes with --allow", run(added, allowed={"c.inox"}), True)
    check("an added program fails without --allow", run(added), False)
    check("an --allow that names no program fails", run(BEFORE, allowed={"z.inox"}), False)
    check("a new program in both snapshots is not compared", run(after, allowed={"b.inox"}), True)

    for failure in failures:
        print("ir_snapshot_selftest: FAIL: " + failure)
    print("ir_snapshot_selftest: %d checks, %d failed" % (count[0], len(failures)))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
