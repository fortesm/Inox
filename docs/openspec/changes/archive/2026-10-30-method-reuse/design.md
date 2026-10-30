## Decisions (approved by Marcelo Fortes, 2026-10-13: 1a, 2a, 3b, 4a+b)

1. **Declaring reuse, 1a**: `TSubscription Struct (TPerson)`, Delphi-like; the
   first line says what is reused; several parts are allowed:
   `TClient Struct (TPerson, TContact)`. (Rejected: a bare type line, Go;
   a `Reuse TPerson` line.)
2. **Self in a reused method, 2a (delegation)**: the method runs on the
   part, so it behaves the same wherever it is reused; `TPerson.Show`
   calling `Self.Title` calls `TPerson.Title` even when `TSubscription`
   overrides `Title`. (Rejected: rebinding Self to the outer struct, the
   static "virtual" of the ChatGPT discussion.)
3. **A method with a reused name, 3b**: allowed only with `Override`, which
   replaces it for callers of the struct; the reused one stays reachable by
   its path (`A.TPerson.Title`). (Rejected: forbidding it.)
4. **Operations on a field, 4a + 4b**: methods of built-in types
   (`String.UpdateStr`, Delphi helpers) and routines with `ref mut`
   parameters (`UpdateStr(A.FPlan, "x")`), both allowed.

## Rules

- A part is a field named after its type. Construction names the part:
  `TSubscription(TPerson := P, FPlan := "x")`; a reused field in a
  construction is an error that says so.
- The members a struct offers: its fields, its methods, and for each part,
  the part's fields, methods and offered members (a part's own `Override`
  hides what it replaces). A member is reached by exactly one path.
- At the struct's declaration it is an error when: a part is not a struct;
  a name is reached by two paths (resolved for methods by an `Override` in
  the struct, otherwise by turning a part into a named field); a field of
  the struct has a reused name; a method has a reused name without
  `Override`, or the name of a reused field; an `Override` replaces nothing
  or changes the signature (parameters after `Self`, their modes, the
  result, and `Self` or `Self mut`). A struct that reuses itself, even
  indirectly, contains itself.
- The analyzer rewrites `A.FName` and `A.UpdateName(X)` to the explicit path
  (`A.TPerson.FName`); codegen sees ordinary field access and static calls.
- No subtyping: a `TSubscription` is never passed where a `TPerson` is
  expected; `A.TPerson` is the part.
- A method of a built-in type belongs to that exact type (an Integer
  method is not a Byte method); `Self` is a copy, `Self mut` the caller's
  variable, which must be writable. Methods of arrays and File do not exist.

## Not in this change

Rebinding Self, protocols or interfaces as nominal entities, member
mapping between differently named fields, dynamic dispatch. Polymorphism
over different types waits for variants (FUTURE) and generics.
