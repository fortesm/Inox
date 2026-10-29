## 1. Links

- [ ] 1.1 Link to a value parameter; decide Links to `ref`/`ref mut` parameters (needs approval)
- [ ] 1.2 Escape analysis: linked locals stay on the stack when no Link escapes
- [ ] 1.3 Remove generation checks proven unnecessary

## 2. Owners

- [ ] 2.1 Move analysis over `leave` and `continue`; drop the backstop check where it is proven
- [ ] 2.2 Free owners and linked locals declared in a loop body at the end of each iteration
- [ ] 2.3 Free an owner returned by a function called as a statement (with that backend gap)

## 3. Contracts

- [ ] 3.1 Check struct invariants after a write through `Own` or `Link`

## 4. With generics

- [ ] 4.1 `Own`, `Link`, `Option` as generic types of the type representation; `Option<T>` for any T; `Vector<Own<T>>`

## 5. Each task

- [ ] 5.1 Tests (runtime, trap, diagnostic), leak check, ASan+UBSan, MSVC IR; STATUS and design.md of `safe-pointers` updated
