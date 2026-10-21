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

    "$inox_exe" check "$test_file" >/dev/null 2>&1
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

    # E1 (change console-abi-terminal): every program links libinoxrt.
    local runtime_lib="${INOX_RUNTIME_LIB:-}"
    if [[ -z "$runtime_lib" ]]; then
        runtime_lib="$(dirname "$inox_exe")/libinoxrt.a"
        [[ -f "$runtime_lib" ]] || runtime_lib="$(dirname "$inox_exe")/inoxrt.lib"
    fi
    local clang_args=("$ll_path" "$runtime_lib" -o "$exe_path")
    if [[ "$(uname -s)" != MINGW* && "$(uname -s)" != MSYS* && "$(uname -s)" != CYGWIN* ]]; then
        clang_args+=("-lm")
    fi
    clang++ "${clang_args[@]}" >/dev/null 2>&1
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

# The driver must show the toolchain's own diagnostics when clang fails. The
# failure is provoked portably: a directory occupies the executable path, so the
# link step cannot write its output.
run_toolchain_failure_test() {
    local test_file="$1"
    local rel stem out_dir actual exit_code
    rel="$(relative_path "$test_file")"

    if ! command -v clang >/dev/null 2>&1; then
        echo "[SKIP] $rel --build (toolchain diagnostics; clang not found)"
        return 0
    fi

    stem="$(basename "$test_file" .inox)"
    out_dir="$(mktemp -d)"
    mkdir -p "$out_dir/$stem" "$out_dir/$stem.exe"
    actual="$(INOX_OUTPUT_DIR="$out_dir" "$inox_exe" --build "$test_file" 2>&1 >/dev/null)"
    exit_code=$?
    rm -rf "$out_dir"
    if [[ $exit_code -ne 0 && "$actual" == *"failed while building"* &&
          "$actual" == *"linker command failed"* && "$actual" == *"full toolchain output"* ]]; then
        record_pass "$rel --build (toolchain diagnostics)"
    else
        record_fail "$rel --build (toolchain diagnostics)" "exit code: $exit_code" "actual error: $actual"
    fi
}

