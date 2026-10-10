#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
# Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.
"""Checks that grammar/grammar.ebnf agrees with the canon, the lexer and the parser.

The grammar is a mirror of docs/INOX_CANONICAL.md (AGENTS.md hierarchy, level 4).
This tool does not prove the parser accepts exactly the grammar; it catches the
drift that has happened before:

  ebnf        every referenced production is defined, none is defined twice,
              every production is reachable from `module`
  keywords    every word terminal is a lexer keyword (or a listed contextual
              word), every lexer keyword appears in the grammar (or is a listed
              reserved-only word), and the kKeywords array size is right
  forbidden   terminals Inox does not have (`end`, `break`, `finally`, `then`,
              `of`, `when`, `=>`, `var`) and empty parentheses never appear
  canon20     the `(* level N, assoc *)` annotations follow the CANON-20 table
              of the canonical document: same levels, same associativity,
              weakest first
  parser      each parser function in PARSER_MAP exists in Parser.cpp and its
              production exists in the grammar; the expression functions call
              each other in the CANON-20 order

Usage: python3 tools/grammar_consistency.py [repo-root]
Exit status 0 when every check passes, 1 otherwise.
"""

import re
import sys
from pathlib import Path

# Words used as terminals that are not reserved by the lexer: they are
# identifiers with a meaning in one position only.
CONTEXTUAL_WORDS = {"self", "range", "array"}

# Reserved by the lexer but not part of any production (reserved for the
# future or kept reserved to reject old syntax with a diagnostic).
RESERVED_ONLY = {"main", "do", "var"}

FORBIDDEN_TERMINALS = {"end", "break", "finally", "then", "of", "when", "=>", "var", "elsif"}

# Lexical productions that are not reached from `module` through other rules.
LEXICAL_ROOTS = {"comment"}

# Parser function -> grammar production it implements.
PARSER_MAP = {
    "parseModule": "module",
    "parseUseDeclaration": "use_decl",
    "parseFunctionDeclaration": "routine_decl",
    "parseTypedLocalStatement": "local_decl",
    "parseExpressionStatement": "assignment_stmt",
    "parseArgument": "argument",
    "parseArgumentList": "argument_list",
    "parseIfStatement": "if_stmt",
    "parseUnlessStatement": "unless_stmt",
    "parseWithStatement": "with_stmt",
    "parseWhileStatement": "while_stmt",
    "parseRepeatStatement": "repeat_stmt",
    "parseUntilStatement": "until_stmt",
    "parseForInStatement": "for_stmt",
    "parseCaseStatement": "case_stmt",
    "parseCaseChoice": "choice",
    "parseTryStatement": "try_stmt",
    "parseExceptionHandler": "on_handler",
    "parseRaiseStatement": "raise_stmt",
    "parseRetryStatement": "retry_stmt",
    "parseReturnStatement": "return_stmt",
    "parseValue": "expression",
    "parseOr": "or_expr",
    "parseXor": "xor_expr",
    "parseAnd": "and_expr",
    "parseRelational": "relational_expr",
    "parseMembership": "membership_expr",
    "parseRange": "range_expr",
    "parseBitOr": "bitor_expr",
    "parseBitXor": "bitxor_expr",
    "parseBitAnd": "bitand_expr",
    "parseShift": "shift_expr",
    "parseAdditive": "additive_expr",
    "parseMultiplicative": "multiplicative_expr",
    "parseUnary": "unary_expr",
    "parsePower": "power_expr",
    "parsePostfix": "postfix_expr",
    "parsePrimary": "primary",
}

# Expression functions, weakest binding first; each must call the next.
EXPRESSION_CHAIN = [
    "parseOr", "parseXor", "parseAnd", "parseRelational", "parseMembership",
    "parseRange", "parseBitOr", "parseBitXor", "parseBitAnd", "parseShift",
    "parseAdditive", "parseMultiplicative", "parseUnary", "parsePower",
    "parsePostfix", "parsePrimary",
]

ASSOC_FROM_CANON = {
    "left": "left",
    "right": "right",
    "non-associative": "none",
    "(prefix, unary)": "prefix",
}


