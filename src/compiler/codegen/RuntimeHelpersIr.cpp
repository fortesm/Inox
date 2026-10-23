// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "RuntimeHelpersIr.h"

#include "LlvmIrEmitter.h"
#include "LlvmText.h"
#include "../support/Platform.h"

#include <cstddef>
#include <sstream>

namespace inox::compiler::codegen::detail {

std::string inputRuntimeHelpers()
{
    return R"llvm(define internal void @__inox_discard_line() {
entry:
  br label %loop

loop:
  %ch = call i32 @__inox_con_getc()
  %is_newline = icmp eq i32 %ch, 10
  %is_eof = icmp eq i32 %ch, -1
  %done = or i1 %is_newline, %is_eof
  br i1 %done, label %exit, label %loop

exit:
  ret void
}

define internal void @__inox_discard_token() {
entry:
  br label %skip_ws

skip_ws:
  %ch0 = call i32 @__inox_con_getc()
  %is_eof0 = icmp eq i32 %ch0, -1
  %is_space0 = icmp eq i32 %ch0, 32
  %is_tab0 = icmp eq i32 %ch0, 9
  %is_lf0 = icmp eq i32 %ch0, 10
  %is_cr0 = icmp eq i32 %ch0, 13
  %ws_a0 = or i1 %is_space0, %is_tab0
  %ws_b0 = or i1 %is_lf0, %is_cr0
  %is_ws0 = or i1 %ws_a0, %ws_b0
  %keep_skipping = and i1 %is_ws0, true
  br i1 %is_eof0, label %exit, label %after_eof

after_eof:
  br i1 %keep_skipping, label %skip_ws, label %consume

consume:
  %ch1 = phi i32 [ %ch0, %after_eof ], [ %ch2, %consume_next ]
  %is_eof1 = icmp eq i32 %ch1, -1
  %is_space1 = icmp eq i32 %ch1, 32
  %is_tab1 = icmp eq i32 %ch1, 9
  %is_lf1 = icmp eq i32 %ch1, 10
  %is_cr1 = icmp eq i32 %ch1, 13
  %ws_a1 = or i1 %is_space1, %is_tab1
  %ws_b1 = or i1 %is_lf1, %is_cr1
  %is_ws1 = or i1 %ws_a1, %ws_b1
  %done1a = or i1 %is_eof1, %is_ws1
  br i1 %done1a, label %exit, label %consume_next

consume_next:
  %ch2 = call i32 @__inox_con_getc()
  br label %consume

exit:
  ret void
}

define internal void @__inox_read_i64(ptr %out) {
entry:
  br label %skip_ws

skip_ws:
  %c0 = call i32 @__inox_con_getc()
  %eof0 = icmp eq i32 %c0, -1
  br i1 %eof0, label %trap, label %ws_check

ws_check:
  %is_sp = icmp eq i32 %c0, 32
  %is_tb = icmp eq i32 %c0, 9
  %is_lf = icmp eq i32 %c0, 10
  %is_cr = icmp eq i32 %c0, 13
  %ws_a = or i1 %is_sp, %is_tb
  %ws_b = or i1 %is_lf, %is_cr
  %is_ws = or i1 %ws_a, %ws_b
  br i1 %is_ws, label %skip_ws, label %sign_check

sign_check:
  %is_minus = icmp eq i32 %c0, 45
  br i1 %is_minus, label %minus, label %first_digit

minus:
  %cm = call i32 @__inox_con_getc()
  br label %first_digit

first_digit:
  %neg = phi i1 [ true, %minus ], [ false, %sign_check ]
  %cf = phi i32 [ %cm, %minus ], [ %c0, %sign_check ]
  %fge = icmp sge i32 %cf, 48
  %fle = icmp sle i32 %cf, 57
  %fis = and i1 %fge, %fle
  br i1 %fis, label %digits, label %trap

digits:
  %ch = phi i32 [ %cf, %first_digit ], [ %next_ch, %digit_ok ]
  %acc = phi i64 [ 0, %first_digit ], [ %sub_v, %digit_ok ]
  %dge = icmp sge i32 %ch, 48
  %dle = icmp sle i32 %ch, 57
  %dis = and i1 %dge, %dle
  br i1 %dis, label %digit_body, label %finish

digit_body:
  %d32 = sub i32 %ch, 48
  %d64 = sext i32 %d32 to i64
  %mul_p = call { i64, i1 } @llvm.smul.with.overflow.i64(i64 %acc, i64 10)
  %mul_v = extractvalue { i64, i1 } %mul_p, 0
  %mul_o = extractvalue { i64, i1 } %mul_p, 1
  %sub_p = call { i64, i1 } @llvm.ssub.with.overflow.i64(i64 %mul_v, i64 %d64)
  %sub_v = extractvalue { i64, i1 } %sub_p, 0
  %sub_o = extractvalue { i64, i1 } %sub_p, 1
  %ovf = or i1 %mul_o, %sub_o
  br i1 %ovf, label %trap, label %digit_ok

digit_ok:
  %next_ch = call i32 @__inox_con_getc()
  br label %digits

finish:
  %e_eof = icmp eq i32 %ch, -1
  %e_sp = icmp eq i32 %ch, 32
  %e_tb = icmp eq i32 %ch, 9
  %e_lf = icmp eq i32 %ch, 10
  %e_cr = icmp eq i32 %ch, 13
  %e_a = or i1 %e_eof, %e_sp
  %e_b = or i1 %e_tb, %e_lf
  %e_c = or i1 %e_a, %e_b
  %e_ok = or i1 %e_c, %e_cr
  br i1 %e_ok, label %sign_fix, label %trap

sign_fix:
  br i1 %neg, label %store_neg, label %store_pos

store_neg:
  store i64 %acc, ptr %out
  ret void

store_pos:
  %is_min = icmp eq i64 %acc, -9223372036854775808
  br i1 %is_min, label %trap, label %store_pos_ok

store_pos_ok:
  %pos = sub i64 0, %acc
  store i64 %pos, ptr %out
  ret void

trap:
  call void @__inox_arith_fault(i32 6)
  unreachable
}

)llvm";
}

