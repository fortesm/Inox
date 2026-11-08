## 1. Links

- [ ] 1.1 Link to a value parameter; decide Links to `ref`/`ref mut` parameters (needs approval)
- [ ] 1.2 Escape analysis: linked locals stay on the stack when no Link escapes
- [ ] 1.3 Remove generation checks proven unnecessary

## 2. Owners

- [x] 2.1 Move analysis over `leave` and `continue` (with `until`, handlers, `ensure` and `Retry`; change `owner-moves-across-jumps`, r3.134); the backstop check stays while 2.4 is open
- [x] 2.2 Free owners and linked locals declared in a loop body at the end of each iteration and on every path out of it (change `loop-body-lifetime`, r3.137); Strings keep their release (approved 2026-11-08)
- [x] 2.3 Free an owner returned by a function called as a statement: the requirement predates r3.88 (a function called as a statement is a compile error) and r3.93 (`_ := F(X)`); the discard frees the owner result at once (`_ := NewNode(7)`, `tests/runtime/discard.inox`, string leak check on main 7401b1a); a call that raises produces no result to discard, and the release of what it already built during unwinding is not covered by that test; reconciled 2026-11-08
- [x] 2.4 Priority, before `Option<T>`, `Result<T,E>` and `Vector<T>`: the `ref mut` call edge of the move analysis (change `owner-refmut-call-edge`, r3.135): a routine gives a `ref mut` owner back before each normal exit; a `ref mut` owner variable counts as given away for the handlers and `ensure` of the caller's trys; the field path is item 2.5
- [x] 2.5 An `Own<T>` field passed as `ref mut` to a routine that frees it and raises: the routine gives a `ref mut` owner back before an exception leaves it (change `owner-refmut-exception-exit`, r3.136); trap 17 stays, checked by an emission test and a native fixture

## 3. Contracts

- [x] 3.1 Check struct invariants after a write through `Own` or `Link` (change `invariants-through-pointers`, 0.4.1.3.138): the struct a pointer points at is checked on field and element writes, `Value(P)` writes and method calls through it

## 4. With generics

- [ ] 4.1 `Own`, `Link`, `Option` as generic types of the type representation; `Option<T>` for any T; `Vector<Own<T>>`

## 5. Each task

- [ ] 5.1 Tests (runtime, trap, diagnostic), leak check, ASan+UBSan, MSVC IR; STATUS and design.md of `safe-pointers` updated
