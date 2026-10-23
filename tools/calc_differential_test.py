#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
# Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

"""Differential test of Inox expressions against an independent oracle.

Random expression trees are rendered as Inox text with only the parentheses
that the CANON-20 precedence table requires (so the compiler's precedence and
associativity are exercised, not bypassed), plus a few redundant ones. The
expected value does not come from Inox: the text is parsed again by a small
recursive-descent parser written from the law (grammar/grammar.ebnf, CANON-20,
the mixing rules) and evaluated with Inox's semantics:

  integers   Int64, checked: a case whose value leaves Int64 at any step is
             not generated (Inox would trap); `/` truncates toward zero, `mod`
             has the sign of the divisor and `rem` that of the dividend (r3.95,
             r3.96, as in Ada); `^` needs an exponent >= 0; shifts 0..63, `shl`
             is A * 2^N and checked (r3.97), `shr` arithmetic; bitand, bitor, bitxor, bitnot on two's complement
  reals      Float64; `/` and `^` real; Std.Math functions
  Bool       comparisons, and, or, xor, not; IIf evaluates one branch

Half of the batches write numbers as literals, which the compiler folds at
compile time (the constant evaluator); the other half pass each number through
a routine (`I(7)`, `F(2.5)`), so the code generator and the runtime compute
them. Each batch is one program that prints one value per line; when a batch
does not compile, its cases run one by one to name the culprit.

Usage: python3 tools/calc_differential_test.py --inox path/to/inox
       [--count N] [--seed S] [--batch B] [--tol T] [--stdlib DIR]
Exit status 0 when Inox agrees with the oracle on every case, 1 otherwise.
"""

import argparse
import math
import os
import random
import subprocess
import sys
import tempfile

INT64_MIN = -(2 ** 63)
INT64_MAX = 2 ** 63 - 1


class OracleError(Exception):
    """The expression would not compile or would trap: it is not generated.

    `trap` names the CANON-19 category when the expression would compile and
    stop at run time (with every number passed through a routine)."""

    def __init__(self, message, trap=None):
        super().__init__(message)
        self.trap = trap


# ---------------------------------------------------------------- oracle ---

WORD_OPS = {"mod", "rem", "shl", "shr", "bitand", "bitor", "bitxor", "bitnot",
            "not", "and", "or", "xor"}
FLOAT_FUNCS = {
    "Sqrt": math.sqrt, "Sin": math.sin, "Cos": math.cos, "Tan": math.tan,
    "ArcTan": math.atan, "Exp": math.exp, "Ln": math.log, "Log10": math.log10,
    "Log2": math.log2, "Floor": math.floor, "Ceil": math.ceil,
    "Cosh": math.cosh, "Sinh": math.sinh, "Tanh": math.tanh,
}
INT_FUNCS = {"Sqr", "Min", "Max", "Sign"}


def tokenize(text):
    tokens = []
    i = 0
    while i < len(text):
        c = text[i]
        if c.isspace():
            i += 1
        elif c.isdigit():
            j = i
            while j < len(text) and (text[j].isdigit() or text[j] == "_"):
                j += 1
            is_real = False
            if j + 1 < len(text) and text[j] == "." and text[j + 1].isdigit():
                is_real = True
                j += 1
                while j < len(text) and text[j].isdigit():
                    j += 1
            if j < len(text) and text[j] in "eE" and is_real:
                k = j + 1
                if k < len(text) and text[k] in "+-":
                    k += 1
                while k < len(text) and text[k].isdigit():
                    k += 1
                j = k
            tokens.append(("real" if is_real else "int", text[i:j].replace("_", "")))
            i = j
        elif c.isalpha():
            j = i
            while j < len(text) and (text[j].isalnum() or text[j] == "_"):
                j += 1
            word = text[i:j]
            tokens.append(("op" if word.lower() in WORD_OPS else "name", word))
            i = j
        elif text.startswith(("<=", ">="), i):
            tokens.append(("op", text[i:i + 2]))
            i += 2
        elif c in "+-*/^()=#<>,":
            tokens.append(("op", c))
            i += 1
        else:
            raise OracleError(f"unexpected character {c!r}")
    tokens.append(("end", ""))
    return tokens


