## Decisions

- The invariant routine of a generic struct is a generic method of it,
  with the struct's type parameters (Q1 of `generic-struct-methods`), made
  by contract lowering like an ordinary struct's.
- The instance of `__invariant` is made with the struct instance
  (`instantiateInvariant`, out of line), not on a first call: the checks
  of CANON-24 ask whether a type has an invariant by its instance's name,
  and code generation finds it through `Program::invariants`.
- A broken invariant names the struct as written (`TRange`), as a generic
  method's contract does; naming the concrete instance is the diagnostics
  item of the 0.5 order.