namespace {

struct RuntimeFaultKind {
    int code;
    const char* message;
};

// CANON-19: a runtime arithmetic fault is a deterministic Inox trap. The
// program flushes its output, prints "Inox runtime error: <category>" on
// standard error and terminates with kRuntimeFaultExitStatus. There is no
// unwinding: try/except/ensure cannot intercept it.
constexpr RuntimeFaultKind kRuntimeFaultKinds[] = {
    {1, "integer overflow"},
    {2, "division by zero"},
    {3, "invalid shift count"},
    {4, "for-loop step must be positive"},
    {5, "negative exponent"},
    {6, "invalid integer input"},
    {7, "value out of range for Natural"},
    {8, "integer conversion out of range"},
    {9, "array index out of bounds"},
    {10, "slice bounds out of range"},
    {11, "value out of range"},
    {12, "out of memory"},
    {13, "invalid UTF-8 input"},
    {14, "assertion failed"},
    {15, "string index out of range"},
    {16, "use of a freed object or an empty Link"},
    {17, "use of an owner after Move or Free"},
    {18, "end of input"},
    {19, "invalid number input"},
    {20, "decimal overflow"},
};

} // namespace

std::string runtimeFaultSeam()
{
    const support::NativeErrorWriter writer = support::nativeErrorWriter();
    const std::string lengthType = "i" + std::to_string(writer.lengthBits);
    const std::string resultType = "i" + std::to_string(writer.resultBits);

    std::ostringstream ir;
    ir << "; Every deterministic runtime fault goes through this one function (CANON-19).\n"
       << "; It flushes the program's output and gives the terminal back (libinoxrt), prints \"Inox runtime error: <category>\" on\n"
       << "; standard error (file descriptor 2) and terminates with status "
       << kRuntimeFaultExitStatus << " without\n"
       << "; unwinding, so try/except/ensure cannot intercept it. Kinds: 1 integer\n"
       << "; overflow, 2 division or modulo by zero, 3 shift count outside 0..63, 4 for\n"
       << "; step <= 0, 5 negative exponent, 6 invalid integer input, 7 negative value\n"
       << "; converted to Natural, 8 integer conversion outside the target range, 9 array\n"
       << "; index outside Low..High, 10 slice bounds outside the base, 11 a value outside\n"
       << "; a Range or past the ends of Succ and Pred, 12 out of memory, 13 invalid UTF-8\n"
       << "; input, 14 a failed Std.Debug.Assert, 15 a position or count outside a String\n"
       << "; (Std.Strings), 16 a Link to a freed object, 17 an owner used after Move or\n"
       << "; Free, 18 a read past the end of input (ReadKey, Get of a Char), 19 invalid\n"
       << "; number input (Get of a real or a decimal), 20 a Currency or Crypto beyond its\n"
       << "; range.\n";
    for (const RuntimeFaultKind& kind : kRuntimeFaultKinds) {
        const std::string text = std::string("Inox runtime error: ") + kind.message + "\n";
        ir << "@.inox.fault." << kind.code << " = private unnamed_addr constant ["
           << text.size() << " x i8] c\"" << llvmByteString(text) << "\"\n";
    }
    ir << "declare " << resultType << " @" << writer.symbol << "(i32, ptr, " << lengthType << ")\n"
       << "declare void @_exit(i32) noreturn nounwind\n\n"
       << "define internal void @__inox_arith_fault(i32 %kind) noreturn nounwind cold noinline {\n"
       << "entry:\n"
       << "  call void @__inox_runtime_shutdown()\n"
       << "  switch i32 %kind, label %report [\n";
    for (const RuntimeFaultKind& kind : kRuntimeFaultKinds) {
        ir << "    i32 " << kind.code << ", label %kind" << kind.code << "\n";
    }
    ir << "  ]\n\n";
    for (const RuntimeFaultKind& kind : kRuntimeFaultKinds) {
        ir << "kind" << kind.code << ":\n  br label %report\n\n";
    }
    const RuntimeFaultKind& fallback = kRuntimeFaultKinds[0];
    ir << "report:\n  %message = phi ptr [ @.inox.fault." << fallback.code << ", %entry ]";
    for (const RuntimeFaultKind& kind : kRuntimeFaultKinds) {
        ir << ", [ @.inox.fault." << kind.code << ", %kind" << kind.code << " ]";
    }
    const std::size_t fallbackLength = std::string("Inox runtime error: ").size() +
                                       std::string(fallback.message).size() + 1;
    ir << "\n  %length = phi " << lengthType << " [ " << fallbackLength << ", %entry ]";
    for (const RuntimeFaultKind& kind : kRuntimeFaultKinds) {
        const std::size_t length = std::string("Inox runtime error: ").size() +
                                   std::string(kind.message).size() + 1;
        ir << ", [ " << length << ", %kind" << kind.code << " ]";
    }
    ir << "\n  %written = call " << resultType << " @" << writer.symbol << "(i32 2, ptr %message, "
       << lengthType << " %length)\n"
       << "  call void @_exit(i32 " << kRuntimeFaultExitStatus << ")\n"
       << "  unreachable\n"
       << "}\n";
    return ir.str();
}

