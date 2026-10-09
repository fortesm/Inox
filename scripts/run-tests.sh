#!/usr/bin/env bash
# SPDX-License-Identifier: MPL-2.0
# Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

set -uo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
inox_exe="${1:-}"

if [[ -z "$inox_exe" ]]; then
    if [[ -f "$repo_root/build/linux-clang-debug/inox" && -x "$repo_root/build/linux-clang-debug/inox" ]]; then
        inox_exe="$repo_root/build/linux-clang-debug/inox"
    elif [[ -f "$repo_root/build/inox" && -x "$repo_root/build/inox" ]]; then
        inox_exe="$repo_root/build/inox"
    elif [[ -x "$repo_root/build-linux/inox" ]]; then
        inox_exe="$repo_root/build-linux/inox"
    elif [[ -x "$repo_root/build/windows-clang-msvc/Debug/inox.exe" ]]; then
        inox_exe="$repo_root/build/windows-clang-msvc/Debug/inox.exe"
    elif [[ -x "$repo_root/build/Debug/inox.exe" ]]; then
        inox_exe="$repo_root/build/Debug/inox.exe"
    else
        inox_exe="$repo_root/build/linux-clang-debug/inox"
    fi
elif [[ "$inox_exe" != /* ]]; then
    inox_exe="$repo_root/$inox_exe"
fi

if [[ ! -f "$inox_exe" || ! -x "$inox_exe" ]]; then
    echo "Inox executable not found: $inox_exe"
    echo "Run: cmake --build build"
    exit 1
fi

passed=0
failed=0

relative_path() {
    local path="$1"
    if [[ "$path" == "$repo_root/"* ]]; then
        echo "${path#"$repo_root/"}"
    else
        echo "$path"
    fi
}

record_pass() {
    local label="$1"
    passed=$((passed + 1))
    echo "[PASS] $label"
}

record_fail() {
    local label="$1"
    shift
    failed=$((failed + 1))
    echo "[FAIL] $label"
    for line in "$@"; do
        echo "       $line"
    done
}

run_inox_test() {
    local test_file="$1"
    local expect_success="$2"
    local rel
    rel="$(relative_path "$test_file")"

    "$inox_exe" "$test_file" >/dev/null 2>&1
    local exit_code=$?
    local ok=0
    local expectation="failure"

    if [[ "$expect_success" == "true" ]]; then
        expectation="success"
        [[ $exit_code -eq 0 ]] && ok=1
    else
        [[ $exit_code -ne 0 ]] && ok=1
    fi

    if [[ $ok -eq 1 ]]; then
        record_pass "$rel"
    else
        record_fail "$rel" "expected $expectation, exit code $exit_code"
    fi
}

contains_fragment() {
    local output="$1"
    local fragment="$2"
    [[ "$output" == *"$fragment"* ]]
}

run_llvm_emission_test() {
    local test_file="$1"
    shift
    local fragments=("$@")
    local rel
    rel="$(relative_path "$test_file")"

    local output
    output="$($inox_exe --emit-llvm "$test_file" 2>&1)"
    local exit_code=$?
    local missing=()

    for fragment in "${fragments[@]}"; do
        if ! contains_fragment "$output" "$fragment"; then
            missing+=("$fragment")
        fi
    done

    if [[ $exit_code -eq 0 && ${#missing[@]} -eq 0 ]]; then
        record_pass "$rel --emit-llvm"
    else
        local details=("expected exit code 0 and all required LLVM fragments" "actual exit code: $exit_code")
        if [[ ${#missing[@]} -ne 0 ]]; then
            local joined
            printf -v joined '%s, ' "${missing[@]}"
            joined="${joined%, }"
            details+=("missing: $joined")
        fi
        record_fail "$rel --emit-llvm" "${details[@]}"
    fi
}


run_mode_exit_test() {
    local mode="$1"
    local test_file="$2"
    local expect_success="$3"
    local rel
    rel="$(relative_path "$test_file")"

    "$inox_exe" "$mode" "$test_file" >/dev/null 2>&1
    local exit_code=$?
    local ok=0
    local expectation="failure"

    if [[ "$expect_success" == "true" ]]; then
        expectation="success"
        [[ $exit_code -eq 0 ]] && ok=1
    else
        [[ $exit_code -ne 0 ]] && ok=1
    fi

    if [[ $ok -eq 1 ]]; then
        record_pass "$rel $mode"
    else
        record_fail "$rel $mode" "expected $expectation, exit code $exit_code"
    fi
}

run_mode_fragment_test() {
    local mode="$1"
    local test_file="$2"
    shift 2
    local fragments=("$@")
    local rel
    rel="$(relative_path "$test_file")"

    local output
    output="$($inox_exe "$mode" "$test_file" 2>&1)"
    local exit_code=$?
    local missing=()

    for fragment in "${fragments[@]}"; do
        if ! contains_fragment "$output" "$fragment"; then
            missing+=("$fragment")
        fi
    done

    if [[ $exit_code -eq 0 && ${#missing[@]} -eq 0 ]]; then
        record_pass "$rel $mode"
    else
        local details=("expected exit code 0 and all required fragments" "actual exit code: $exit_code")
        if [[ ${#missing[@]} -ne 0 ]]; then
            local joined
            printf -v joined '%s, ' "${missing[@]}"
            joined="${joined%, }"
            details+=("missing: $joined")
        fi
        record_fail "$rel $mode" "${details[@]}"
    fi
}

run_linked_execution_test() {
    local test_file="$1"
    local expected_file="$2"
    local rel
    rel="$(relative_path "$test_file")"

    if ! command -v clang >/dev/null 2>&1; then
        echo "[SKIP] $rel link/run (clang not found)"
        return 0
    fi

    local temp_dir
    temp_dir="$(mktemp -d)"
    local ll_path="$temp_dir/program.ll"
    local exe_path="$temp_dir/program"

    "$inox_exe" --emit-llvm "$test_file" > "$ll_path"
    local emit_exit=$?
    if [[ $emit_exit -ne 0 ]]; then
        rm -rf "$temp_dir"
        record_fail "$rel link/run" "LLVM emission failed with exit code $emit_exit"
        return 0
    fi

    local clang_args=("$ll_path" -o "$exe_path")
    if [[ "$(uname -s)" != MINGW* && "$(uname -s)" != MSYS* && "$(uname -s)" != CYGWIN* ]]; then
        clang_args+=("-lm")
    fi
    clang "${clang_args[@]}" >/dev/null 2>&1
    local clang_exit=$?
    if [[ $clang_exit -ne 0 ]]; then
        rm -rf "$temp_dir"
        record_fail "$rel link/run" "clang link failed with exit code $clang_exit"
        return 0
    fi

    local actual expected
    actual="$($exe_path | sed 's/\r$//')"
    expected="$(sed 's/\r$//' "$expected_file")"

    if [[ "$actual" == "$expected" ]]; then
        record_pass "$rel link/run"
    else
        record_fail "$rel link/run" "expected output: $expected" "actual output: $actual"
    fi

    rm -rf "$temp_dir"
}

run_build_driver_test() {
    local test_file="$1"
    local rel
    rel="$(relative_path "$test_file")"

    if ! command -v clang >/dev/null 2>&1; then
        echo "[SKIP] $rel --build (clang not found)"
        return 0
    fi

    "$inox_exe" --build "$test_file" >/dev/null 2>&1
    local exit_code=$?
    if [[ $exit_code -eq 0 ]]; then
        record_pass "$rel --build"
    else
        record_fail "$rel --build" "exit code: $exit_code"
    fi
}

run_driver_execution_test() {
    local test_file="$1"
    local expected_file="$2"
    local rel
    rel="$(relative_path "$test_file")"

    if ! command -v clang >/dev/null 2>&1; then
        echo "[SKIP] $rel --run (clang not found)"
        return 0
    fi

    local actual expected exit_code
    actual="$("$inox_exe" --run "$test_file" 2>&1)"
    exit_code=$?
    expected="$(sed 's/\r$//' "$expected_file")"
    actual="$(printf '%s' "$actual" | sed 's/\r$//')"

    if [[ $exit_code -eq 0 && "$actual" == "$expected" ]]; then
        record_pass "$rel --run"
    else
        record_fail "$rel --run" "exit code: $exit_code" "expected output: $expected" "actual output: $actual"
    fi
}


run_driver_input_test() {
    local test_file="$1"
    local input_file="$2"
    local expected_file="$3"
    local rel
    rel="$(relative_path "$test_file")"

    if ! command -v clang >/dev/null 2>&1; then
        echo "[SKIP] $rel --run < input (clang not found)"
        return 0
    fi

    local actual expected exit_code
    actual="$("$inox_exe" --run "$test_file" < "$input_file" 2>&1)"
    exit_code=$?
    expected="$(sed 's/\r$//' "$expected_file")"
    actual="$(printf '%s' "$actual" | sed 's/\r$//')"

    if [[ $exit_code -eq 0 && "$actual" == "$expected" ]]; then
        record_pass "$rel --run < input"
    else
        record_fail "$rel --run < input" "exit code: $exit_code" "expected output: $expected" "actual output: $actual"
    fi
}

run_driver_trap_test() {
    local test_file="$1"
    local input_file="${2:-}"
    local trap_file="${test_file%.inox}.trap"
    local rel
    rel="$(relative_path "$test_file")"
    # NAME.trap holds the diagnostic the program must print, for example
    # "Inox runtime error: division by zero" (CANON-19).
    local expected_message
    expected_message="$(head -n 1 "$trap_file" | sed 's/\r$//')"

    if ! command -v clang >/dev/null 2>&1; then
        echo "[SKIP] $rel --run (expect trap; clang not found)"
        return 0
    fi

    local actual exit_code
    if [[ -n "$input_file" ]]; then
        actual="$("$inox_exe" --run "$test_file" < "$input_file" 2>&1)"
    else
        actual="$("$inox_exe" --run "$test_file" 2>&1 < /dev/null)"
    fi
    exit_code=$?
    actual="$(printf '%s' "$actual" | sed 's/\r$//')"

    # A compile error also exits non-zero, so require the runtime diagnostic itself.
    # Optional NAME.out next to NAME.trap: the complete output (program output,
    # runtime diagnostic, driver note) must match exactly, which proves that no
    # handler or finally block ran.
    local exact_file="${test_file%.inox}.out"
    local exact_ok=1
    if [[ -f "$exact_file" && "$actual" != "$(sed 's/\r$//' "$exact_file")" ]]; then
        exact_ok=0
    fi
    if [[ $exit_code -ne 0 && -n "$expected_message" && "$actual" == *"$expected_message"* && $exact_ok -eq 1 ]]; then
        record_pass "$rel --run (trap)"
    else
        record_fail "$rel --run (trap)" "expected the program to compile, run and stop with: $expected_message" "exit code: $exit_code" "actual output: $actual"
    fi
}

# Runs every NAME.inox in a directory according to its sidecar files:
#   NAME.out  exit 0 and exactly this output     NAME.trap  must trap at run time
#   NAME.in   optional standard input
run_runtime_tree() {
    local root="$1"
    [[ -d "$root" ]] || return 0
    while IFS= read -r -d '' test_file; do
        local base="${test_file%.inox}"
        local input_file=""
        [[ -f "$base.in" ]] && input_file="$base.in"
        if [[ -f "$base.trap" ]]; then
            run_driver_trap_test "$test_file" "$input_file"
        elif [[ -f "$base.out" ]]; then
            if [[ -n "$input_file" ]]; then
                run_driver_input_test "$test_file" "$input_file" "$base.out"
            else
                run_driver_execution_test "$test_file" "$base.out"
            fi
        fi
    done < <(find "$root" -maxdepth 1 -type f -name '*.inox' -print0 | sort -z)
}

# Each NAME.inox must be rejected, and the error output must contain NAME.err.
run_diagnostic_tree() {
    local root="$1"
    [[ -d "$root" ]] || return 0
    while IFS= read -r -d '' test_file; do
        local rel expected_file expected actual exit_code
        rel="$(relative_path "$test_file")"
        expected_file="${test_file%.inox}.err"
        if [[ ! -f "$expected_file" ]]; then
            record_fail "$rel diagnostic" "missing expectation file: $expected_file"
            continue
        fi
        expected="$(sed 's/\r$//' "$expected_file")"
        actual="$("$inox_exe" --emit-llvm "$test_file" 2>&1 >/dev/null)"
        exit_code=$?
        actual="$(printf '%s' "$actual" | sed 's/\r$//')"
        if [[ $exit_code -ne 0 && "$actual" == *"$expected"* ]]; then
            record_pass "$rel diagnostic"
        else
            record_fail "$rel diagnostic" "exit code: $exit_code" "expected error containing: $expected" "actual error: $actual"
        fi
    done < <(find "$root" -maxdepth 1 -type f -name '*.inox' -print0 | sort -z)
}

run_test_tree() {
    local root="$1"
    local maxdepth="$2"
    local expect_success="$3"

    [[ -d "$root" ]] || return 0
    while IFS= read -r -d '' test_file; do
        run_inox_test "$test_file" "$expect_success"
    done < <(find "$root" -maxdepth "$maxdepth" -type f -name '*.inox' -print0 | sort -z)
}

run_test_tree "$repo_root/examples" 1 true
run_test_tree "$repo_root/tests/parser/valid" 10 true
run_test_tree "$repo_root/tests/semantic/valid" 10 true

run_test_tree "$repo_root/tests/invalid" 1 false
run_test_tree "$repo_root/tests/lexer/invalid" 10 false
run_test_tree "$repo_root/tests/parser/invalid" 10 false
run_test_tree "$repo_root/tests/semantic/invalid" 10 false

run_mode_fragment_test --dump-tokens "$repo_root/tests/lexer/valid/tokens-keywords-literals.inox" \
    'Keyword lexeme="Module" normalized="module"' 'Keyword lexeme="Type" normalized="type"' 'Keyword lexeme="Struct" normalized="struct"' 'Keyword lexeme="Retry" normalized="retry"' 'IntegerLiteral lexeme="$2A"' 'StringLiteral lexeme="hello"' 'CharLiteral lexeme=' 'Identifier lexeme="End" normalized="end"'
run_mode_exit_test --parse-only "$repo_root/tests/parser/valid/canonical-type-and-var.inox" true
run_mode_exit_test --parse-only "$repo_root/tests/parser/invalid/var-colon.inox" false

run_llvm_emission_test "$repo_root/examples/empty.inox" \
    "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/llvm-integer-function.inox" \
    "define i64 @inox_sum" "%tmp0 = call i64 @__inox_add_i64(i64 %a, i64 %b)" "ret i64 %tmp0" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/llvm-function-call.inox" \
    "define i64 @inox_sum" "define i64 @inox_double" "%tmp0 = call i64 @inox_sum(i64 %x, i64 %x)" "ret i64 %tmp0" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/llvm-local-variables.inox" \
    "define i64 @inox_compute" "%a = alloca i64" "%b = alloca i64" "store i64 10, ptr %a" "store i64 20, ptr %b" "load i64, ptr %a" "load i64, ptr %b" "call i64 @__inox_add_i64" "ret i64" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/llvm-inline-typed-local.inox" \
    "define i64 @inox_compute" "%a = alloca i64" "%b = alloca i64" "store i64 10, ptr %a" "store i64 20, ptr %b" "load i64, ptr %a" "load i64, ptr %b" "call i64 @__inox_add_i64" "ret i64" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/llvm-local-assignment.inox" \
    "define i64 @inox_compute" "%a = alloca i64" "%b = alloca i64" "store i64 10, ptr %a" "store i64 20, ptr %b" "call i64 @__inox_add_i64" "call i64 @__inox_mul_i64" "store i64 %tmp0, ptr %a" "store i64 %tmp3, ptr %b" "ret i64" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/llvm-integer-operators.inox" \
    "define i64 @inox_compute" "%tmp0 = call i64 @__inox_div_i64(i64 %a, i64 %b)" "call i64 @__inox_mod_i64" "call i64 @__inox_shl_i64" "call i64 @__inox_shr_i64" "and i64" "or i64" "xor i64" "ret i64" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/llvm-bool-comparisons.inox" \
    "define i1 @inox_isgreater" "define i1 @inox_isequal" "define i1 @inox_isdifferent" "icmp sgt i64" "icmp eq i64" "icmp ne i64" "icmp slt i64" "icmp sle i64" "icmp sge i64" "ret i1" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/llvm-bool-operators.inox" \
    "define i1 @inox_both" "define i1 @inox_either" "define i1 @inox_different" "define i1 @inox_notpositive" "and i1" "or i1" "xor i1" "xor i1 %tmp0, true" "ret i1" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/llvm-if-return.inox" \
    "define i64 @inox_max" "icmp sgt i64" "br i1" "label %then0" "label %else0" "then0:" "else0:" "ret i64" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/llvm-if-merge.inox" \
    "define i64 @inox_maxplusone" "%m = alloca i64" "icmp sgt i64" "br i1" "label %then0" "label %else0" "then0:" "else0:" "br label %endif0" "endif0:" "store i64" "load i64" "call i64 @__inox_add_i64" "ret i64" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/llvm-while-loop.inox" \
    "define i64 @inox_sumto" "whilecond0:" "whilebody0:" "whileend0:" "br i1" "br label %whilecond0" "icmp sgt i64" "call i64 @__inox_add_i64" "call i64 @__inox_sub_i64" "ret i64" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/llvm-while-break-continue.inox" \
    "define i64 @inox_findfirstbelow" "whilecond0:" "whilebody0:" "whileend0:" "br i1" "br label %whilecond0" "br label %whileend0" "icmp eq i64" "call i64 @__inox_sub_i64" "store i64" "ret i64" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/llvm-if-no-else.inox" \
    "define i64 @inox_clamppositive" "%x = alloca i64" "icmp slt i64" "br i1" "label %then0" "label %endif0" "then0:" "br label %endif0" "endif0:" "store i64" "load i64" "ret i64" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/llvm-elif-return.inox" \
    "define i64 @inox_compare" "icmp sgt i64" "icmp eq i64" "br i1" "elifcond0_0:" "elifthen0_0:" "ret i64 1" "ret i64 0" "ret i64 -1" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/llvm-repeat-flexible-end.inox" \
    "define i64 @inox_countdown" "repeatbody" "repeatend" "br i1" "br label" "icmp" "ret i64" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/llvm-repeat-flexible-start.inox" \
    "define i64 @inox_countdown" "repeatbody" "repeatcontinue" "repeatend" "br i1" "br label" "icmp" "ret i64" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/llvm-repeat-flexible-middle.inox" \
    "define i64 @inox_countdown" "repeatbody" "repeatcontinue" "repeatend" "br i1" "br label" "icmp" "ret i64" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/llvm-repeat-break-continue.inox" \
    "define i64 @inox_findvalue" "repeatbody" "repeatend" "br i1" "br label" "icmp eq i64" "call i64 @__inox_sub_i64" "store i64" "ret i64" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/llvm-for-range-break-continue.inox" \
    "define i64 @inox_sumrange" "forcond" "forbody" "forstep" "forend" "br i1" "br label" "icmp sle i64" "icmp eq i64" "call i64 @__inox_add_i64" "@llvm.sadd.with.overflow.i64" "store i64" "load i64" "ret i64" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/llvm-for-range-step.inox" \
    "define i64 @inox_sumevenuntil" "forcond" "forbody" "forstep" "forend" "store i64 2, ptr %i" "icmp sle i64" "icmp eq i64" "call i64 @__inox_add_i64" "@__inox_for_step_i64(i64 2)" "@llvm.sadd.with.overflow.i64" "br i1" "br label" "ret i64" "define i32 @main()" "ret i32 0"

run_llvm_emission_test "$repo_root/examples/llvm-putln-integer.inox" \
    "@.inox.fmt.i64.nl" "declare i32 @printf" "define i64 @inox_value" "define i32 @main()" "call i32 (ptr, ...) @printf" "ret i32 0"

run_llvm_emission_test "$repo_root/examples/llvm-put-output-basic.inox" \
    "@.inox.fmt.str.nl" "@.inox.fmt.str" "@.inox.true" "@.inox.false" "@.inox.str." "select i1" "call i32 (ptr, ...) @printf" "define i32 @main()" "ret i32 0"

run_llvm_emission_test "$repo_root/examples/llvm-subroutine-calls.inox" \
    "define i64 @inox_value" "define void @inox_report" "call void @inox_report" "ret void" "report=" "call i32 (ptr, ...) @printf" "define i32 @main()" "ret i32 0"

run_llvm_emission_test "$repo_root/examples/llvm-struct-basic.inox" \
    "%tpoint = type { i64, i64 }" "define i64 @inox_sumpoint" "alloca %tpoint" "zeroinitializer" "getelementptr %tpoint" "store i64 10" "store i64 20" "load i64" "call i64 @__inox_add_i64" "call i64 @inox_sumpoint" "ret i32 0"

run_llvm_emission_test "$repo_root/examples/llvm-associated-methods.inox" \
    "%tpoint = type { i64, i64 }" "define void @inox_tpoint.move" "define i64 @inox_tpoint.sum" "ptr %self" "call void @inox_tpoint.move" "call i64 @inox_tpoint.sum" "getelementptr %tpoint" "ret void" "ret i64" "define i32 @main()" "ret i32 0"

run_llvm_emission_test "$repo_root/examples/llvm-struct-field-defaults.inox" \
    "%tconfig = type { i64, i1 }" "define i64 @inox_getport" "alloca %tconfig" "zeroinitializer" "store i64 8080" "store i1 1" "getelementptr %tconfig" "load i64" "call i64 @inox_getport" "ret i32 0"

run_llvm_emission_test "$repo_root/examples/with-statement.inox" \
    "%tpoint = type { i64, i64 }" "define i64 @inox_sumpoint" "alloca %tpoint" "getelementptr %tpoint" "store i64 10" "store i64 20" "load i64" "call i64 @__inox_add_i64" "call i64 @inox_sumpoint" "ret i32 0"

run_llvm_emission_test "$repo_root/examples/llvm-struct-values.inox" \
    "%tpoint = type { i64, i64 }" "define %tpoint @inox_makepoint" "define i64 @inox_sumpoint" "define %tpoint @inox_copypoint" "%p.addr = alloca %tpoint" "store %tpoint %p, ptr %p.addr" "load %tpoint" "ret %tpoint" "call %tpoint @inox_makepoint" "call %tpoint @inox_copypoint" "call i64 @inox_sumpoint" "ret i32 0"


run_llvm_emission_test "$repo_root/tests/codegen/llvm-struct-value-smoke.inox"     "%tpair = type { i64, i64 }" "define %tpair @inox_makepair" "define i64 @inox_sumpair" "call %tpair @inox_makepair" "call i64 @inox_sumpair" "ret i32 0"
run_llvm_emission_test "$repo_root/tests/codegen/llvm-exceptions-smoke.inox"     "personality ptr @__gxx_personality_v0" "invoke void @inox_fail()" "landingpad { ptr, i32 } catch ptr null" "call i64 @__inox_exception_type" "call void @__inox_exception_release" "call void @__inox_exception_rethrow"
run_llvm_emission_test "$repo_root/tests/codegen/llvm-exceptions-retry-smoke.inox" \
    "%eh.retry.slot" "%eh.action.slot" "icmp slt i64" "store i32 2" "switch i32" "eh.retry.perform"

run_linked_execution_test "$repo_root/tests/integration/output-basic.inox" "$repo_root/tests/integration/output-basic.out"
run_build_driver_test "$repo_root/tests/integration/run-hello.inox"
run_driver_execution_test "$repo_root/tests/integration/run-hello.inox" "$repo_root/tests/integration/run-hello.out"
run_driver_execution_test "$repo_root/tests/integration/exceptions/typed-finally.inox" "$repo_root/tests/integration/exceptions/typed-finally.out"
run_driver_execution_test "$repo_root/tests/integration/exceptions/rethrow.inox" "$repo_root/tests/integration/exceptions/rethrow.out"
run_driver_execution_test "$repo_root/tests/integration/exceptions/plain-except.inox" "$repo_root/tests/integration/exceptions/plain-except.out"
run_driver_execution_test "$repo_root/tests/integration/exceptions/finally-propagation.inox" "$repo_root/tests/integration/exceptions/finally-propagation.out"
run_driver_execution_test "$repo_root/tests/integration/exceptions/retry-success.inox" "$repo_root/tests/integration/exceptions/retry-success.out"
run_driver_execution_test "$repo_root/tests/integration/exceptions/retry-exhausted.inox" "$repo_root/tests/integration/exceptions/retry-exhausted.out"
run_driver_execution_test "$repo_root/tests/integration/exceptions/retry-else.inox" "$repo_root/tests/integration/exceptions/retry-else.out"
run_driver_execution_test "$repo_root/tests/integration/exceptions/retry-zero.inox" "$repo_root/tests/integration/exceptions/retry-zero.out"
run_driver_execution_test "$repo_root/tests/integration/exceptions/retry-nested.inox" "$repo_root/tests/integration/exceptions/retry-nested.out"
run_driver_execution_test "$repo_root/tests/integration/exceptions/taxonomy-arithmetic.inox" "$repo_root/tests/integration/exceptions/taxonomy-arithmetic.out"
run_driver_execution_test "$repo_root/tests/integration/exceptions/taxonomy-range.inox" "$repo_root/tests/integration/exceptions/taxonomy-range.out"
run_driver_execution_test "$repo_root/tests/integration/exceptions/finally-control-transfers.inox" "$repo_root/tests/integration/exceptions/finally-control-transfers.out"
run_driver_execution_test "$repo_root/tests/integration/modules/Main.inox" "$repo_root/tests/integration/modules/Main.out"
run_driver_execution_test "$repo_root/tests/integration/modules/math-showcase.inox" "$repo_root/tests/integration/modules/math-showcase.out"
run_driver_execution_test "$repo_root/tests/integration/stdlib/StdMathDemo.inox" "$repo_root/tests/integration/stdlib/StdMathDemo.out"
run_driver_execution_test "$repo_root/tests/integration/stdlib/StdMathExpanded.inox" "$repo_root/tests/integration/stdlib/StdMathExpanded.out"
run_driver_execution_test "$repo_root/tests/integration/showcase/account-showcase.inox" "$repo_root/tests/integration/showcase/account-showcase.out"
run_driver_execution_test "$repo_root/tests/integration/output/variadic-put.inox" "$repo_root/tests/integration/output/variadic-put.out"
run_runtime_tree "$repo_root/tests/integration/input"
run_runtime_tree "$repo_root/tests/runtime"
run_diagnostic_tree "$repo_root/tests/diagnostics"
run_mode_exit_test --emit-llvm "$repo_root/tests/integration/cycles/Cycle.A.inox" false

total=$((passed + failed))
echo ""
echo "Summary: $passed passed, $failed failed, $total total"

if [[ $failed -ne 0 ]]; then
    exit 1
fi

exit 0
