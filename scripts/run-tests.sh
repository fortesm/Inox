#!/usr/bin/env bash
set -uo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
inox_exe="${1:-}"

if [[ -z "$inox_exe" ]]; then
    if [[ -x "$repo_root/build/inox" ]]; then
        inox_exe="$repo_root/build/inox"
    elif [[ -x "$repo_root/build-linux/inox" ]]; then
        inox_exe="$repo_root/build-linux/inox"
    elif [[ -x "$repo_root/build/Debug/inox.exe" ]]; then
        inox_exe="$repo_root/build/Debug/inox.exe"
    else
        inox_exe="$repo_root/build/inox"
    fi
elif [[ "$inox_exe" != /* ]]; then
    inox_exe="$repo_root/$inox_exe"
fi

if [[ ! -x "$inox_exe" && ! -f "$inox_exe" ]]; then
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

while IFS= read -r -d '' example; do
    run_inox_test "$example" true
done < <(find "$repo_root/examples" -maxdepth 1 -type f -name '*.inox' -print0 | sort -z)

while IFS= read -r -d '' test_file; do
    run_inox_test "$test_file" false
done < <(find "$repo_root/tests/invalid" -maxdepth 1 -type f -name '*.inox' -print0 | sort -z)

run_llvm_emission_test "$repo_root/examples/empty.inox" \
    "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/llvm-integer-function.inox" \
    "define i64 @sum" "%tmp0 = add i64 %a, %b" "ret i64 %tmp0" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/llvm-function-call.inox" \
    "define i64 @sum" "define i64 @double" "%tmp0 = call i64 @sum(i64 %x, i64 %x)" "ret i64 %tmp0" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/llvm-local-variables.inox" \
    "define i64 @compute" "%a = alloca i64" "%b = alloca i64" "store i64 10, ptr %a" "store i64 20, ptr %b" "load i64, ptr %a" "load i64, ptr %b" "add i64" "ret i64" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/llvm-local-assignment.inox" \
    "define i64 @compute" "%a = alloca i64" "%b = alloca i64" "store i64 10, ptr %a" "store i64 20, ptr %b" "add i64" "mul i64" "store i64 %tmp2, ptr %a" "store i64 %tmp4, ptr %b" "ret i64" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/llvm-integer-operators.inox" \
    "define i64 @compute" "%tmp0 = sdiv i64 %a, %b" "%tmp1 = sdiv i64 %a, %b" "srem i64" "shl i64" "ashr i64" "and i64" "or i64" "xor i64" "ret i64" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/llvm-bool-comparisons.inox" \
    "define i1 @isgreater" "define i1 @isequal" "define i1 @isdifferent" "icmp sgt i64" "icmp eq i64" "icmp ne i64" "icmp slt i64" "icmp sle i64" "icmp sge i64" "ret i1" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/llvm-bool-operators.inox" \
    "define i1 @both" "define i1 @either" "define i1 @different" "define i1 @notpositive" "and i1" "or i1" "xor i1" "xor i1 %tmp0, true" "ret i1" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/llvm-if-return.inox" \
    "define i64 @max" "icmp sgt i64" "br i1" "label %then0" "label %else0" "then0:" "else0:" "ret i64" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/llvm-if-merge.inox" \
    "define i64 @maxplusone" "%m = alloca i64" "icmp sgt i64" "br i1" "label %then0" "label %else0" "then0:" "else0:" "br label %endif0" "endif0:" "store i64" "load i64" "add i64" "ret i64" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/llvm-while-loop.inox" \
    "define i64 @sumto" "whilecond0:" "whilebody0:" "whileend0:" "br i1" "br label %whilecond0" "icmp sgt i64" "add i64" "sub i64" "ret i64" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/llvm-while-break-continue.inox" \
    "define i64 @findfirstbelow" "whilecond0:" "whilebody0:" "whileend0:" "br i1" "br label %whilecond0" "br label %whileend0" "icmp eq i64" "sub i64" "store i64" "ret i64" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/llvm-if-no-else.inox" \
    "define i64 @clamppositive" "%x = alloca i64" "icmp slt i64" "br i1" "label %then0" "label %endif0" "then0:" "br label %endif0" "endif0:" "store i64" "load i64" "ret i64" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/llvm-elif-return.inox" \
    "define i64 @compare" "icmp sgt i64" "icmp eq i64" "br i1" "elifcond0_0:" "elifthen0_0:" "ret i64 1" "ret i64 0" "ret i64 -1" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/llvm-repeat-flexible-end.inox" \
    "define i64 @countdown" "repeatbody" "repeatend" "br i1" "br label" "icmp" "ret i64" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/llvm-repeat-flexible-start.inox" \
    "define i64 @countdown" "repeatbody" "repeatcontinue" "repeatend" "br i1" "br label" "icmp" "ret i64" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/llvm-repeat-flexible-middle.inox" \
    "define i64 @countdown" "repeatbody" "repeatcontinue" "repeatend" "br i1" "br label" "icmp" "ret i64" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/llvm-repeat-break-continue.inox" \
    "define i64 @findvalue" "repeatbody" "repeatend" "br i1" "br label" "icmp eq i64" "sub i64" "store i64" "ret i64" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/llvm-for-range-break-continue.inox" \
    "define i64 @sumrange" "forcond" "forbody" "forstep" "forend" "br i1" "br label" "icmp sle i64" "icmp eq i64" "add i64" "store i64" "load i64" "ret i64" "define i32 @main()" "ret i32 0"
run_llvm_emission_test "$repo_root/examples/llvm-for-range-step.inox" \
    "define i64 @sumevenuntil" "forcond" "forbody" "forstep" "forend" "store i64 2, ptr %i" "icmp sle i64" "icmp eq i64" "add i64" ", 2" "br i1" "br label" "ret i64" "define i32 @main()" "ret i32 0"

run_llvm_emission_test "$repo_root/examples/llvm-putln-integer.inox" \
    "@.inox.fmt.i64.nl" "declare i32 @printf" "define i64 @value" "define i32 @main()" "call i32 (ptr, ...) @printf" "ret i32 0"

run_llvm_emission_test "$repo_root/examples/llvm-put-output-basic.inox" \
    "@.inox.fmt.str.nl" "@.inox.fmt.str" "@.inox.true" "@.inox.false" "@.inox.str." "select i1" "call i32 (ptr, ...) @printf" "define i32 @main()" "ret i32 0"

run_llvm_emission_test "$repo_root/examples/llvm-subroutine-calls.inox" \
    "define i64 @value" "define void @report" "call void @report" "ret void" "report=" "call i32 (ptr, ...) @printf" "define i32 @main()" "ret i32 0"

run_llvm_emission_test "$repo_root/examples/llvm-struct-basic.inox" \
    "%tpoint = type { i64, i64 }" "define i64 @sumpoint" "alloca %tpoint" "zeroinitializer" "getelementptr %tpoint" "store i64 10" "store i64 20" "load i64" "add i64" "call i64 @sumpoint" "ret i32 0"

run_llvm_emission_test "$repo_root/examples/llvm-associated-methods.inox" \
    "%tpoint = type { i64, i64 }" "define void @tpoint.move" "define i64 @tpoint.sum" "ptr %self" "call void @tpoint.move" "call i64 @tpoint.sum" "getelementptr %tpoint" "ret void" "ret i64" "define i32 @main()" "ret i32 0"

run_llvm_emission_test "$repo_root/examples/llvm-struct-field-defaults.inox" \
    "%tconfig = type { i64, i1 }" "define i64 @getport" "alloca %tconfig" "zeroinitializer" "store i64 8080" "store i1 1" "getelementptr %tconfig" "load i64" "call i64 @getport" "ret i32 0"

run_llvm_emission_test "$repo_root/examples/llvm-struct-values.inox" \
    "%tpoint = type { i64, i64 }" "define %tpoint @makepoint" "define i64 @sumpoint" "define %tpoint @copypoint" "%p.addr = alloca %tpoint" "store %tpoint %p, ptr %p.addr" "load %tpoint" "ret %tpoint" "call %tpoint @makepoint" "call %tpoint @copypoint" "call i64 @sumpoint" "ret i32 0"

total=$((passed + failed))
echo ""
echo "Summary: $passed passed, $failed failed, $total total"

if [[ $failed -ne 0 ]]; then
    exit 1
fi

exit 0