std::string assertMessageHelper()
{
    const support::NativeErrorWriter writer = support::nativeErrorWriter();
    const std::string lengthType = "i" + std::to_string(writer.lengthBits);
    const std::string resultType = "i" + std::to_string(writer.resultBits);
    const std::string prefix = "Inox runtime error: assertion failed: ";

    std::ostringstream ir;
    ir << "; A failed Assert(Condition, Message) (Std.Debug): the trap of kind 14 with the\n"
       << "; message after the category, read from the String block { count, length, bytes }.\n"
       << "@.inox.assert.prefix = private unnamed_addr constant [" << prefix.size() << " x i8] c\""
       << llvmByteString(prefix) << "\"\n"
       << "@.inox.assert.newline = private unnamed_addr constant [1 x i8] c\"\\0A\"\n"
       << "define internal void @__inox_assert_message(ptr %message) noreturn nounwind cold noinline {\n"
       << "entry:\n"
       << "  call void @__inox_runtime_shutdown()\n"
       << "  %prefix = call " << resultType << " @" << writer.symbol << "(i32 2, ptr @.inox.assert.prefix, "
       << lengthType << " " << prefix.size() << ")\n"
       << "  %lengthPointer = getelementptr { i64, i64 }, ptr %message, i32 0, i32 1\n"
       << "  %length = load i64, ptr %lengthPointer\n";
    std::string length = "%length";
    if (writer.lengthBits != 64) {
        ir << "  %lengthNarrow = trunc i64 %length to " << lengthType << "\n";
        length = "%lengthNarrow";
    }
    ir << "  %bytes = getelementptr i8, ptr %message, i64 16\n"
       << "  %text = call " << resultType << " @" << writer.symbol << "(i32 2, ptr %bytes, " << lengthType
       << " " << length << ")\n"
       << "  %newline = call " << resultType << " @" << writer.symbol << "(i32 2, ptr @.inox.assert.newline, "
       << lengthType << " 1)\n"
       << "  call void @_exit(i32 " << kRuntimeFaultExitStatus << ")\n"
       << "  unreachable\n"
       << "}";
    return ir.str();
}