class Report:
    def __init__(self):
        self.failures = []

    def check(self, name, problems):
        if problems:
            print(f"[FAIL] {name}")
            for problem in problems:
                print(f"       {problem}")
            self.failures.append(name)
        else:
            print(f"[PASS] {name}")


def ebnf_block(text):
    match = re.search(r"```ebnf\n(.*?)```", text, re.S)
    if not match:
        raise SystemExit("grammar_consistency: no ```ebnf block in grammar.ebnf")
    return match.group(1)


def strip_comments(block):
    return re.sub(r"\(\*.*?\*\)", " ", block, flags=re.S)


def productions(block):
    """Returns {name: [rhs text, ...]} and the list of names in order."""
    body = strip_comments(block)
    rules = {}
    order = []
    current = None
    for line in body.splitlines():
        head = re.match(r"^([A-Za-z_][A-Za-z0-9_]*)\s*::=(.*)$", line)
        if head:
            current = head.group(1)
            order.append(current)
            rules.setdefault(current, []).append(head.group(2))
        elif current and line.strip():
            rules[current].append(line)
    return rules, order


def rhs_symbols(rhs):
    terminals = re.findall(r'"([^"]*)"|\'([^\']*)\'', rhs)
    terminals = [a or b for a, b in terminals]
    without = re.sub(r'"[^"]*"|\'[^\']*\'', " ", rhs)
    names = re.findall(r"[A-Za-z_][A-Za-z0-9_]*", without)
    return terminals, names


def check_ebnf(rules, order):
    problems = []
    for name in sorted({n for n in order if order.count(n) > 1}):
        problems.append(f"production defined more than once: {name}")
    referenced = {}
    for name, parts in rules.items():
        _, names = rhs_symbols(" ".join(parts))
        referenced[name] = set(names)
    for name, refs in referenced.items():
        for ref in sorted(refs - set(rules)):
            problems.append(f"{name} references undefined production: {ref}")
    reachable = set()
    pending = ["module", *LEXICAL_ROOTS]
    while pending:
        name = pending.pop()
        if name in reachable or name not in rules:
            continue
        reachable.add(name)
        pending.extend(referenced.get(name, ()))
    for name in sorted(set(rules) - reachable):
        problems.append(f"production not reachable from module: {name}")
    return problems


def lexer_keywords(lexer_text):
    match = re.search(
        r"std::array<std::string_view,\s*(\d+)>\s*kKeywords\s*=\s*\{(.*?)\};",
        lexer_text, re.S)
    if not match:
        raise SystemExit("grammar_consistency: kKeywords not found in Lexer.cpp")
    declared = int(match.group(1))
    words = re.findall(r'"([^"]+)"', match.group(2))
    return declared, words


def check_keywords(rules, lexer_text):
    problems = []
    declared, words = lexer_keywords(lexer_text)
    if declared != len(words):
        problems.append(f"kKeywords declares {declared} entries but lists {len(words)}")
    keywords = {w.lower() for w in words}
    terminals = set()
    for parts in rules.values():
        found, _ = rhs_symbols(" ".join(parts))
        terminals.update(found)
    # Single letters are hex digits ("A".."F"), not words.
    word_terminals = {t.lower() for t in terminals if re.fullmatch(r"[A-Za-z]{2,}", t)}
    for word in sorted(word_terminals - keywords - CONTEXTUAL_WORDS):
        problems.append(f"grammar word terminal is not a lexer keyword: {word}")
    for word in sorted(keywords - word_terminals - RESERVED_ONLY):
        problems.append(f"lexer keyword missing from the grammar: {word}")
    for word in sorted(RESERVED_ONLY & word_terminals):
        problems.append(f"reserved-only word used as a terminal: {word}")
    return problems


def check_forbidden(rules, block):
    problems = []
    terminals = set()
    for parts in rules.values():
        found, _ = rhs_symbols(" ".join(parts))
        terminals.update(t.lower() for t in found)
    for word in sorted(terminals & FORBIDDEN_TERMINALS):
        problems.append(f"forbidden terminal in the grammar: {word}")
    if re.search(r'"\("\s*"\)"', strip_comments(block)):
        problems.append('empty parentheses "(" ")" appear in a production (CANON-7)')
    return problems