# The third argument is the driver command (CANON-23): `--run` (default),
# `run`, or `-` for `inox <file>` alone, which also builds and runs.
run_driver_execution_test() {
    local test_file="$1"
    local expected_file="$2"
    local command="${3:---run}"
    local rel label
    rel="$(relative_path "$test_file")"
    label="$rel $command"
    [[ "$command" == "-" ]] && label="$rel (inox <file>)"

    if ! command -v clang >/dev/null 2>&1; then
        echo "[SKIP] $label (clang not found)"
        return 0
    fi

    local actual expected exit_code
    if [[ "$command" == "-" ]]; then
        actual="$("$inox_exe" "$test_file" 2>&1)"
    else
        actual="$("$inox_exe" "$command" "$test_file" 2>&1)"
    fi
    exit_code=$?
    expected="$(sed 's/\r$//' "$expected_file")"
    actual="$(printf '%s' "$actual" | sed 's/\r$//')"

    if [[ $exit_code -eq 0 && "$actual" == "$expected" ]]; then
        record_pass "$label"
    else
        record_fail "$label" "exit code: $exit_code" "expected output: $expected" "actual output: $actual"
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
    # handler or ensure block ran.
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

# Every examples/NAME.inox must build and run: its expected output is
# tests/examples/NAME.out (and its input, if any, tests/examples/NAME.in), so
# the examples directory holds only programs.
run_examples() {
    local root="$1"
    local expected_root="$2"
    while IFS= read -r -d '' example; do
        local name expected input
        name="$(basename "$example" .inox)"
        expected="$expected_root/$name.out"
        input="$expected_root/$name.in"
        if [[ ! -f "$expected" ]]; then
            record_fail "$(relative_path "$example") runs" "missing expected output: $(relative_path "$expected")"
        elif [[ -f "$input" ]]; then
            run_driver_input_test "$example" "$input" "$expected"
        else
            # stdin from /dev/null: the loop reads the file list from stdin.
            run_driver_execution_test "$example" "$expected" < /dev/null
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
    'Identifier lexeme="Main" normalized="main"' 'Keyword lexeme="Type" normalized="type"' 'Keyword lexeme="Struct" normalized="struct"' 'Keyword lexeme="Retry" normalized="retry"' 'IntegerLiteral lexeme="$2A"' 'StringLiteral lexeme="hello"' 'CharLiteral lexeme=' 'Identifier lexeme="End" normalized="end"' 'Identifier lexeme="Leave" normalized="leave"' 'Keyword lexeme="Ensure" normalized="ensure"' 'Identifier lexeme="Break" normalized="break"' 'Identifier lexeme="Finally" normalized="finally"'
run_mode_exit_test --parse-only "$repo_root/tests/parser/valid/canonical-type-and-var.inox" true
run_mode_exit_test --parse-only "$repo_root/tests/parser/invalid/var-colon.inox" false

run_llvm_emission_test "$repo_root/examples/empty.inox" \
    "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/integer-function.inox" \
    "define i64 @inox_sum" "%tmp0 = call i64 @__inox_add_i64(i64 %a, i64 %b)" "ret i64 %tmp0" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/function-call.inox" \
    "define i64 @inox_sum" "define i64 @inox_double" "%tmp0 = call i64 @inox_sum(i64 %x, i64 %x)" "ret i64 %tmp0" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/local-variables.inox" \
    "define i64 @inox_compute" "%a = alloca i64" "%b = alloca i64" "store i64 10, ptr %a" "store i64 20, ptr %b" "load i64, ptr %a" "load i64, ptr %b" "call i64 @__inox_add_i64" "ret i64" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/inline-typed-local.inox" \
    "define i64 @inox_compute" "%a = alloca i64" "%b = alloca i64" "store i64 10, ptr %a" "store i64 20, ptr %b" "load i64, ptr %a" "load i64, ptr %b" "call i64 @__inox_add_i64" "ret i64" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/local-assignment.inox" \
    "define i64 @inox_compute" "%a = alloca i64" "%b = alloca i64" "store i64 10, ptr %a" "store i64 20, ptr %b" "call i64 @__inox_add_i64" "call i64 @__inox_mul_i64" "store i64 %tmp0, ptr %a" "store i64 %tmp3, ptr %b" "ret i64" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/integer-operators.inox" \
    "define i64 @inox_compute" "%tmp0 = call i64 @__inox_div_i64(i64 %a, i64 %b)" "call i64 @__inox_mod_i64" "call i64 @__inox_shl_i64" "call i64 @__inox_shr_i64" "and i64" "or i64" "xor i64" "ret i64" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/bool-comparisons.inox" \
    "define i1 @inox_isgreater" "define i1 @inox_isequal" "define i1 @inox_isdifferent" "icmp sgt i64" "icmp eq i64" "icmp ne i64" "icmp slt i64" "icmp sle i64" "icmp sge i64" "ret i1" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/bool-operators.inox" \
    "define i1 @inox_both" "define i1 @inox_either" "define i1 @inox_different" "define i1 @inox_notpositive" "and i1" "or i1" "xor i1" "xor i1 %tmp0, true" "ret i1" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/if-return.inox" \
    "define i64 @inox_max" "icmp sgt i64" "br i1" "label %then0" "label %else0" "then0:" "else0:" "ret i64" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/if-merge.inox" \
    "define i64 @inox_maxplusone" "%m = alloca i64" "icmp sgt i64" "br i1" "label %then0" "label %else0" "then0:" "else0:" "br label %endif0" "endif0:" "store i64" "load i64" "call i64 @__inox_add_i64" "ret i64" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/while-loop.inox" \
    "define i64 @inox_sumto" "whilecond0:" "whilebody0:" "whileend0:" "br i1" "br label %whilecond0" "icmp sgt i64" "call i64 @__inox_add_i64" "call i64 @__inox_sub_i64" "ret i64" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/while-leave-continue.inox" \
    "define i64 @inox_findfirstbelow" "whilecond0:" "whilebody0:" "whileend0:" "br i1" "br label %whilecond0" "br label %whileend0" "icmp eq i64" "call i64 @__inox_sub_i64" "store i64" "ret i64" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/if-no-else.inox" \
    "define i64 @inox_clamppositive" "%x = alloca i64" "icmp slt i64" "br i1" "label %then0" "label %endif0" "then0:" "br label %endif0" "endif0:" "store i64" "load i64" "ret i64" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/elif-return.inox" \
    "define i64 @inox_compare" "icmp sgt i64" "icmp eq i64" "br i1" "elifcond0_0:" "elifthen0_0:" "ret i64 1" "ret i64 0" "ret i64 -1" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/repeat-flexible-end.inox" \
    "define i64 @inox_countdown" "repeatbody" "repeatend" "br i1" "br label" "icmp" "ret i64" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/repeat-flexible-start.inox" \
    "define i64 @inox_countdown" "repeatbody" "repeatend" "br i1" "br label" "icmp" "ret i64" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/repeat-flexible-middle.inox" \
    "define i64 @inox_countdown" "repeatbody" "repeatend" "br i1" "br label" "icmp" "ret i64" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/repeat-leave-continue.inox" \
    "define i64 @inox_findvalue" "repeatbody" "repeatend" "br i1" "br label" "icmp eq i64" "call i64 @__inox_sub_i64" "store i64" "ret i64" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/for-range-leave-continue.inox" \
    "define i64 @inox_sumrange" "forcond" "forbody" "forstep" "forend" "br i1" "br label" "icmp sle i64" "icmp eq i64" "call i64 @__inox_add_i64" "@llvm.sadd.with.overflow.i64" "store i64" "load i64" "ret i64" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/for-range-step.inox" \
    "define i64 @inox_sumevenuntil" "forcond" "forbody" "forstep" "forend" "store i64 2, ptr %i" "icmp sle i64" "icmp eq i64" "call i64 @__inox_add_i64" "@__inox_for_step_i64(i64 2)" "@llvm.sadd.with.overflow.i64" "br i1" "br label" "ret i64" "define i32 @main()" "ret i32 0"

run_llvm_emission_test "$repo_root/examples/putln-integer.inox" \
    "declare void @__inox_con_put_i64(i64)" "define i64 @inox_value" "define i32 @main()" "call void @__inox_runtime_init()" "call void @__inox_con_put_i64" "call void @__inox_con_put_newline()" "ret i32 0"

run_llvm_emission_test "$repo_root/examples/put-output-basic.inox" \
    "@.inox.str." "call void @__inox_con_write(ptr @.inox.str." "call void @__inox_con_put_bool(i1 zeroext" "call void @__inox_con_put_newline()" "define i32 @main()" "ret i32 0"

run_llvm_emission_test "$repo_root/examples/subroutine-calls.inox" \
    "define i64 @inox_value" "define void @inox_report" "call void @inox_report" "ret void" "report=" "call void @__inox_con_write" "define i32 @main()" "ret i32 0"

run_llvm_emission_test "$repo_root/examples/struct-basic.inox" \
    "%tpoint = type { i64, i64 }" "define i64 @inox_sumpoint" "alloca %tpoint" "zeroinitializer" "getelementptr %tpoint" "store i64 10" "store i64 20" "load i64" "call i64 @__inox_add_i64" "call i64 @inox_sumpoint" "ret i32 0"

run_llvm_emission_test "$repo_root/examples/associated-methods.inox" \
    "%tpoint = type { i64, i64 }" "define void @inox_tpoint.move" "define i64 @inox_tpoint.sum" "ptr %self" "call void @inox_tpoint.move" "call i64 @inox_tpoint.sum" "getelementptr %tpoint" "ret void" "ret i64" "define i32 @main()" "ret i32 0"

run_llvm_emission_test "$repo_root/examples/struct-field-defaults.inox" \
    "%tconfig = type { i64, i1 }" "define i64 @inox_getport" "alloca %tconfig" "zeroinitializer" "store i64 8080" "store i1 1" "getelementptr %tconfig" "load i64" "call i64 @inox_getport" "ret i32 0"

run_llvm_emission_test "$repo_root/examples/with-statement.inox" \
    "%tpoint = type { i64, i64 }" "define i64 @inox_sumpoint" "alloca %tpoint" "getelementptr %tpoint" "store i64 10" "store i64 20" "load i64" "call i64 @__inox_add_i64" "call i64 @inox_sumpoint" "ret i32 0"

run_llvm_emission_test "$repo_root/examples/struct-values.inox" \
    "%tpoint = type { i64, i64 }" "define %tpoint @inox_makepoint" "define i64 @inox_sumpoint" "define %tpoint @inox_copypoint" "%p.addr = alloca %tpoint" "store %tpoint %p, ptr %p.addr" "load %tpoint" "ret %tpoint" "call %tpoint @inox_makepoint" "call %tpoint @inox_copypoint" "call i64 @inox_sumpoint" "ret i32 0"


run_llvm_emission_test "$repo_root/tests/codegen/llvm-struct-value-smoke.inox"     "%tpair = type { i64, i64 }" "define %tpair @inox_makepair" "define i64 @inox_sumpair" "call %tpair @inox_makepair" "call i64 @inox_sumpair" "ret i32 0"
run_llvm_emission_test "$repo_root/tests/codegen/llvm-string-append-smoke.inox" "call ptr @__inox_str_concat(i64 3" "call void @__inox_str_append(ptr %s, i64 1" "call void @__inox_str_append(ptr %s, i64 2" "call ptr @__inox_str_concat(i64 2" "ret i32 0"
run_llvm_emission_test "$repo_root/tests/codegen/llvm-exceptions-smoke.inox"     "personality ptr @__gxx_personality_v0" "invoke void @inox_fail()" "landingpad { ptr, i32 } catch ptr null" "call i64 @__inox_exception_type" "call void @__inox_exception_release" "call void @__inox_exception_rethrow"
run_llvm_emission_test "$repo_root/tests/codegen/llvm-exceptions-retry-smoke.inox" \
    "%eh.retry.slot" "%eh.action.slot" "icmp slt i64" "store i32 2" "switch i32" "eh.retry.perform"

run_linked_execution_test "$repo_root/tests/integration/output-basic.inox" "$repo_root/tests/integration/output-basic.out"
run_build_driver_test "$repo_root/tests/integration/run-hello.inox"
run_toolchain_failure_test "$repo_root/tests/integration/run-hello.inox"
run_driver_execution_test "$repo_root/tests/integration/run-hello.inox" "$repo_root/tests/integration/run-hello.out"
run_driver_execution_test "$repo_root/tests/integration/run-hello.inox" "$repo_root/tests/integration/run-hello.out" run
run_driver_execution_test "$repo_root/tests/integration/run-hello.inox" "$repo_root/tests/integration/run-hello.out" -

# Driver commands (CANON-23): `check` checks without building, `help` and
# `version` are aliases of their switches, a command without a file is an error.
run_driver_command_tests() {
    local hello="$repo_root/tests/integration/run-hello.inox"
    local output
    output="$("$inox_exe" check "$hello" 2>&1)"
    if [[ $? -eq 0 && "$output" == *"semantic ok"* ]]; then
        record_pass "inox check <file>"
    else
        record_fail "inox check <file>" "actual output: $output"
    fi
    if [[ "$("$inox_exe" version 2>&1)" == "$("$inox_exe" --version 2>&1)" ]]; then
        record_pass "inox version = inox --version"
    else
        record_fail "inox version = inox --version"
    fi
    output="$("$inox_exe" help 2>&1)"
    if [[ $? -eq 0 && "$output" == *"inox run <file.inox>"* ]]; then
        record_pass "inox help"
    else
        record_fail "inox help" "actual output: $output"
    fi
    "$inox_exe" run >/dev/null 2>&1
    if [[ $? -ne 0 ]]; then
        record_pass "inox run without a file is an error"
    else
        record_fail "inox run without a file is an error"
    fi
}
run_driver_command_tests
run_driver_execution_test "$repo_root/tests/integration/exceptions/typed-ensure.inox" "$repo_root/tests/integration/exceptions/typed-ensure.out"
run_driver_execution_test "$repo_root/tests/integration/exceptions/rethrow.inox" "$repo_root/tests/integration/exceptions/rethrow.out"
run_driver_execution_test "$repo_root/tests/integration/exceptions/plain-except.inox" "$repo_root/tests/integration/exceptions/plain-except.out"
run_driver_execution_test "$repo_root/tests/integration/exceptions/ensure-propagation.inox" "$repo_root/tests/integration/exceptions/ensure-propagation.out"
run_driver_execution_test "$repo_root/tests/integration/exceptions/retry-success.inox" "$repo_root/tests/integration/exceptions/retry-success.out"
run_driver_execution_test "$repo_root/tests/integration/exceptions/retry-exhausted.inox" "$repo_root/tests/integration/exceptions/retry-exhausted.out"
run_driver_execution_test "$repo_root/tests/integration/exceptions/retry-else.inox" "$repo_root/tests/integration/exceptions/retry-else.out"
run_driver_execution_test "$repo_root/tests/integration/exceptions/retry-zero.inox" "$repo_root/tests/integration/exceptions/retry-zero.out"
run_driver_execution_test "$repo_root/tests/integration/exceptions/retry-nested.inox" "$repo_root/tests/integration/exceptions/retry-nested.out"
run_driver_execution_test "$repo_root/tests/integration/exceptions/taxonomy-arithmetic.inox" "$repo_root/tests/integration/exceptions/taxonomy-arithmetic.out"
run_driver_execution_test "$repo_root/tests/integration/exceptions/taxonomy-range.inox" "$repo_root/tests/integration/exceptions/taxonomy-range.out"
run_driver_execution_test "$repo_root/tests/integration/exceptions/ensure-control-transfers.inox" "$repo_root/tests/integration/exceptions/ensure-control-transfers.out"
run_driver_execution_test "$repo_root/tests/integration/exceptions/nested-loop-ensure-transfers.inox" "$repo_root/tests/integration/exceptions/nested-loop-ensure-transfers.out"
run_driver_execution_test "$repo_root/tests/integration/exceptions/nested-loop-retry-raise.inox" "$repo_root/tests/integration/exceptions/nested-loop-retry-raise.out"
run_driver_execution_test "$repo_root/tests/integration/exceptions/until-crossing-ensure.inox" "$repo_root/tests/integration/exceptions/until-crossing-ensure.out"
run_driver_execution_test "$repo_root/tests/integration/modules/Main.inox" "$repo_root/tests/integration/modules/Main.out"
run_driver_execution_test "$repo_root/tests/integration/modules/math-showcase.inox" "$repo_root/tests/integration/modules/math-showcase.out"
run_driver_execution_test "$repo_root/tests/integration/modules/generics-across-modules.inox" "$repo_root/tests/integration/modules/generics-across-modules.out"
run_driver_execution_test "$repo_root/tests/integration/stdlib/StdMathDemo.inox" "$repo_root/tests/integration/stdlib/StdMathDemo.out"
run_driver_execution_test "$repo_root/tests/integration/stdlib/StdMathExpanded.inox" "$repo_root/tests/integration/stdlib/StdMathExpanded.out"
run_driver_execution_test "$repo_root/tests/integration/showcase/account-showcase.inox" "$repo_root/tests/integration/showcase/account-showcase.out"
run_driver_execution_test "$repo_root/tests/integration/output/variadic-put.inox" "$repo_root/tests/integration/output/variadic-put.out"
run_runtime_tree "$repo_root/tests/integration/input"
run_runtime_tree "$repo_root/tests/runtime"
run_examples "$repo_root/examples" "$repo_root/tests/examples"
run_diagnostic_tree "$repo_root/tests/diagnostics"
run_mode_exit_test --emit-llvm "$repo_root/tests/integration/cycles/Cycle.A.inox" false

# grammar/grammar.ebnf must agree with the constitution, the lexer and the parser.
run_grammar_consistency() {
    local python_exe=""
    if command -v python3 >/dev/null 2>&1; then
        python_exe="python3"
    elif command -v python >/dev/null 2>&1; then
        python_exe="python"
    else
        echo "[SKIP] grammar/grammar.ebnf consistency (Python 3 not found)"
        return
    fi
    local output
    output="$("$python_exe" "$repo_root/tools/grammar_consistency.py" "$repo_root" 2>&1)"
    if [[ $? -eq 0 ]]; then
        record_pass "grammar/grammar.ebnf consistency"
    else
        local details=()
        while IFS= read -r line; do
            [[ "$line" == *FAIL* || "$line" == "       "* ]] && details+=("$line")
        done <<< "$output"
        record_fail "grammar/grammar.ebnf consistency" "${details[@]}"
    fi
}
run_grammar_consistency

# `inox --version` must match the version and revision at the top of
# openspec/specs/constitution/revisions.md (constitution VERSIONING).
run_version_check() {
    local revisions="$repo_root/openspec/specs/constitution/revisions.md"
    local language revision expected actual
    # "Language version: Inox 0.2.0 pre-alpha"; the program prints
    # MAJOR.MINOR.RELEASE.BUILD, BUILD being set by each build.
    language="$(sed -n 's/^Language version: Inox \([0-9]*\.[0-9]*\.[0-9]*\) \([a-z-]*\).*/\1 \2/p' "$revisions" | head -n 1)"
    revision="$(sed -n 's/^Canon revision: \(r[0-9.]*\).*/\1/p' "$revisions" | head -n 1)"
    local number="${language%% *}"
    local stage="${language#* }"
    expected="inox ${number}.<BUILD> ${stage} (canon $revision)"
    actual="$("$inox_exe" --version 2>&1 | sed 's/\r$//')"
    local pattern="^inox ${number//./\\.}\\.[0-9]{1,4} ${stage} \\(canon ${revision//./\\.}\\)\$"
    if [[ -n "$language" && -n "$revision" && "$actual" =~ $pattern ]]; then
        record_pass "inox --version matches the constitution"
    else
        record_fail "inox --version matches the constitution" "expected: $expected" "actual: $actual"
    fi
}
run_version_check

# Unit tests built next to the compiler: the runtime library (inoxrt-tests)
# and the compiler's own modules (inox-compiler-tests).
run_unit_tests() {
    local name="$1"
    local label="$2"
    local unit_exe
    unit_exe="$(dirname "$inox_exe")/$name"
    if [[ ! -x "$unit_exe" ]]; then
        echo "[SKIP] $label ($name not built next to $inox_exe)"
        return
    fi
    local output
    output="$("$unit_exe" 2>&1)"
    if [[ $? -eq 0 ]]; then
        record_pass "$label"
    else
        local details=()
        while IFS= read -r line; do
            details+=("$line")
        done <<< "$output"
        record_fail "$label" "${details[@]}"
    fi
}
run_unit_tests inoxrt-tests "inoxrt unit tests"
run_unit_tests inox-compiler-tests "compiler unit tests"

total=$((passed + failed))
echo ""
echo "Summary: $passed passed, $failed failed, $total total"

if [[ $failed -ne 0 ]]; then
    exit 1
fi

exit 0
