#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
# Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
"""No frame on a recursion path of the compiler grows (engineering E19).

Compares the `.su` files that `-fstack-usage` writes for two builds of the
compiler, the base (main) and the head, and fails when a function on a
recursion path of the parser, the analyzer, the clone or code generation has a
larger frame
on the head. Other compiler functions whose frame grew, and new functions
with a large frame, are listed for review. Change structured-types (F4),
design.md "Stack budget".

Build each tree with:
  cmake -S <tree> -B <tree>/build/su -G Ninja -DCMAKE_BUILD_TYPE=Debug \\
        -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_CXX_FLAGS=-fstack-usage
  cmake --build <tree>/build/su --target inox

Usage: python3 tools/stack_frames.py <base-build-dir> <head-build-dir>
"""

import pathlib
import re
import subprocess
import sys

# The recursion paths (design.md): a regular expression on the demangled
# name, without its parameter list.
RECURSION_PATHS = [
    r"inox::compiler::parser::Parser::parse(Statement|TypedLocalStatement|ExpressionStatement|IfStatement|"
    r"ForInStatement|CaseStatement|TryStatement|Postfix|Primary|Unary|Expression|Value|BlockBody|"
    r"DelimitedBody|TypeInto|TypeArgumentsInto|ArrayTypeInto|Binary\w*|Or|And|Comparison|Additive|"
    r"Multiplicative|Power)",
    r"inox::compiler::semantic::SemanticAnalyzer::analyze(Statement|Expression|BinaryExpression|"
    r"CallExpression|GenericCall|Construction|GenericConstruction|UnaryExpression|MemberExpression|"
    r"IndexExpression|MethodCallExpression|UserFunctionCall|ExpressionFor)",
    r"inox::compiler::semantic::SemanticAnalyzer::(resolveWrittenType|resolveTypeExpr|resolveArrayType|"
    r"resolvePointerType|resolveGenericStructType|resolveSetType|instantiateRoutine|instantiateStruct|"
    r"unifyTypeExpr|cloneInstanceBody|substituteTypeParameters|writtenFormOf)",
    r"inox::compiler::ast::\(anonymous namespace\)::clone(Statement|Expression)Node",
    r"inox::compiler::ast::clone(Statement|Statements|Expression)",
    # Code generation (F6, typed-ir, step 1): every FunctionEmitter function
    # that can reach itself; F6 changes codegen step by step, and none of
    # these may grow.
    r"inox::compiler::codegen::detail::FunctionEmitter::(constructedStruct|defaultedStruct|dropFunctionFor|"
    r"emitArguments|emitArrayBaseAddress|emitArrayElementDefaults|emitAssignmentBranch|emitCase|"
    r"emitConcatenation|emitConditional|emitConstruction|emitDecimalConversion|emitElementAddress|"
    r"emitExpression|emitExpressionValue|emitFileCall|emitFill|emitForIn|emitHeaderOfPlace|"
    r"emitHeaderOfPointer|emitIfMerge|emitInt64Argument|emitIntrinsicCall|emitInvariantChecks|"
    r"emitInvariantOf|emitLinkOf|emitLoopStatements|emitMathBuiltinCall|emitMemberAddress|emitMembership|"
    r"emitMethodCall|emitNoArgumentMethodCall|emitOrdinalBuiltin|emitOwnerFreesAt|emitPlace|"
    r"emitPointerBuiltin|emitPointerTarget|emitRealConversion|emitReceiver|emitReferenceArgument|"
    r"emitReleaseAt|emitRepeat|emitRounding|emitSetCall|emitSetComparison|emitSlice|emitSliceSource|"
    r"emitStatement|emitStatementKind|emitStringCall|emitStringParts|emitStructFieldDefaults|"
    r"emitTextToNumber|emitTry|emitUnless|emitWhile|emitWith|forEachLeafAt|forEachStringAt|isMoveOnly|"
    r"lastPointerOnPath|placeMayChange|resolveMethodCall|resolveNoArgumentMethodAccess|samePlace|"
    r"structHasDefaults|typeOf|typeOwnsReferences)",
]
NEW_FRAME_TO_REVIEW = 256
# A recursion a change adds (phase 2b of F5 walks the statements between
# the parser and the analyzer): its functions are new, so they cannot grow;
# each must stay within NEW_PATH_BUDGET bytes, and they are listed apart.
NEW_PATHS = [
    r"inox::compiler::lowering::\(anonymous namespace\)::(Desugarer::(lower|lowerLoop|rebuildLoop|lowerWith|"
    r"lowerChain|lowerCompound|appendBody|number|body)|hasWrittenForms|requireNoForms|expressionHasWithTarget)",
    # Phase 5b (F6, typed-ir, step 1): the lowering to the typed IR, its
    # verifier and its printer.
    r"inox::compiler::tir::detail::Lowerer::(expression|identifier|call|construction|checked|"
    r"expressions|place|variable|through|invariants|statement|statements|expressionStatement|declaration|"
    r"ifStatement|caseStatement|tryStatement|withStatement|forStatement|raiseStatement|slice|check|builtin)",
    r"inox::compiler::tir::\(anonymous namespace\)::Verifier::(statements|statement|expression|place|invariants)",
    r"inox::compiler::tir::\(anonymous namespace\)::Printer::(statements|statement|expression|place|invariants|operation|"
    r"inlineInvariants)",
]
NEW_PATH_BUDGET = 512
# A function renamed or split by F4 is compared with the one it replaces.
# parseTypeName became parseTypeInto, which recurses through
# parseArrayTypeInto and parseTypeArgumentsInto: each, and each level of
# that recursion, must stay within the old frame.
REPLACES = {
    "inox::compiler::parser::Parser::parseTypeInto": "inox::compiler::parser::Parser::parseTypeName[abi:cxx11]",
    "inox::compiler::parser::Parser::parseArrayTypeInto": "inox::compiler::parser::Parser::parseTypeName[abi:cxx11]",
    "inox::compiler::parser::Parser::parseTypeArgumentsInto":
        "inox::compiler::parser::Parser::parseTypeName[abi:cxx11]",
    "inox::compiler::semantic::SemanticAnalyzer::resolveTypeExpr":
        "inox::compiler::semantic::SemanticAnalyzer::resolveTypeSyntax",
    "inox::compiler::semantic::SemanticAnalyzer::unifyTypeExpr":
        "inox::compiler::semantic::SemanticAnalyzer::unifyTypeSyntax",
    # Substitution on the tree replaces the text substitution, which read
    # the replacement back (readTypeSyntax recursed on nested types).
    "inox::compiler::semantic::SemanticAnalyzer::substituteTypeParameters":
        "inox::compiler::types::substituteTypeNames",
    "inox::compiler::semantic::SemanticAnalyzer::writtenFormOf": "inox::compiler::types::readTypeSyntax",
    # The clone of an instance's body moved out of instantiateRoutine.
    "inox::compiler::semantic::SemanticAnalyzer::cloneInstanceBody":
        "inox::compiler::semantic::SemanticAnalyzer::instantiateRoutine",
}


