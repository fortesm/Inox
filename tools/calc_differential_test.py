#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
# Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
#
# Differential test: "Is the Inox compiler a true calculator?"
#
# Generates N distinct random arithmetic expressions, evaluates each one with
# Python (the oracle) and with the Inox compiler (build+run), and compares.
# - Integer expressions are compared for EXACT equality.
# - Float expressions are compared with a tolerance (libm vs Python may differ
#   in the last ulp), default 1e-9 relative/absolute.
#
# The generator only uses operators and functions that Inox actually supports,
# in Inox syntax (so `div` not `/` for integers, `Sqrt(x)` not the root symbol,
# parentheses freely nested). This keeps failures meaningful: a mismatch means
# the evaluator is wrong, not that we wrote paper-notation the language lacks.
#
# Usage:
#   python3 tools/calc_differential_test.py --inox ./build/.../inox --count 300
#
# Exit code 0 if all pass, 1 if any mismatch (prints the failing cases).

import argparse
import math
import os
import random
import subprocess
import sys
import tempfile


# --------------------------------------------------------------------------
# Expression generators. Each returns a tuple:
#   (inox_source_expr, python_expr, kind)  where kind is "int" or "float".
# inox_source_expr  : the expression written in Inox syntax
# python_expr       : the same expression written in Python syntax
# We keep the two in lockstep so the same math is evaluated on both sides.
# --------------------------------------------------------------------------

# Float intrinsics available in Inox -> their Python equivalents.
FLOAT_FUNCS = {
    "Sqrt":   ("math.sqrt",  lambda: round(random.uniform(0.1, 200.0), 3)),
    "Sin":    ("math.sin",   lambda: round(random.uniform(-6.28, 6.28), 3)),
    "Cos":    ("math.cos",   lambda: round(random.uniform(-6.28, 6.28), 3)),
    "Tan":    ("math.tan",   lambda: round(random.uniform(-1.4, 1.4), 3)),
    "ArcTan": ("math.atan",  lambda: round(random.uniform(-50.0, 50.0), 3)),
    "Exp":    ("math.exp",   lambda: round(random.uniform(-3.0, 4.0), 3)),
    "Ln":     ("math.log",   lambda: round(random.uniform(0.1, 200.0), 3)),
    "Log10":  ("math.log10", lambda: round(random.uniform(0.1, 1000.0), 3)),
    "Log2":   ("math.log2",  lambda: round(random.uniform(0.1, 1000.0), 3)),
    "Floor":  ("math.floor", lambda: round(random.uniform(-100.0, 100.0), 3)),
    "Ceil":   ("math.ceil",  lambda: round(random.uniform(-100.0, 100.0), 3)),
    "Cosh":   ("math.cosh",  lambda: round(random.uniform(-3.0, 3.0), 3)),
    "Sinh":   ("math.sinh",  lambda: round(random.uniform(-3.0, 3.0), 3)),
    "Tanh":   ("math.tanh",  lambda: round(random.uniform(-3.0, 3.0), 3)),
}


def gen_int_expr(depth):
    """Generate an integer expression in lockstep Inox/Python syntax."""
    if depth <= 0 or random.random() < 0.35:
        v = random.randint(1, 50)
        return (str(v), str(v))
    op = random.choice(["+", "-", "*", "div", "mod", "^"])
    left_i, left_p = gen_int_expr(depth - 1)
    if op == "^":
        # Inox: integer ^ with small non-negative exponent (avoid overflow).
        base_i, base_p = gen_int_expr(depth - 1)
        exp = random.randint(0, 3)
        return (f"({base_i} ^ {exp})", f"({base_p} ** {exp})")
    if op in ("div", "mod"):
        # Use a guaranteed non-zero literal divisor so Python and Inox evaluate
        # the SAME math. Inox integer div/mod TRUNCATE toward zero (C/LLVM
        # sdiv/srem), NOT Python's floor //. We emit Python that emulates the
        # truncating semantics via the inox_div / inox_mod helpers so the oracle
        # matches Inox exactly.
        divisor = random.randint(2, 19)
        if op == "div":
            return (f"({left_i} div {divisor})", f"inox_div({left_p}, {divisor})")
        return (f"({left_i} mod {divisor})", f"inox_mod({left_p}, {divisor})")
    right_i, right_p = gen_int_expr(depth - 1)
    return (f"({left_i} {op} {right_i})", f"({left_p} {op} {right_p})")


def gen_float_expr(depth):
    """Generate a float expression in lockstep Inox/Python syntax."""
    if depth <= 0 or random.random() < 0.4:
        if random.random() < 0.5:
            # a float literal
            v = round(random.uniform(0.5, 50.0), 3)
            return (f"{v}", f"{v}")
        # a float function call
        name = random.choice(list(FLOAT_FUNCS.keys()))
        pyfn, argmaker = FLOAT_FUNCS[name]
        arg = argmaker()
        return (f"{name}({arg})", f"{pyfn}({arg})")
    op = random.choice(["+", "-", "*"])
    left_i, left_p = gen_float_expr(depth - 1)
    right_i, right_p = gen_float_expr(depth - 1)
    return (f"({left_i} {op} {right_i})", f"({left_p} {op} {right_p})")