// CANON-24: a broken contract stops the program like any trap (CANON-19):
// "Inox runtime error: <text>", where the text (built by the compiler) names
// the category, the routine or struct, the condition and its message.
std::string contractMessageHelper()
{
    const support::NativeErrorWriter writer = support::nativeErrorWriter();
    const std::string lengthType = "i" + std::to_string(writer.lengthBits);
    const std::string resultType = "i" + std::to_string(writer.resultBits);
    const std::string prefix = "Inox runtime error: ";

    std::ostringstream ir;
    ir << "; A broken contract (pre, pos, invariant, loop invariant or variant): a trap\n"
       << "; whose text is read from the String block { count, length, bytes }.\n"
       << "@.inox.contract.prefix = private unnamed_addr constant [" << prefix.size() << " x i8] c\""
       << llvmByteString(prefix) << "\"\n"
       << "@.inox.contract.newline = private unnamed_addr constant [1 x i8] c\"\\0A\"\n"
       << "define internal void @__inox_contract_fault(ptr %text) noreturn nounwind cold noinline {\n"
       << "entry:\n"
       << "  call void @__inox_runtime_shutdown()\n"
       << "  %prefix = call " << resultType << " @" << writer.symbol << "(i32 2, ptr @.inox.contract.prefix, "
       << lengthType << " " << prefix.size() << ")\n"
       << "  %lengthPointer = getelementptr { i64, i64 }, ptr %text, i32 0, i32 1\n"
       << "  %length = load i64, ptr %lengthPointer\n";
    std::string length = "%length";
    if (writer.lengthBits != 64) {
        ir << "  %lengthNarrow = trunc i64 %length to " << lengthType << "\n";
        length = "%lengthNarrow";
    }
    ir << "  %bytes = getelementptr i8, ptr %text, i64 16\n"
       << "  %written = call " << resultType << " @" << writer.symbol << "(i32 2, ptr %bytes, " << lengthType
       << " " << length << ")\n"
       << "  %newline = call " << resultType << " @" << writer.symbol << "(i32 2, ptr @.inox.contract.newline, "
       << lengthType << " 1)\n"
       << "  call void @_exit(i32 " << kRuntimeFaultExitStatus << ")\n"
       << "  unreachable\n"
       << "}";
    return ir.str();
}

