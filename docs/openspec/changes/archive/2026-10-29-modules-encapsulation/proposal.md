## Why

Until 0.3 every declaration of a module was public, and a module saw
everything its dependencies saw. Names of different modules shared one
space, so two libraries could not declare the same name, and any module
could write any field of any struct: a struct invariant (CANON-24) caught
a bad write only after the fact. The ROADMAP puts modules and
encapsulation in 0.4, before collections. This change also answers the
open questions of the `encapsulation` change (2026-10-14), which it
replaces.

The design followed Ada/SPARK, Rust, modern Delphi and Go: privacy per
module, private by default, nothing public exposing something private, and
a compiler that never resolves an ambiguity on its own.

Risk: normal (the front end resolves names and checks visibility; the
generated code and the runtime ABI do not change).

Status: **implemented** (canon r3.99).

## Approvals (Marcelo Fortes)

- 2026-10-26: private by default, `Public` marks what other modules see
  through `Use`; `Use` is not transitive; `Export` only as `Export Use M`,
  which passes M's public names on without changing them.
- 2026-10-29, header syntax: `Public` is the last word of a header, after
  the result type and `Override`: "Correto e a sintaxe correta. Se
  retornasse um valor (no caso uma string), seria:
  TPerson.UpDateName(Self mut, Name String) String Public :", then
  "Xonfirmado". The struct model `TPerson Struct(TBeing) Public`, private
  fields, then a `Public` line: "Está correto". Line breaks: "São
  equivalentes e não uma imposição de sintaxe. A quebra de linha em inox
  só influencia para identificar onde termina uma instrução de uma linha
  exatamente igual a Ruby".
- A1 (own names first, two imported names an ambiguity error): "Esse é um
  problema clássico de ambiguidade. Na parte 1, aceito sua recomendação,
  deve ficar como Delphi e Rust... Na parte dois, também acato sua regra
  proposta".
- A2 (qualified names): "Acatada as proostas".
- A3 (aliases): "Usaremos Bank.Accounts as Accounts como apelidos, já que
  temos uma hierarquia mais segura que Delphi, como você explicou."
- A4 (selective use): "`use bank.accounts.(deposit, withdraw)`".
- B3 (State never Public), B4 (`inox interface`), Const Public: approved.
- C1 to C4: approved "se não for contra a proposta da linguagem de alta
  segurança como Rust , Spark/Ada e Ada" (they follow Rust and Ada/SPARK);
  C5: "C5 para depois".
- D1 (search order): approved, with "futuramente teremos uma ferramenta de
  buscar e baixar pacotes". D2 (`Std` reserved): "mediante isso acato sua
  proposta".
- E1: "um PR só com um commit por etapa aprovado"; E2: the bank in several
  files (`examples/bank/`) as the acceptance program, approved.

## What Changes

1. **Names (A1, A2, A3, A4).** A pass before contract lowering
   (`semantic/ModuleResolution`) gives each declaration of a used module an
   internal qualified name, resolves every name a module writes (own first,
   then the one public name among its uses; an ambiguity is an error naming
   the qualified forms), qualified names, aliases (`Use M as A`, local to
   the file; `Export Use ... as` is an error) and selective uses (`Use
   M.(X, Y)`). Diagnostics show the names as written.
2. **Visibility.** `Public` (a reserved word) at the end of the headers of
   types, routines, methods and Consts; the `Public` line of a struct;
   State never public; `Export Use`. Extra rules the design needed, in the
   spirit of Ada and Rust: a method is declared in its type's module; a
   private type has no public method or field; a public struct does not
   reuse a private one; a public routine does not name a private type.
3. **Encapsulation (C1 to C4).** The analyzer checks field and method
   visibility, construction of a type with a private field without default
   (also bare declarations and `New`), reuse across modules, and keeps the
   invariant check after outside writes to public fields.
4. **Search path and Std (D1, D2).** `--path <dir>` (repeatable),
   `INOX_PATH`, then the standard library; `Std` names only the library.
5. **`inox interface` (B4).** Prints a module's public surface as Inox.
6. **Acceptance (E2).** `examples/bank/`: Main uses Bank, which re-exports
   Bank.Accounts and Bank.Reports; tests prove FBalance is out of reach.
7. **Library.** Each `Std` module marks its public declarations; the pairs
   use `Export Use` (`Std.StrUtils`, `Std.IOUtils`, `Std.DateUtils`).
8. **Fix found on the way.** A method argument is analyzed against its
   parameter type, like a routine argument (`A.Deposit(500.00)` with a
   Currency parameter was rejected).
9. **Law and docs.** CANON-1, CANON-2, CANON-3 (rewritten), CANON-18,
   CANON-23, CANON-24, FUTURE; grammar; site, CHANGELOG, STATUS, ROADMAP,
   stdlib README, AGENTS.md.

## Later (recorded, not designed)

- Read-only fields (C5).
- The package tool, whose folder joins the search order (FUTURE).
- `inox interface` names a module by its file's stem; a nested file
  (`Bank/Accounts.inox`) shows as `Accounts`.