def build_inox_program(expr, kind):
    """Wrap an expression in a minimal Inox program that prints its value."""
    return (
        "Module CalcTest\n"
        "Use Std.Math\n"
        "Main :\n"
        f"    R := {expr}\n"
        "    PutLn(R)\n"
        ";\n"
    )


def _inox_div(a, b):
    # Truncate toward zero, like C/LLVM sdiv and Inox `div`.
    q = abs(a) // abs(b)
    return q if (a < 0) == (b < 0) else -q


def _inox_mod(a, b):
    # Remainder consistent with truncating division, like LLVM srem / Inox `mod`.
    return a - _inox_div(a, b) * b


def eval_python(py_expr, kind):
    env = {"math": math, "inox_div": _inox_div, "inox_mod": _inox_mod}
    try:
        val = eval(py_expr, {"__builtins__": {}}, env)
    except Exception as e:
        return None, f"python eval error: {e}"
    return val, None


def run_inox(inox_exe, program, stdlib_dir, tmpdir):
    src = os.path.join(tmpdir, "calc.inox")
    with open(src, "w") as f:
        f.write(program)
    env = dict(os.environ)
    if stdlib_dir:
        env["INOX_STDLIB"] = stdlib_dir
    env["INOX_OUTPUT_DIR"] = tmpdir
    try:
        p = subprocess.run([inox_exe, "--run", src], capture_output=True,
                           env=env, timeout=30)
    except subprocess.TimeoutExpired:
        return None, "inox timeout"
    if p.returncode != 0:
        return None, f"inox exit {p.returncode}: {p.stderr.decode(errors='replace')[:200]}"
    out = p.stdout.decode(errors="replace").strip()
    return out, None


def compare(py_val, inox_out, kind, tol):
    try:
        if kind == "int":
            return int(inox_out) == int(py_val)
        inox_f = float(inox_out)
        py_f = float(py_val)
        if math.isnan(py_f) and math.isnan(inox_f):
            return True
        if math.isinf(py_f) or math.isinf(inox_f):
            return py_f == inox_f
        return abs(inox_f - py_f) <= tol + tol * abs(py_f)
    except (ValueError, TypeError):
        return False


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--inox", required=True, help="path to the inox executable")
    ap.add_argument("--stdlib", default=None, help="path to the stdlib directory")
    ap.add_argument("--count", type=int, default=300, help="number of distinct expressions")
    ap.add_argument("--tol", type=float, default=1e-5, help="float comparison tolerance (Inox prints 6 decimals via printf %f, so ~1e-6 absolute; 1e-5 is safe)")
    ap.add_argument("--seed", type=int, default=None, help="random seed (for reproducibility)")
    ap.add_argument("--max-show", type=int, default=20, help="max failing cases to print")
    args = ap.parse_args()

    if args.seed is not None:
        random.seed(args.seed)

    seen = set()
    cases = []
    attempts = 0
    while len(cases) < args.count and attempts < args.count * 50:
        attempts += 1
        kind = "int" if random.random() < 0.5 else "float"
        if kind == "int":
            inox_e, py_e = gen_int_expr(random.randint(2, 4))
        else:
            inox_e, py_e = gen_float_expr(random.randint(2, 4))
        if inox_e in seen:
            continue
        # Pre-filter: python must evaluate to a finite, sane number.
        val, err = eval_python(py_e, kind)
        if err is not None or val is None:
            continue
        if kind == "float" and (isinstance(val, complex) or
                                (isinstance(val, float) and (math.isinf(val) or abs(val) > 1e15))):
            continue
        if kind == "int" and abs(int(val)) > 2**62:
            continue
        seen.add(inox_e)
        cases.append((inox_e, py_e, kind, val))

    tmpdir = tempfile.mkdtemp(prefix="inox_calc_")
    passed = 0
    failures = []
    for i, (inox_e, py_e, kind, py_val) in enumerate(cases, 1):
        prog = build_inox_program(inox_e, kind)
        inox_out, err = run_inox(args.inox, prog, args.stdlib, tmpdir)
        if err is not None:
            failures.append((inox_e, py_e, kind, py_val, f"[{err}]"))
            continue
        if compare(py_val, inox_out, kind, args.tol):
            passed += 1
        else:
            failures.append((inox_e, py_e, kind, py_val, inox_out))

    total = len(cases)
    print(f"\n{'='*70}")
    print(f"INOX DIFFERENTIAL CALCULATOR TEST")
    print(f"{'='*70}")
    print(f"Generated : {total} distinct expressions")
    print(f"Passed    : {passed}")
    print(f"Failed    : {len(failures)}")
    print(f"Tolerance : {args.tol} (float); exact (int)")
    if failures:
        print(f"\n--- FAILURES (showing up to {args.max_show}) ---")
        for inox_e, py_e, kind, py_val, got in failures[:args.max_show]:
            print(f"[{kind}] {inox_e}")
            print(f"       python = {py_val}")
            print(f"       inox   = {got}")
    print(f"{'='*70}")
    if failures:
        print("RESULT: MISMATCHES FOUND — investigate the cases above.")
        sys.exit(1)
    print("RESULT: ALL PASSED — Inox agrees with Python on every expression.")
    sys.exit(0)


if __name__ == "__main__":
    main()