std::string mathRuntimeHelpers()
{
    return R"llvm(declare { i64, i1 } @llvm.smul.with.overflow.i64(i64, i64)

)llvm" + runtimeFaultSeam() + R"llvm(
declare { i64, i1 } @llvm.sadd.with.overflow.i64(i64, i64)
declare { i64, i1 } @llvm.ssub.with.overflow.i64(i64, i64)

define internal i64 @__inox_add_i64(i64 %a, i64 %b) alwaysinline nounwind {
entry:
  %p = call { i64, i1 } @llvm.sadd.with.overflow.i64(i64 %a, i64 %b)
  %v = extractvalue { i64, i1 } %p, 0
  %o = extractvalue { i64, i1 } %p, 1
  br i1 %o, label %trap, label %ok

ok:
  ret i64 %v

trap:
  call void @__inox_arith_fault(i32 1)
  unreachable
}

define internal i64 @__inox_sub_i64(i64 %a, i64 %b) alwaysinline nounwind {
entry:
  %p = call { i64, i1 } @llvm.ssub.with.overflow.i64(i64 %a, i64 %b)
  %v = extractvalue { i64, i1 } %p, 0
  %o = extractvalue { i64, i1 } %p, 1
  br i1 %o, label %trap, label %ok

ok:
  ret i64 %v

trap:
  call void @__inox_arith_fault(i32 1)
  unreachable
}

define internal i64 @__inox_mul_i64(i64 %a, i64 %b) alwaysinline nounwind {
entry:
  %p = call { i64, i1 } @llvm.smul.with.overflow.i64(i64 %a, i64 %b)
  %v = extractvalue { i64, i1 } %p, 0
  %o = extractvalue { i64, i1 } %p, 1
  br i1 %o, label %trap, label %ok

ok:
  ret i64 %v

trap:
  call void @__inox_arith_fault(i32 1)
  unreachable
}

define internal i64 @__inox_neg_i64(i64 %a) alwaysinline nounwind {
entry:
  %o = icmp eq i64 %a, -9223372036854775808
  br i1 %o, label %trap, label %ok

ok:
  %r = sub i64 0, %a
  ret i64 %r

trap:
  call void @__inox_arith_fault(i32 1)
  unreachable
}

define internal i64 @__inox_abs_i64(i64 %a) alwaysinline nounwind {
entry:
  %o = icmp eq i64 %a, -9223372036854775808
  br i1 %o, label %trap, label %ok

ok:
  %n = icmp slt i64 %a, 0
  %m = sub i64 0, %a
  %r = select i1 %n, i64 %m, i64 %a
  ret i64 %r

trap:
  call void @__inox_arith_fault(i32 1)
  unreachable
}

define internal i64 @__inox_div_i64(i64 %a, i64 %b) alwaysinline nounwind {
entry:
  %z = icmp eq i64 %b, 0
  br i1 %z, label %zero, label %check

check:
  %m1 = icmp eq i64 %b, -1
  %mn = icmp eq i64 %a, -9223372036854775808
  %ov = and i1 %m1, %mn
  br i1 %ov, label %overflow, label %ok

ok:
  %r = sdiv i64 %a, %b
  ret i64 %r

zero:
  call void @__inox_arith_fault(i32 2)
  unreachable

overflow:
  call void @__inox_arith_fault(i32 1)
  unreachable
}

define internal i64 @__inox_rem_i64(i64 %a, i64 %b) alwaysinline nounwind {
entry:
  %z = icmp eq i64 %b, 0
  br i1 %z, label %trap, label %ok

ok:
  %m1 = icmp eq i64 %b, -1
  %d = select i1 %m1, i64 1, i64 %b
  %q = srem i64 %a, %d
  %r = select i1 %m1, i64 0, i64 %q
  ret i64 %r

trap:
  call void @__inox_arith_fault(i32 2)
  unreachable
}

define internal i64 @__inox_mod_i64(i64 %a, i64 %b) alwaysinline nounwind {
entry:
  %r = call i64 @__inox_rem_i64(i64 %a, i64 %b)
  %nonzero = icmp ne i64 %r, 0
  %signs = xor i64 %r, %b
  %differ = icmp slt i64 %signs, 0
  %fix = and i1 %nonzero, %differ
  %adjusted = add i64 %r, %b
  %m = select i1 %fix, i64 %adjusted, i64 %r
  ret i64 %m
}

define internal i64 @__inox_shl_i64(i64 %a, i64 %b) alwaysinline nounwind {
entry:
  %bad = icmp ugt i64 %b, 63
  br i1 %bad, label %trap, label %ok

ok:
  ; r3.97 (CANON-8): A shl N is A * 2^N. The count is 0..63 here, so the
  ; plain shl has no poison; shifting back must give A, or bits were lost.
  %r = shl i64 %a, %b
  %back = ashr i64 %r, %b
  %kept = icmp eq i64 %back, %a
  br i1 %kept, label %done, label %overflow

done:
  ret i64 %r

overflow:
  call void @__inox_arith_fault(i32 1)
  unreachable

trap:
  call void @__inox_arith_fault(i32 3)
  unreachable
}

define internal i64 @__inox_shr_i64(i64 %a, i64 %b) alwaysinline nounwind {
entry:
  %bad = icmp ugt i64 %b, 63
  br i1 %bad, label %trap, label %ok

ok:
  %r = ashr i64 %a, %b
  ret i64 %r

trap:
  call void @__inox_arith_fault(i32 3)
  unreachable
}

define internal i64 @__inox_for_step_i64(i64 %s) alwaysinline nounwind {
entry:
  %bad = icmp sle i64 %s, 0
  br i1 %bad, label %trap, label %ok

ok:
  ret i64 %s

trap:
  call void @__inox_arith_fault(i32 4)
  unreachable
}

define internal i64 @__inox_range_i64(i64 %v, i64 %low, i64 %high, i32 %kind) alwaysinline nounwind {
entry:
  %below = icmp slt i64 %v, %low
  %above = icmp sgt i64 %v, %high
  %bad = or i1 %below, %above
  br i1 %bad, label %trap, label %ok

ok:
  ret i64 %v

trap:
  call void @__inox_arith_fault(i32 %kind)
  unreachable
}

define internal i64 @__inox_natural_i64(i64 %v) alwaysinline nounwind {
entry:
  %bad = icmp slt i64 %v, 0
  br i1 %bad, label %trap, label %ok

ok:
  ret i64 %v

trap:
  call void @__inox_arith_fault(i32 7)
  unreachable
}


define internal i64 @__inox_ipow_i64(i64 %base, i64 %exponent) {
entry:
  %negative = icmp slt i64 %exponent, 0
  br i1 %negative, label %negexp, label %loop

loop:
  %result.cur = phi i64 [ 1, %entry ], [ %result.next, %continue ]
  %base.cur = phi i64 [ %base, %entry ], [ %base.next, %continue ]
  %exp.cur = phi i64 [ %exponent, %entry ], [ %exp.next, %continue ]
  %done = icmp eq i64 %exp.cur, 0
  br i1 %done, label %exit, label %body

body:
  %lowbit = and i64 %exp.cur, 1
  %is_odd = icmp ne i64 %lowbit, 0
  br i1 %is_odd, label %mul_result, label %after_mul_result

mul_result:
  %mul.result.pair = call { i64, i1 } @llvm.smul.with.overflow.i64(i64 %result.cur, i64 %base.cur)
  %mul.result = extractvalue { i64, i1 } %mul.result.pair, 0
  %mul.result.overflow = extractvalue { i64, i1 } %mul.result.pair, 1
  br i1 %mul.result.overflow, label %overflow, label %after_mul_result

after_mul_result:
  %result.after = phi i64 [ %mul.result, %mul_result ], [ %result.cur, %body ]
  %exp.next = ashr i64 %exp.cur, 1
  %need_square = icmp ne i64 %exp.next, 0
  br i1 %need_square, label %square_base, label %continue

square_base:
  %square.pair = call { i64, i1 } @llvm.smul.with.overflow.i64(i64 %base.cur, i64 %base.cur)
  %square = extractvalue { i64, i1 } %square.pair, 0
  %square.overflow = extractvalue { i64, i1 } %square.pair, 1
  br i1 %square.overflow, label %overflow, label %continue

continue:
  %result.next = phi i64 [ %result.after, %after_mul_result ], [ %result.after, %square_base ]
  %base.next = phi i64 [ %base.cur, %after_mul_result ], [ %square, %square_base ]
  br label %loop

exit:
  ret i64 %result.cur

negexp:
  call void @__inox_arith_fault(i32 5)
  unreachable

overflow:
  call void @__inox_arith_fault(i32 1)
  unreachable
}

)llvm";
}

