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
              a crash, a timeout, a backend diagnostic that is not marked
              as unsupported, or (when clang is on PATH) emitted LLVM IR that
              clang rejects. A BUG is always a defect.

Usage (any OS):  python tools/backend_gaps.py path/to/inox [--markdown]

The exit status is 1 when any probe is a BUG, 0 otherwise. GAP entries are
expected while the backend is incomplete; the list exists so the gap is
visible and measured instead of discovered by users (CANON E15).
"""

import os
import shutil
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
    ("until-in-if", "until inside if within repeat", """Main :
    X Integer := 0
    repeat
        X := X + 1
        if X > 1
            until X = 3
        ;
    ;
;
"""),
    ("until-across-loop", "until with a loop between it and its repeat", """Main :
    X Integer := 0
    repeat
        while X < 3
            X := X + 1
            until X = 2
        ;
    ;
;
"""),
    ("float32-conversion", "Float32 local initialized by explicit conversion", """Main :
    F Float32 := Float32(0.0)
    G Float32 := F
    PutLn(G = F)
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
    R TRect
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
    ("grouped-decl-scalar", "grouped declaration A, B, C T := X (X evaluated once)", """Next(N Integer) Integer :
    PutLn(N)
    Return N + 1
;

Main :
    A, B, C Integer := Next(10)
    PutLn(A + B + C)
;
"""),
    ("grouped-decl-struct", "grouped struct declaration P, Q TPoint := Base", """Type
    TPoint Struct
        X Integer
        Y Integer
    ;

Main :
    Base TPoint
    Base.X := 5
    P, Q TPoint := Base
    Q.X := 9
    PutLn(P.X + Q.X)
;
"""),
    ("chained-assignment", "chained assignment A := B := C := X", """Main :
    A := B := C := 4
    B := C := A + 1
    PutLn(A + B + C)
;
"""),
    ("named-struct-construction", "named struct construction TPoint(X := 1, Y := 2)", """Type
    TPoint Struct
        X Integer
        Y Integer
    ;

Main :
    P := TPoint(Y := 2, X := 1)
    PutLn(P.X + P.Y)
;
"""),
    ("for-step-expression-bounds", "for I in 1..N + 1 step N div 2", """Main :
    N := 4
    T := 0
    for I in 1..N + 1 step N div 2
        T := T + I
    ;
    PutLn(T)
;
"""),
    ("case-ada-choices", "case with | alternatives and a static range", """Main :
    X := 4
    case X
        1 | 2 PutLn(1)
        3..9 PutLn(2)
        otherwise PutLn(0)
    ;
;
"""),
    ("digit-separators", "numeric literals with the digit separator _", """Main :
    X := 1_000 + $F_F
    F := 2.5_0
    PutLn(X)
    PutLn(F > 2.0)
;
"""),
    ("compound-assignment", "compound assignment += -= *= /= ^=", """Main :
    I := 10
    I += 5
    I *= 2
    I -= 1
    F := 9.0
    F /= 3.0
    F ^= 2.0
    PutLn(I)
    PutLn(F)
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


def verify_ir(compiler, path):
    """Return None when clang accepts the emitted IR (or clang is absent),
    otherwise the first line of clang's complaint. Emitting text is not enough:
    invalid IR would only surface later as an opaque build failure."""
    clang = shutil.which("clang")
    if clang is None:
        return None
    emitted = subprocess.run([compiler, "--emit-llvm", path], capture_output=True, timeout=60)
    handle, ir_path = tempfile.mkstemp(suffix=".ll")
    object_path = ir_path[:-3] + ".o"
    try:
        with os.fdopen(handle, "wb") as stream:
            stream.write(emitted.stdout)
        result = subprocess.run([clang, "-c", "-w", "-x", "ir", ir_path, "-o", object_path],
                                capture_output=True, timeout=120)
        if result.returncode == 0:
            return None
        lines = [line for line in result.stderr.decode("utf-8", errors="replace").splitlines()
                 if "error:" in line]
        return lines[0].split("error:", 1)[1].strip() if lines else "exit %d" % result.returncode
    finally:
        for leftover in (ir_path, object_path):
            if os.path.exists(leftover):
                os.unlink(leftover)


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
            rejection = verify_ir(compiler, path)
            if rejection is not None:
                return "BUG", "emitted LLVM IR rejected by clang: " + rejection
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
