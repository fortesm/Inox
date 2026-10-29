## Lowering

```
  %c = <condition>
  [%m = <message>]                       ; evaluated in order, before the test
  br i1 %c, label %assert.okN, label %assert.failN
assert.failN:
  call void @__inox_arith_fault(i32 14)  ; or @__inox_assert_message(ptr %m)
  unreachable
assert.okN:
```

`__inox_assert_message` is emitted only by a module that needs it: it
flushes the program's output (`fflush(NULL)`), writes the prefix
`Inox runtime error: assertion failed: `, the message bytes read from the
String block `{ count, length, bytes }` and a line break to file descriptor
2 through the native writer (`write` / `_write`, with the length narrowed
to 32 bits on Windows), and calls `_exit(70)`. Both functions are
`noreturn nounwind`, so an Assert inside `try` needs no `invoke`.