// UInt64 (change uint64): the 64-bit pattern read unsigned. A result below 0
// or above 2^64-1 is an overflow (kind 1), never a wraparound.
std::string unsignedRuntimeHelpers()
{
    return R"llvm(; UInt64 arithmetic (change uint64): overflow and underflow trap with kind 1.
declare { i64, i1 } @llvm.uadd.with.overflow.i64(i64, i64)
declare { i64, i1 } @llvm.usub.with.overflow.i64(i64, i64)
declare { i64, i1 } @llvm.umul.with.overflow.i64(i64, i64)

define internal i64 @__inox_addu_i64(i64 %a, i64 %b) alwaysinline nounwind {
entry:
  %p = call { i64, i1 } @llvm.uadd.with.overflow.i64(i64 %a, i64 %b)
  %v = extractvalue { i64, i1 } %p, 0
  %o = extractvalue { i64, i1 } %p, 1
  br i1 %o, label %trap, label %ok

ok:
  ret i64 %v

trap:
  call void @__inox_arith_fault(i32 1)
  unreachable
}

define internal i64 @__inox_subu_i64(i64 %a, i64 %b) alwaysinline nounwind {
entry:
  %p = call { i64, i1 } @llvm.usub.with.overflow.i64(i64 %a, i64 %b)
  %v = extractvalue { i64, i1 } %p, 0
  %o = extractvalue { i64, i1 } %p, 1
  br i1 %o, label %trap, label %ok

ok:
  ret i64 %v

trap:
  call void @__inox_arith_fault(i32 1)
  unreachable
}

define internal i64 @__inox_mulu_i64(i64 %a, i64 %b) alwaysinline nounwind {
entry:
  %p = call { i64, i1 } @llvm.umul.with.overflow.i64(i64 %a, i64 %b)
  %v = extractvalue { i64, i1 } %p, 0
  %o = extractvalue { i64, i1 } %p, 1
  br i1 %o, label %trap, label %ok

ok:
  ret i64 %v

trap:
  call void @__inox_arith_fault(i32 1)
  unreachable
}

define internal i64 @__inox_divu_i64(i64 %a, i64 %b) alwaysinline nounwind {
entry:
  %z = icmp eq i64 %b, 0
  br i1 %z, label %trap, label %ok

ok:
  %r = udiv i64 %a, %b
  ret i64 %r

trap:
  call void @__inox_arith_fault(i32 2)
  unreachable
}

define internal i64 @__inox_modu_i64(i64 %a, i64 %b) alwaysinline nounwind {
entry:
  %z = icmp eq i64 %b, 0
  br i1 %z, label %trap, label %ok

ok:
  %r = urem i64 %a, %b
  ret i64 %r

trap:
  call void @__inox_arith_fault(i32 2)
  unreachable
}

define internal i64 @__inox_shlu_i64(i64 %a, i64 %b) alwaysinline nounwind {
entry:
  %bad = icmp ugt i64 %b, 63
  br i1 %bad, label %count, label %shift

shift:
  %r = shl i64 %a, %b
  %back = lshr i64 %r, %b
  %lost = icmp ne i64 %back, %a
  br i1 %lost, label %overflow, label %ok

ok:
  ret i64 %r

count:
  call void @__inox_arith_fault(i32 3)
  unreachable

overflow:
  call void @__inox_arith_fault(i32 1)
  unreachable
}

define internal i64 @__inox_shru_i64(i64 %a, i64 %b) alwaysinline nounwind {
entry:
  %bad = icmp ugt i64 %b, 63
  br i1 %bad, label %trap, label %ok

ok:
  %r = lshr i64 %a, %b
  ret i64 %r

trap:
  call void @__inox_arith_fault(i32 3)
  unreachable
}

define internal i64 @__inox_for_stepu_i64(i64 %s) alwaysinline nounwind {
entry:
  %bad = icmp eq i64 %s, 0
  br i1 %bad, label %trap, label %ok

ok:
  ret i64 %s

trap:
  call void @__inox_arith_fault(i32 4)
  unreachable
}

define internal i64 @__inox_ipowu_i64(i64 %base, i64 %exponent) {
entry:
  br label %loop

loop:
  %result.cur = phi i64 [ 1, %entry ], [ %result.next, %continue ]
  %base.cur = phi i64 [ %base, %entry ], [ %base.next, %continue ]
  %exp.cur = phi i64 [ %exponent, %entry ], [ %exp.next, %continue ]
  %done = icmp eq i64 %exp.cur, 0
  br i1 %done, label %exit, label %body

body:
  %lowbit = and i64 %exp.cur, 1
  %is_odd = icmp ne i64 %lowbit, 0
  br i1 %is_odd, label %mul_result, label %after_mul_result

mul_result:
  %mul.result.pair = call { i64, i1 } @llvm.umul.with.overflow.i64(i64 %result.cur, i64 %base.cur)
  %mul.result = extractvalue { i64, i1 } %mul.result.pair, 0
  %mul.result.overflow = extractvalue { i64, i1 } %mul.result.pair, 1
  br i1 %mul.result.overflow, label %overflow, label %after_mul_result

after_mul_result:
  %result.after = phi i64 [ %mul.result, %mul_result ], [ %result.cur, %body ]
  %exp.next = lshr i64 %exp.cur, 1
  %need_square = icmp ne i64 %exp.next, 0
  br i1 %need_square, label %square_base, label %continue

square_base:
  %square.pair = call { i64, i1 } @llvm.umul.with.overflow.i64(i64 %base.cur, i64 %base.cur)
  %square = extractvalue { i64, i1 } %square.pair, 0
  %square.overflow = extractvalue { i64, i1 } %square.pair, 1
  br i1 %square.overflow, label %overflow, label %continue

continue:
  %result.next = phi i64 [ %result.after, %after_mul_result ], [ %result.after, %square_base ]
  %base.next = phi i64 [ %base.cur, %after_mul_result ], [ %square, %square_base ]
  br label %loop

exit:
  ret i64 %result.cur

overflow:
  call void @__inox_arith_fault(i32 1)
  unreachable
})llvm";
}