class Node:
    def __init__(self, kind, value=None, children=(), parenthesized=False):
        self.kind = kind          # "int", "real", "bool", "call", "unary", "binary"
        self.value = value        # literal value, call name, or operator
        self.children = list(children)
        self.parenthesized = parenthesized


BITWISE_FAMILY = {"bitand": "bitand", "bitor": "bitor", "bitxor": "bitxor",
                  "shl": "shift", "shr": "shift"}
LOGICAL = {"and", "or", "xor"}


class Parser:
    """Recursive descent over the CANON-20 levels (grammar/grammar.ebnf)."""

    def __init__(self, text):
        self.tokens = tokenize(text)
        self.pos = 0

    def peek(self):
        return self.tokens[self.pos]

    def at(self, *ops):
        kind, text = self.peek()
        return kind == "op" and text.lower() in ops

    def take(self):
        token = self.tokens[self.pos]
        self.pos += 1
        return token

    def expect(self, op):
        if not self.at(op):
            raise OracleError(f"expected {op!r}, got {self.peek()[1]!r}")
        self.take()

    def parse(self):
        node = self.parse_or()
        if self.peek()[0] != "end":
            raise OracleError(f"trailing {self.peek()[1]!r}")
        return node

    # Levels 13-15: and, xor, or; two different ones need parentheses.
    def parse_or(self):
        node = self.parse_and_level("or", self.parse_xor)
        return node

    def parse_xor(self):
        return self.parse_and_level("xor", self.parse_and)

    def parse_and(self):
        return self.parse_and_level("and", self.parse_relational)

    def parse_and_level(self, op, operand):
        node = operand()
        while self.at(op):
            self.take()
            right = operand()
            for child in (node, right):
                if child.kind == "binary" and child.value in LOGICAL and child.value != op \
                        and not child.parenthesized:
                    raise OracleError("mixing and/or/xor needs parentheses")
            node = Node("binary", op, (node, right))
        return node

    # Level 12: relational, left associative.
    def parse_relational(self):
        node = self.parse_bitor()
        while self.at("=", "#", "<", "<=", ">", ">="):
            op = self.take()[1]
            node = Node("binary", op, (node, self.parse_bitor()))
        return node

    # Levels 6-9: shifts and the three bitwise families never mix bare.
    def parse_bitor(self):
        return self.parse_bitwise(("bitor",), self.parse_bitxor)

    def parse_bitxor(self):
        return self.parse_bitwise(("bitxor",), self.parse_bitand)

    def parse_bitand(self):
        return self.parse_bitwise(("bitand",), self.parse_shift)

    def parse_shift(self):
        return self.parse_bitwise(("shl", "shr"), self.parse_additive)

    def parse_bitwise(self, ops, operand):
        node = operand()
        while self.at(*ops):
            op = self.take()[1].lower()
            right = operand()
            for child in (node, right):
                if child.kind == "binary" and child.value in BITWISE_FAMILY and not child.parenthesized \
                        and BITWISE_FAMILY[child.value] != BITWISE_FAMILY[op]:
                    raise OracleError("mixing bitwise operators needs parentheses")
            node = Node("binary", op, (node, right))
        return node

    def parse_additive(self):
        node = self.parse_multiplicative()
        while self.at("+", "-"):
            op = self.take()[1]
            node = Node("binary", op, (node, self.parse_multiplicative()))
        return node

    def parse_multiplicative(self):
        node = self.parse_unary()
        while self.at("*", "/", "mod", "rem"):
            op = self.take()[1].lower()
            node = Node("binary", op, (node, self.parse_unary()))
        return node

    # Level 3: prefix operators apply to a unary_expr.
    def parse_unary(self):
        if self.at("-", "+", "not", "bitnot"):
            op = self.take()[1].lower()
            return Node("unary", op, (self.parse_unary(),))
        return self.parse_power()

    # Level 2: `^` is right associative; its right side is a power_expr, so a
    # prefix operator there needs parentheses.
    def parse_power(self):
        base = self.parse_primary()
        if self.at("^"):
            self.take()
            if self.at("-", "+", "not", "bitnot"):
                raise OracleError("a prefix operator after ^ needs parentheses")
            return Node("binary", "^", (base, self.parse_power()))
        return base

    def parse_primary(self):
        kind, text = self.take()
        if kind == "int":
            return Node("int", int(text))
        if kind == "real":
            return Node("real", float(text))
        if kind == "op" and text == "(":
            node = self.parse_or()
            self.expect(")")
            node.parenthesized = True
            return node
        if kind == "name":
            if text in ("True", "False"):
                return Node("bool", text == "True")
            if not self.at("("):
                raise OracleError(f"{text} needs arguments")
            self.take()
            arguments = [self.parse_or()]
            while self.at(","):
                self.take()
                arguments.append(self.parse_or())
            self.expect(")")
            return Node("call", text, arguments)
        raise OracleError(f"unexpected {text!r}")


