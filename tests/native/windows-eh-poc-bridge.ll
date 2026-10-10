; SPDX-License-Identifier: MPL-2.0
; Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
;
; Native Windows/MSVC EH PoC. Deliberately omits target triple/datalayout so the
; same clang++ installation that builds Inox supplies its concrete MSVC target.

declare i32 @__CxxFrameHandler3(...)
declare ptr @__inox_exception_capture(ptr) nounwind
declare i64 @__inox_exception_type(ptr) nounwind
declare void @__inox_exception_release(ptr) nounwind
declare void @__inox_exception_rethrow(ptr) noreturn

declare void @__inox_raise(i64) noreturn
declare void @inox_poc_report_type(i64) nounwind
declare void @inox_poc_throw_foreign()
declare void @inox_poc_raise_seh()
declare void @inox_poc_foreign_else_marker() nounwind
declare void @inox_poc_seh_bridge_marker() nounwind

define void @inox_poc_foreign_else() personality ptr @__CxxFrameHandler3 {
entry:
  %state.slot = alloca ptr
  store ptr null, ptr %state.slot
  invoke void @inox_poc_throw_foreign()
          to label %unexpected unwind label %catch.dispatch

unexpected:
  ret void

catch.dispatch:
  %cs = catchswitch within none [label %catch] unwind to caller

catch:
  %cp = catchpad within %cs [ptr null, i32 64, ptr null]
  %state = call ptr @__inox_exception_capture(ptr null) [ "funclet"(token %cp) ]
  store ptr %state, ptr %state.slot
  catchret from %cp to label %parent

parent:
  %saved = load ptr, ptr %state.slot
  %type = call i64 @__inox_exception_type(ptr %saved)
  %is.foreign = icmp eq i64 %type, 0
  br i1 %is.foreign, label %else, label %rethrow

else:
  call void @inox_poc_foreign_else_marker()
  call void @__inox_exception_release(ptr %saved)
  ret void

rethrow:
  call void @__inox_exception_rethrow(ptr %saved)
  unreachable
}

define void @inox_poc_foreign_rethrow() personality ptr @__CxxFrameHandler3 {
entry:
  %state.slot = alloca ptr
  store ptr null, ptr %state.slot
  invoke void @inox_poc_throw_foreign()
          to label %unexpected unwind label %catch.dispatch

unexpected:
  ret void

catch.dispatch:
  %cs = catchswitch within none [label %catch] unwind to caller

catch:
  %cp = catchpad within %cs [ptr null, i32 64, ptr null]
  %state = call ptr @__inox_exception_capture(ptr null) [ "funclet"(token %cp) ]
  store ptr %state, ptr %state.slot
  catchret from %cp to label %parent

parent:
  %saved = load ptr, ptr %state.slot
  call void @__inox_exception_rethrow(ptr %saved)
  unreachable
}

define void @inox_poc_seh() personality ptr @__CxxFrameHandler3 {
entry:
  %state.slot = alloca ptr
  store ptr null, ptr %state.slot
  invoke void @inox_poc_raise_seh()
          to label %normal unwind label %catch.dispatch

normal:
  ret void

catch.dispatch:
  %cs = catchswitch within none [label %catch] unwind to caller

catch:
  %cp = catchpad within %cs [ptr null, i32 64, ptr null]
  %state = call ptr @__inox_exception_capture(ptr null) [ "funclet"(token %cp) ]
  store ptr %state, ptr %state.slot
  catchret from %cp to label %parent

parent:
  %saved = load ptr, ptr %state.slot
  call void @inox_poc_seh_bridge_marker()
  call void @__inox_exception_release(ptr %saved)
  ret void
}

; Criterion (v): an Inox exception raised by __inox_raise(42) must be classified
; with its own type id after the funclet bridge, not as foreign (0).
define void @inox_poc_inox_typed() personality ptr @__CxxFrameHandler3 {
entry:
  %state.slot = alloca ptr
  store ptr null, ptr %state.slot
  invoke void @__inox_raise(i64 42)
          to label %unexpected unwind label %catch.dispatch

unexpected:
  ret void

catch.dispatch:
  %cs = catchswitch within none [label %catch] unwind to caller

catch:
  %cp = catchpad within %cs [ptr null, i32 64, ptr null]
  %state = call ptr @__inox_exception_capture(ptr null) [ "funclet"(token %cp) ]
  store ptr %state, ptr %state.slot
  catchret from %cp to label %parent

parent:
  %saved = load ptr, ptr %state.slot
  %type = call i64 @__inox_exception_type(ptr %saved)
  call void @inox_poc_report_type(i64 %type)
  call void @__inox_exception_release(ptr %saved)
  ret void
}