std::string unsignedInputHelper()
{
    return R"llvm(; Get into a UInt64 (change uint64): 0..18446744073709551615, else kind 6.
define internal void @__inox_read_u64(ptr %out) {
entry:
  br label %skip_ws

skip_ws:
  %c0 = call i32 @__inox_con_getc()
  %eof0 = icmp eq i32 %c0, -1
  br i1 %eof0, label %trap, label %ws_check

ws_check:
  %is_sp = icmp eq i32 %c0, 32
  %is_tb = icmp eq i32 %c0, 9
  %is_lf = icmp eq i32 %c0, 10
  %is_cr = icmp eq i32 %c0, 13
  %ws_a = or i1 %is_sp, %is_tb
  %ws_b = or i1 %is_lf, %is_cr
  %is_ws = or i1 %ws_a, %ws_b
  br i1 %is_ws, label %skip_ws, label %first_digit

first_digit:
  %fge = icmp sge i32 %c0, 48
  %fle = icmp sle i32 %c0, 57
  %fis = and i1 %fge, %fle
  br i1 %fis, label %digits, label %trap

digits:
  %ch = phi i32 [ %c0, %first_digit ], [ %next_ch, %digit_ok ]
  %acc = phi i64 [ 0, %first_digit ], [ %add_v, %digit_ok ]
  %dge = icmp sge i32 %ch, 48
  %dle = icmp sle i32 %ch, 57
  %dis = and i1 %dge, %dle
  br i1 %dis, label %digit_body, label %finish

digit_body:
  %d32 = sub i32 %ch, 48
  %d64 = zext i32 %d32 to i64
  %mul_p = call { i64, i1 } @llvm.umul.with.overflow.i64(i64 %acc, i64 10)
  %mul_v = extractvalue { i64, i1 } %mul_p, 0
  %mul_o = extractvalue { i64, i1 } %mul_p, 1
  %add_p = call { i64, i1 } @llvm.uadd.with.overflow.i64(i64 %mul_v, i64 %d64)
  %add_v = extractvalue { i64, i1 } %add_p, 0
  %add_o = extractvalue { i64, i1 } %add_p, 1
  %ovf = or i1 %mul_o, %add_o
  br i1 %ovf, label %trap, label %digit_ok

digit_ok:
  %next_ch = call i32 @__inox_con_getc()
  br label %digits

finish:
  %e_eof = icmp eq i32 %ch, -1
  %e_sp = icmp eq i32 %ch, 32
  %e_tb = icmp eq i32 %ch, 9
  %e_lf = icmp eq i32 %ch, 10
  %e_cr = icmp eq i32 %ch, 13
  %e_a = or i1 %e_eof, %e_sp
  %e_b = or i1 %e_tb, %e_lf
  %e_c = or i1 %e_a, %e_b
  %e_ok = or i1 %e_c, %e_cr
  br i1 %e_ok, label %store, label %trap

store:
  store i64 %acc, ptr %out
  ret void

trap:
  call void @__inox_arith_fault(i32 6)
  unreachable
})llvm";
}

} // namespace inox::compiler::codegen::detail
