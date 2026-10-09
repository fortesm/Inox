# SPDX-License-Identifier: MPL-2.0
# Copyright (c) 2026 Marcelo Fortes and Inox contributors. All rights reserved.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.
"""Measure the gap between the semantic analyzer and the LLVM backend.

Each probe is a small, legal-looking Inox program for one construct. The tool
runs the compiler twice per probe:

  * ``--dump-types``  -> does the semantic analyzer accept it?
  * ``--emit-llvm``   -> does the backend lower it?

and classifies the probe as

  OK          both phases accept it
  GAP         semantic analysis accepts it, the backend reports
              "not yet implemented in the LLVM backend" (CodegenUnsupported)
  SEMA        the semantic analyzer rejects it (not a backend gap)
  BUG         anything else after sema accepted: an internal codegen error,
              a crash, a timeout, or a backend diagnostic that is not marked
              as unsupported. A BUG is always a defect.

Usage (any OS):  python tools/backend_gaps.py path/to/inox [--markdown]

The exit status is 1 when any probe is a BUG, 0 otherwise. GAP entries are
expected while the backend is incomplete; the list exists so the gap is
visible and measured instead of discovered by users (CANON E15).
"""

import os
import subprocess
import sys
import tempfile

HEADER = "Module Probe\n\n"

# (name, construct, source) -- sources are complete modules after HEADER.
PROBES = [
    ("nested-for", "for inside for", """Main :
    for I in 1..2
        for J in 1..2
            Put(I)
        ;
    ;
;
"""),
    ("while-in-for", "while inside for", """Main :
    X Integer := 0
    for I in 1..2
        while X < I
            X := X + 1
        ;
    ;
;
"""),
    ("for-in-while", "for inside while", """Main :
    X Integer := 0
    while X < 2
        for I in 1..2
            X := X + 1
        ;
    ;
;
"""),
    ("repeat-in-for", "repeat inside for", """Main :
    X Integer := 0
    for I in 1..2
        repeat
            X := X + 1
        until X > I
        ;
    ;
;
"""),
    ("loop-if-elif", "if/elif inside a loop body", """Main :
    for I in 1..3
        if I = 1
            Put(1)
        elif I = 2
            Put(2)
        ;
    ;
;
"""),
    ("loop-if-else", "if/else inside a loop body", """Main :
    for I in 1..3
        if I = 1
            Put(1)
        else
            Put(0)
        ;
    ;
;
"""),
    ("loop-local-var", "local declaration inside a loop body", """Main :
    for I in 1..3
        T Integer := I * 2
        Put(T)
    ;
;
"""),
    ("nested-if", "if inside if (straight-line code)", """Main :
    X Integer := 1
    if X = 1
        if X > 0
            PutLn(1)
        ;
    ;
;
"""),
    ("const-use", "module Const used in an expression", """Const Limit := 10

Main :
    PutLn(Limit + 1)
;
"""),
    ("case", "case statement", """Main :
    X Integer := 1
    case X
        0 PutLn(0)
        otherwise PutLn(1)
    ;
;
"""),
    ("unless", "unless statement", """Main :
    X Integer := 1
    unless X = 0
        PutLn(1)
    ;
;
"""),
    ("string-local", "String local variable", """Main :
    S := "abc"
    PutLn(S)
;
"""),
    ("bool-local", "Boolean local variable", """Main :
    B := true
    PutLn(B)
;
"""),
    ("float-arith", "Float arithmetic", """Main :
    F := 1.5
    PutLn(F * 2.0)
;
"""),
    ("recursion", "recursive function", """Fact(N Integer) Integer :
    if N <= 1
        Return 1
    ;
    Return N * Fact(N - 1)
;

Main :
    PutLn(Fact(5))
;
"""),
    ("loop-in-function", "while loop in an Integer function", """SumTo(N Integer) Integer :
    S Integer := 0
    I Integer := 0
    while I < N
        I := I + 1
        S := S + I
    ;
    Return S
;

Main :
    PutLn(SumTo(4))
;
"""),
    ("struct-local", "struct local with field assignment", """Type
    TPoint Struct
        FX Integer
        FY Integer
    ;

Main :
    P TPoint
    P.FX := 1
    P.FY := 2
    PutLn(P.FX + P.FY)
;
"""),
    ("with", "with statement on a struct local", """Type
    TRect Struct
        Width Integer
        Height Integer
    ;

Main :
    Var
        R TRect
    ;
    with R
        .Width := 2
        .Height := 3
    ;
    PutLn(R.Width * R.Height)
;
"""),
    ("try-in-for", "try/except inside a loop body", """Main :
    for I in 1..2
        try
            PutLn(I)
        except
            PutLn(0)
        ;
    ;
;
"""),
    ("state-global", "State section variable", """State :
    Counter Integer := 0
;

Main :
    Counter := Counter + 1
    PutLn(Counter)
;
"""),
]


def run(compiler, mode, path):
    try:
        result = subprocess.run([compiler, mode, path], capture_output=True, timeout=60)
    except subprocess.TimeoutExpired:
        return None, "timeout"
    text = result.stderr.decode("utf-8", errors="replace").strip()
    return result.returncode, text.splitlines()[0] if text else ""


def classify(compiler, source):
    handle, path = tempfile.mkstemp(suffix=".inox")
    try:
        with os.fdopen(handle, "w", encoding="utf-8", newline="\n") as stream:
            stream.write(HEADER + source)
        code, message = run(compiler, "--dump-types", path)
        if code is None:
            return "BUG", "semantic analysis timed out"
        if code != 0:
            if code < 0 or code > 1:
                return "BUG", "semantic analysis crashed (exit %d)" % code
            return "SEMA", message
        code, message = run(compiler, "--emit-llvm", path)
        if code == 0:
            return "OK", ""
        if code is None:
            return "BUG", "codegen timed out"
        if "not yet implemented in the LLVM backend" in message:
            return "GAP", message.split("backend: ", 1)[-1]
        return "BUG", message or ("exit %d" % code)
    finally:
        os.unlink(path)


def main(argv):
    if len(argv) < 2:
        print(__doc__)
        return 2
    compiler = argv[1]
    markdown = "--markdown" in argv[2:]
    rows = [(name, construct) + classify(compiler, source) for name, construct, source in PROBES]
    if markdown:
        print("| Probe | Construct | Result | Detail |")
        print("|---|---|---|---|")
        for name, construct, status, detail in rows:
            print("| `%s` | %s | **%s** | %s |" % (name, construct, status, detail.replace("|", "\\|")))
    else:
        for name, construct, status, detail in rows:
            print("%-18s %-5s %s%s" % (name, status, construct, (" -- " + detail) if detail else ""))
    counts = {}
    for row in rows:
        counts[row[2]] = counts.get(row[2], 0) + 1
    print("\nSummary: " + ", ".join("%s=%d" % item for item in sorted(counts.items())))
    return 1 if counts.get("BUG") else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