def canon_table(canon_text):
    section = canon_text.split("### Precedence table", 1)
    if len(section) < 2:
        raise SystemExit("grammar_consistency: CANON-20 precedence table not found")
    table = section[1].split("```", 2)[1]
    lines = table.splitlines()
    header = next((line for line in lines if "Associativity" in line), None)
    if header is None:
        raise SystemExit("grammar_consistency: CANON-20 table has no Associativity column")
    column = header.index("Associativity")
    levels = {}
    for line in lines:
        row = re.match(r"^\s*(\d+)\s", line)
        if not row:
            continue
        assoc = line[column:].strip().lower()
        levels[int(row.group(1))] = ASSOC_FROM_CANON.get(assoc, assoc)
    return levels


def check_canon20(block, canon_text):
    problems = []
    canon = canon_table(canon_text)
    if not canon:
        return ["CANON-20 table has no rows"]
    annotated = [(int(n), a.lower()) for n, a in
                 re.findall(r"\(\*\s*level\s+(\d+),\s*(\w+)\s*\*\)", block)]
    levels = [n for n, _ in annotated]
    if levels != sorted(levels, reverse=True):
        problems.append(f"grammar levels are not weakest-first: {levels}")
    top = max(canon)
    if not re.search(rf'":="\s*\(level\s+{top}\)', block):
        problems.append(f'the grammar does not place ":=" at level {top} (the weakest)')
    expected = {n: a for n, a in canon.items() if n != top}
    found = dict(annotated)
    for level in sorted(set(expected) | set(found)):
        if level not in found:
            problems.append(f"CANON-20 level {level} has no grammar production")
        elif level not in expected:
            problems.append(f"grammar level {level} is not in CANON-20")
        elif found[level] != expected[level]:
            problems.append(f"level {level}: grammar says {found[level]}, "
                            f"CANON-20 says {expected[level]}")
    return problems


def function_bodies(parser_text):
    bodies = {}
    pattern = re.compile(r"^[A-Za-z_:<>\s\*&]*?Parser::(\w+)\([^;{]*\)\s*(?:const\s*)?\{", re.M)
    matches = list(pattern.finditer(parser_text))
    for index, match in enumerate(matches):
        end = matches[index + 1].start() if index + 1 < len(matches) else len(parser_text)
        bodies.setdefault(match.group(1), parser_text[match.end():end])
    return bodies


def check_parser(rules, parser_text):
    problems = []
    bodies = function_bodies(parser_text)
    for function, production in PARSER_MAP.items():
        if function not in bodies:
            problems.append(f"parser function not found: {function}")
        if production not in rules:
            problems.append(f"{function} maps to undefined production: {production}")
    for upper, lower in zip(EXPRESSION_CHAIN, EXPRESSION_CHAIN[1:]):
        body = bodies.get(upper, "")
        if not re.search(rf"\b{lower}\s*\(", body):
            problems.append(f"{upper} does not call {lower} (CANON-20 order)")
    return problems


def main(argv):
    root = Path(argv[1]) if len(argv) > 1 else Path(__file__).resolve().parent.parent
    grammar = (root / "grammar" / "grammar.ebnf").read_text(encoding="utf-8")
    canon = (root / "docs" / "INOX_CANONICAL.md").read_text(encoding="utf-8")
    lexer = (root / "src" / "compiler" / "lexer" / "Lexer.cpp").read_text(encoding="utf-8")
    parser = (root / "src" / "compiler" / "parser" / "Parser.cpp").read_text(encoding="utf-8")

    block = ebnf_block(grammar)
    rules, order = productions(block)
    report = Report()
    report.check("grammar ebnf integrity", check_ebnf(rules, order))
    report.check("grammar keywords vs lexer", check_keywords(rules, lexer))
    report.check("grammar forbidden terminals", check_forbidden(rules, block))
    report.check("grammar CANON-20 levels", check_canon20(block, canon))
    report.check("grammar parser map", check_parser(rules, parser))
    print(f"\nSummary: {5 - len(report.failures)} passed, {len(report.failures)} failed")
    return 1 if report.failures else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