def checked(value):
    if not INT64_MIN <= value <= INT64_MAX:
        raise OracleError("integer overflow", trap="integer overflow")
    return value


def trunc_div(a, b):
    q = abs(a) // abs(b)
    return q if (a < 0) == (b < 0) else -q


def evaluate(node):
    """The value of a parsed expression as (type, value), with Inox's rules."""
    if node.kind == "int":
        return ("int", checked(node.value))
    if node.kind == "real":
        return ("real", node.value)
    if node.kind == "bool":
        return ("bool", node.value)
    if node.kind == "call":
        return evaluate_call(node)
    if node.kind == "unary":
        kind, value = evaluate(node.children[0])
        op = node.value
        if op == "not":
            if kind != "bool":
                raise OracleError("not of a number")
            return ("bool", not value)
        if kind == "bool":
            raise OracleError(f"{op} of a Bool")
        if op == "bitnot":
            if kind != "int":
                raise OracleError("bitnot of a real")
            return ("int", ~value)
        if op == "-":
            return (kind, checked(-value) if kind == "int" else -value)
        return (kind, value)
    op = node.value
    left = evaluate(node.children[0])
    right = evaluate(node.children[1])
    if op in LOGICAL:
        if left[0] != "bool" or right[0] != "bool":
            raise OracleError(f"{op} needs Bools")
        a, b = left[1], right[1]
        return ("bool", a and b if op == "and" else a or b if op == "or" else a != b)
    if left[0] != right[0]:
        raise OracleError("operands of different types")
    kind, a, b = left[0], left[1], right[1]
    if op in ("=", "#", "<", "<=", ">", ">="):
        if kind == "bool" and op not in ("=", "#"):
            a, b = int(a), int(b)
        result = {"=": a == b, "#": a != b, "<": a < b, "<=": a <= b, ">": a > b, ">=": a >= b}[op]
        return ("bool", result)
    if kind == "bool":
        raise OracleError(f"{op} of Bools")
    if kind == "real":
        if op == "+":
            value = a + b
        elif op == "-":
            value = a - b
        elif op == "*":
            value = a * b
        elif op == "/":
            if b == 0:
                raise OracleError("real division by zero")
            value = a / b
        elif op == "^":
            if a <= 0:
                raise OracleError("real power of a non-positive base")
            try:
                value = math.pow(a, b)
            except OverflowError:
                raise OracleError("real out of range")
        else:
            raise OracleError(f"{op} of reals")
        if math.isinf(value) or math.isnan(value) or abs(value) > 1e15:
            raise OracleError("real out of range")
        return ("real", value)
    if op == "+":
        return ("int", checked(a + b))
    if op == "-":
        return ("int", checked(a - b))
    if op == "*":
        return ("int", checked(a * b))
    if op in ("/", "mod", "rem"):
        if b == 0:
            raise OracleError("division by zero", trap="division by zero")
        if op == "/":
            return ("int", checked(trunc_div(a, b)))
        if op == "mod":
            return ("int", a % b)  # Python's % has the sign of the divisor, as Ada's mod
        return ("int", a - trunc_div(a, b) * b)
    if op == "^":
        if b < 0:
            raise OracleError("negative exponent")
        if b > 64 and abs(a) > 1:
            raise OracleError("integer overflow")
        return ("int", checked(a ** b))
    if op in ("shl", "shr"):
        if not 0 <= b <= 63:
            raise OracleError("shift count")
        if op == "shr":
            return ("int", a >> b)
        # r3.97 (CANON-8): A shl N is A * 2^N, checked as any product.
        return ("int", checked(a << b))
    if op == "bitand":
        return ("int", a & b)
    if op == "bitor":
        return ("int", a | b)
    if op == "bitxor":
        return ("int", a ^ b)
    raise OracleError(f"unknown operator {op}")