def without_parameters(name: str) -> str:
    """The name without its parameter list (and a trailing const): the last
    balanced parenthesized group, so `(anonymous namespace)` stays."""
    text = re.sub(r"\s+const$", "", name.strip())
    if not text.endswith(")"):
        return text
    depth = 0
    for index in range(len(text) - 1, -1, -1):
        if text[index] == ")":
            depth += 1
        elif text[index] == "(":
            depth -= 1
            if depth == 0:
                return text[:index]
    return text


def frames(build: pathlib.Path) -> dict[str, int]:
    """Demangled function name (without parameters) -> largest frame."""
    mangled: list[tuple[str, int]] = []
    for path in build.rglob("*.su"):
        if "compiler" not in path.as_posix():
            continue
        for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
            parts = line.split("\t")
            if len(parts) < 2:
                continue
            name = parts[0].rsplit(":", 1)[-1]
            try:
                mangled.append((name, int(parts[1])))
            except ValueError:
                continue
    demangled = subprocess.run(["c++filt"], input="\n".join(name for name, _ in mangled),
                               capture_output=True, text=True, check=True).stdout.splitlines()
    result: dict[str, int] = {}
    for (_, size), name in zip(mangled, demangled):
        key = without_parameters(name)
        result[key] = max(size, result.get(key, 0))
    return result


def main(argv: list[str]) -> int:
    if len(argv) != 3:
        print(__doc__)
        return 2
    base, head = frames(pathlib.Path(argv[1])), frames(pathlib.Path(argv[2]))
    patterns = [re.compile(pattern + r"$") for pattern in RECURSION_PATHS]
    on_path = sorted(name for name in set(base) | set(head) if any(p.match(name) for p in patterns))
    grown = 0
    print("%-90s %8s %8s" % ("recursion path", "main", "head"))
    for name in on_path:
        before, after = base.get(name), head.get(name)
        if before is None and name in REPLACES:
            before = base.get(REPLACES[name])
        mark = ""
        if after is not None and (before is None or after > before):
            mark = "  GROWS" if before is not None else "  NEW"
            grown += 1
        print("%-90s %8s %8s%s" % (name[:90], before if before is not None else "-",
                                   after if after is not None else "-", mark))
    others = [name for name in sorted(head) if name.startswith("inox::compiler") and name not in on_path and
              ((name in base and head[name] > base[name]) or (name not in base and head[name] >= NEW_FRAME_TO_REVIEW))]
    if others:
        print("\nfor review (off the recursion paths): grown, or new with a frame of %d bytes or more" %
              NEW_FRAME_TO_REVIEW)
        for name in others:
            print("%-90s %8s %8s" % (name[:90], base.get(name, "-"), head[name]))
    level = head.get("inox::compiler::parser::Parser::parseTypeInto", 0) + \
        head.get("inox::compiler::parser::Parser::parseArrayTypeInto", 0)
    old = base.get(REPLACES["inox::compiler::parser::Parser::parseTypeInto"])
    if old is not None and level:
        print("\none level of the type recursion: main %d, head %d (parseTypeInto + parseArrayTypeInto)%s" %
              (old, level, "  GROWS" if level > old else ""))
        grown += 1 if level > old else 0
    new_patterns = [re.compile(pattern + r"$") for pattern in NEW_PATHS]
    new_path = sorted(name for name in head if any(p.match(name) for p in new_patterns))
    if new_path:
        print("\nnew recursion paths (budget %d bytes a frame)" % NEW_PATH_BUDGET)
        for name in new_path:
            over = head[name] > NEW_PATH_BUDGET
            grown += 1 if over else 0
            print("%-90s %8s %8s%s" % (name[:90], base.get(name, "-"), head[name], "  OVER BUDGET" if over else ""))
    print("\nstack_frames: %d function(s) on the recursion paths, %d grown or new" % (len(on_path), grown))
    return 1 if grown else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