def evaluate_call(node):
    name = node.value
    if name == "IIf":
        if len(node.children) != 3:
            raise OracleError("IIf takes three arguments")
        condition = evaluate(node.children[0])
        if condition[0] != "bool":
            raise OracleError("IIf needs a Bool")
        # Only the chosen branch runs, but both are checked (and a constant
        # one folded) at compile time: both must be valid and of one type.
        first = evaluate(node.children[1])
        second = evaluate(node.children[2])
        if first[0] != second[0]:
            raise OracleError("IIf branches of different types")
        return first if condition[1] else second
    arguments = [evaluate(child) for child in node.children]
    if name == "I":
        return arguments[0]
    if name == "F":
        return arguments[0]
    if name == "Abs":
        kind, value = arguments[0]
        return (kind, checked(abs(value)) if kind == "int" else abs(value))
    if name in INT_FUNCS:
        if any(kind != "int" for kind, _ in arguments):
            raise OracleError(f"{name} takes integers")
        values = [value for _, value in arguments]
        if name == "Sqr":
            return ("int", checked(values[0] * values[0]))
        if name == "Min":
            return ("int", min(values))
        if name == "Max":
            return ("int", max(values))
        return ("int", (values[0] > 0) - (values[0] < 0))
    if name in FLOAT_FUNCS:
        kind, value = arguments[0]
        if kind != "real":
            raise OracleError(f"{name} takes a real")
        try:
            result = FLOAT_FUNCS[name](value)
        except (ValueError, OverflowError):
            raise OracleError(f"{name} out of its domain")
        if name in ("Floor", "Ceil"):
            result = float(result)
        if math.isinf(result) or math.isnan(result) or abs(result) > 1e15:
            raise OracleError("real out of range")
        return ("real", result)
    raise OracleError(f"unknown routine {name}")


def oracle(text):
    return evaluate(Parser(text).parse())


# ------------------------------------------------------------- generator ---

# The level of each operator (CANON-20); a primary is level 1.
LEVEL = {"^": 2, "*": 4, "/": 4, "mod": 4, "rem": 4, "+": 5, "-": 5,
         "shl": 6, "shr": 6, "bitand": 7, "bitxor": 8, "bitor": 9,
         "=": 12, "#": 12, "<": 12, "<=": 12, ">": 12, ">=": 12,
         "and": 13, "xor": 14, "or": 15}


class Gen:
    """A random expression tree, rendered with the parentheses Inox needs."""

    def __init__(self, runtime):
        self.runtime = runtime  # numbers through I(...) / F(...), not folded

    def int_literal(self):
        roll = random.random()
        if roll < 0.6:
            value = random.randint(0, 20)
        elif roll < 0.9:
            value = random.randint(0, 1000)
        else:
            value = random.randint(0, 3_000_000_000)
        text = str(value)
        return (f"I({text})" if self.runtime else text), 1

    def real_literal(self):
        value = random.choice([random.uniform(0.001, 10.0), random.uniform(0.5, 500.0)])
        text = f"{value:.3f}"
        if float(text) == 0.0:
            text = "0.5"
        return (f"F({text})" if self.runtime else text), 1

    @staticmethod
    def wrap(text, level, limit):
        return text if level <= limit else f"({text})"

    def binary(self, op, left, right, right_limit=None):
        level = LEVEL[op]
        left_text, left_level = left
        right_text, right_level = right
        if op == "^":
            # base: postfix only; exponent: a power_expr (level 2) without prefix.
            left_text = self.wrap(left_text, left_level, 1)
            right_text = self.wrap(right_text, right_level, 2)
        else:
            left_text = self.wrap(left_text, left_level, level)
            right_text = self.wrap(right_text, right_level, level - 1 if right_limit is None else right_limit)
        if random.random() < 0.08:  # a redundant pair now and then
            left_text = f"({left_text})"
        return f"{left_text} {op} {right_text}", level

    def int_expr(self, depth):
        if depth <= 0 or random.random() < 0.25:
            return self.int_literal()
        roll = random.random()
        if roll < 0.45:
            op = random.choice(["+", "-", "*", "+", "-", "*", "/", "mod", "rem"])
            return self.binary(op, self.int_expr(depth - 1), self.int_expr(depth - 1))
        if roll < 0.55:
            base = self.int_expr(depth - 1)
            exponent = (str(random.randint(0, 3)), 1)
            if random.random() < 0.3:
                exponent = self.binary("^", (str(random.randint(1, 2)), 1), (str(random.randint(0, 2)), 1))
            return self.binary("^", base, exponent)
        if roll < 0.67:
            operand_text, operand_level = self.int_expr(depth - 1)
            op = random.choice(["-", "-", "+", "bitnot"])
            # The operand of a prefix operator is a unary_expr (level 3); a
            # prefix after a prefix is written with a space.
            text = self.wrap(operand_text, operand_level, 3)
            separator = " " if op == "bitnot" or text.startswith(("-", "+")) else ""
            return f"{op}{separator}{text}", 3
        if roll < 0.75:
            family = random.choice(["shift", "bitand", "bitor", "bitxor"])
            if family == "shift":
                op = random.choice(["shl", "shr"])
                count = (str(random.choice([random.randint(0, 10), random.randint(0, 63)])), 1)
                left = self.int_expr(depth - 1)
                return self.mixed_bitwise(op, left, count)
            return self.mixed_bitwise(family, self.int_expr(depth - 1), self.int_expr(depth - 1))
        if roll < 0.88:
            name = random.choice(["Abs", "Sqr", "Min", "Max", "Sign"])
            count = 2 if name in ("Min", "Max") else 1
            arguments = ", ".join(self.int_expr(depth - 1)[0] for _ in range(count))
            return f"{name}({arguments})", 1
        condition = self.bool_expr(depth - 1)[0]
        return f"IIf({condition}, {self.int_expr(depth - 1)[0]}, {self.int_expr(depth - 1)[0]})", 1

    def mixed_bitwise(self, op, left, right):
        # Two bitwise families, or a bitwise operator and a shift, never mix
        # without parentheses: an operand of another family is wrapped.
        def guard(side):
            text, level = side
            if 6 <= level <= 9 and BITWISE_FAMILY.get(op) != self.family_of(level):
                return f"({text})", 1
            return side
        return self.binary(op, guard(left), guard(right))

    @staticmethod
    def family_of(level):
        return {6: "shift", 7: "bitand", 8: "bitxor", 9: "bitor"}.get(level)

    def real_expr(self, depth):
        if depth <= 0 or random.random() < 0.3:
            if random.random() < 0.7:
                return self.real_literal()
            name = random.choice(list(FLOAT_FUNCS))
            return f"{name}({self.real_expr(depth - 1)[0]})", 1
        roll = random.random()
        if roll < 0.6:
            op = random.choice(["+", "-", "*", "/"])
            return self.binary(op, self.real_expr(depth - 1), self.real_expr(depth - 1))
        if roll < 0.7:
            return self.binary("^", self.real_expr(depth - 1), self.real_literal())
        if roll < 0.82:
            text, level = self.real_expr(depth - 1)
            text = self.wrap(text, level, 3)
            return f"-{' ' if text.startswith(('-', '+')) else ''}{text}", 3
        if roll < 0.9:
            return f"Abs({self.real_expr(depth - 1)[0]})", 1
        condition = self.bool_expr(depth - 1)[0]
        return f"IIf({condition}, {self.real_expr(depth - 1)[0]}, {self.real_expr(depth - 1)[0]})", 1

    def comparison(self, depth):
        op = random.choice(["=", "#", "<", "<=", ">", ">="])
        if random.random() < 0.7:
            return self.binary(op, self.int_expr(depth), self.int_expr(depth), right_limit=11)
        return self.binary(op, self.real_expr(depth), self.real_expr(depth), right_limit=11)

    def bool_expr(self, depth):
        if depth <= 0 or random.random() < 0.4:
            return self.comparison(max(depth, 0))
        roll = random.random()
        if roll < 0.6:
            op = random.choice(["and", "or", "xor"])
            parts = []
            for _ in range(random.randint(2, 3)):
                text, level = self.bool_expr(depth - 1)
                # A different logical operator, or a comparison under a
                # logical one, is grouped when it is weaker than `op`.
                if level >= 13 and level != LEVEL[op]:
                    text, level = f"({text})", 1
                elif level == LEVEL[op] and parts:
                    text, level = f"({text})", 1
                parts.append(text)
            return f" {op} ".join(parts), LEVEL[op]
        if roll < 0.8:
            text, level = self.bool_expr(depth - 1)
            return f"not {self.wrap(text, level, 3)}", 3
        return f"{random.choice(['True', 'False'])}", 1

    def any_expr(self, depth):
        roll = random.random()
        if roll < 0.55:
            return self.int_expr(depth)[0], "int"
        if roll < 0.85:
            return self.real_expr(depth)[0], "real"
        return self.bool_expr(depth)[0], "bool"


# ----------------------------------------------------------------- runner ---

PRELUDE = """Use Std.Math

I(X Integer) Integer :
    Return X
;

F(X Float) Float :
    Return X
;

"""


def program_for(expressions):
    body = "".join(f"    PutLn({text})\n" for text in expressions)
    return PRELUDE + "Main :\n" + body + ";\n"


def run_program(inox, source, stdlib, workdir):
    path = os.path.join(workdir, "calc.inox")
    with open(path, "w", encoding="utf-8") as handle:
        handle.write(source)
    env = dict(os.environ)
    if stdlib:
        env["INOX_STDLIB"] = stdlib
    env["INOX_OUTPUT_DIR"] = workdir
    try:
        result = subprocess.run([inox, "--run", path], capture_output=True, env=env, timeout=120)
    except subprocess.TimeoutExpired:
        return None, "inox timeout"
    if result.returncode != 0:
        return None, f"inox exit {result.returncode}: {result.stderr.decode(errors='replace').strip()[:300]}"
    return result.stdout.decode(errors="replace").splitlines(), None


def agrees(expected, printed, tol):
    kind, value = expected
    if kind == "bool":
        return printed == ("true" if value else "false")
    if kind == "int":
        try:
            return int(printed) == value
        except ValueError:
            return False
    try:
        got = float(printed)
    except ValueError:
        return False
    return abs(got - value) <= tol + tol * abs(value)


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--inox", required=True, help="path to the inox executable")
    parser.add_argument("--stdlib", default=None, help="path to the stdlib directory")
    parser.add_argument("--count", type=int, default=2000, help="number of distinct expressions")
    parser.add_argument("--batch", type=int, default=250, help="expressions per program")
    parser.add_argument("--tol", type=float, default=1e-9,
                        help="relative tolerance for reals (both sides compute in Float64; libm may "
                             "differ in the last digits)")
    parser.add_argument("--seed", type=int, default=None, help="random seed (for reproducibility)")
    parser.add_argument("--max-show", type=int, default=20, help="max failing cases to print")
    parser.add_argument("--traps", type=int, default=60,
                        help="expressions that must stop at run time (one program each)")
    args = parser.parse_args()
    if args.seed is not None:
        random.seed(args.seed)

    cases = []
    seen = set()
    attempts = 0
    while len(cases) < args.count and attempts < args.count * 100:
        attempts += 1
        runtime = (len(cases) // args.batch) % 2 == 1
        text, _ = Gen(runtime).any_expr(random.randint(1, 5))
        if text in seen:
            continue
        try:
            expected = oracle(text)
        except OracleError:
            continue
        except RecursionError:
            continue
        seen.add(text)
        cases.append((text, expected))

    # Expressions the oracle says overflow or divide by zero, without IIf
    # (whose other branch would not run). With every number through
    # I(...)/F(...) each must stop at run time with its CANON-19 category;
    # written with literals, the compiler folds what it can and rejects it
    # ("constant integer overflow", "constant division by zero"), and what it
    # does not fold (a call such as Sqr) stops at run time.
    traps = []
    attempts = 0
    while len(traps) < args.traps and attempts < args.traps * 2000:
        attempts += 1
        text, kind = Gen(len(traps) % 2 == 0).any_expr(random.randint(2, 5))
        if text in seen or "IIf(" in text:
            continue
        try:
            oracle(text)
        except OracleError as error:
            if error.trap is not None:
                seen.add(text)
                traps.append((text, error.trap))
        except RecursionError:
            continue

    workdir = tempfile.mkdtemp(prefix="inox_calc_")
    failures = []
    passed = 0
    for start in range(0, len(cases), args.batch):
        batch = cases[start:start + args.batch]
        lines, error = run_program(args.inox, program_for([text for text, _ in batch]), args.stdlib, workdir)
        if error is not None or len(lines) != len(batch):
            # Find the culprit: each case alone.
            for text, expected in batch:
                single, single_error = run_program(args.inox, program_for([text]), args.stdlib, workdir)
                if single_error is not None or len(single) != 1:
                    failures.append((text, expected, single_error or f"printed {single!r}"))
                elif agrees(expected, single[0], args.tol):
                    passed += 1
                else:
                    failures.append((text, expected, single[0]))
            continue
        for (text, expected), printed in zip(batch, lines):
            if agrees(expected, printed, args.tol):
                passed += 1
            else:
                failures.append((text, expected, printed))

    trap_passed = 0
    for text, category in traps:
        source = program_for([text])
        path = os.path.join(workdir, "trap.inox")
        with open(path, "w", encoding="utf-8") as handle:
            handle.write(source)
        env = dict(os.environ)
        if args.stdlib:
            env["INOX_STDLIB"] = args.stdlib
        env["INOX_OUTPUT_DIR"] = workdir
        result = subprocess.run([args.inox, "--run", path], capture_output=True, env=env, timeout=120)
        stderr = result.stderr.decode(errors="replace")
        # Written with literals, an expression may hold two constant errors
        # (an overflow and a zero divisor); the compiler names the one it
        # meets first, and either is right.
        folded = "I(" not in text and "F(" not in text
        if result.returncode != 0 and (f"Inox runtime error: {category}" in stderr or
                                       f"semantic error: constant {category}" in stderr or
                                       (folded and "semantic error: constant " in stderr)):
            trap_passed += 1
        else:
            failures.append((text, ("trap", category),
                             f"exit {result.returncode}: {stderr.strip()[:200] or result.stdout.decode(errors='replace').strip()}"))

    kinds = {"int": 0, "real": 0, "bool": 0}
    for _, (kind, _) in cases:
        kinds[kind] += 1
    print("=" * 70)
    print("INOX EXPRESSION DIFFERENTIAL TEST")
    print("=" * 70)
    print(f"Generated : {len(cases)} distinct expressions "
          f"({kinds['int']} integer, {kinds['real']} real, {kinds['bool']} Bool)")
    print(f"Traps     : {len(traps)} expressions that must stop at run time")
    print(f"Passed    : {passed + trap_passed}")
    print(f"Failed    : {len(failures)}")
    if failures:
        print(f"\n--- FAILURES (showing up to {args.max_show}) ---")
        for text, expected, got in failures[:args.max_show]:
            print(f"[{expected[0]}] {text}")
            print(f"       oracle = {expected[1]!r}")
            print(f"       inox   = {got}")
    print("=" * 70)
    if failures:
        print("RESULT: MISMATCHES FOUND — investigate the cases above.")
        return 1
    print("RESULT: ALL PASSED — Inox agrees with the oracle on every expression.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
