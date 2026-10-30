## Why

Inox uses `|` only between `case` alternatives and does not use `{ }` at
all (CANON-2; `{ }` as block delimiters is refused by "The idea of the
syntax"). The maintainer wants both to carry a meaning that makes Inox
easier for the programmer and for the compiler, and that sets it apart
from other compiled languages. Marcelo Fortes, 2026-10-16: a convenience
that sets the language apart; these are speculations and brainstorming,
nothing is decided.

This change records the brainstorm (a conversation of the maintainer with
another assistant, passed on 2026-10-16, and this session's analysis) so
nothing is lost. **Nothing here is decided** (G4): until the maintainer
approves a design, the law stands, including FUTURE Concurrency ("the
`|...|` capture idea is not decided").

Status: exploration, no design. Risk: high (syntax of closures,
concurrency and generics).

## Already settled by the maintainer

- **No interpolation in Strings** (2026-10-16): no `{variable}` inside a
  String as in Ruby; Inox keeps the Pascal style, which needs no extra
  symbol.
  `PutLn("Total: ", X)` stays the way to mix text and values.
- **Same look as Ruby's `|x|`, different meaning**: the same visual
  construction with another semantics. Ruby's
  `|x|` declares block parameters (`[1, 2].each do |x| ... end`); copying
  that would add little.

## Ideas on the table

Written below in today's Inox (`do`, not `go`; `Move` and `Link` of
CANON-25, not `move`/`borrow`/`copy` words; `<T>` generics); the
brainstorm's own spellings were illustrative.

### `| |`: capture clause

`|...|` after the header of a block that runs somewhere else (a `do`
task, later a closure) says how each outer value enters it:

```
do |Move(Order), Link(Log), Attempts|
    Send(Order, Attempts)
    Log.Write("sent")
;
```

- a plain name is a copy (`Attempts`);
- `Move(X)` gives an owner to the block; using X afterwards is the
  compile error of CANON-25;
- `Link(X)` is a checked reference, never dangling.

The items reuse the built-ins of CANON-25, so the clause adds no reserved
word. The compiler learns every crossing of the boundary from the source
instead of inferring it, and a value not listed cannot be used inside.
Later, for closures (change `routine-types`): `|Link(Bank)| (Id Integer)`,
where `( )` are the arguments of each call and `| |` what the closure
keeps from where it was made. The brainstorm advises against using `| |`
for block-local variables or as a second parameter list (a syntactic
catch-all).

### `{ }`: two strong candidates, at most one

- **Effect clause**: what a routine may do outside its parameters,
  from a small closed set (`Load(Path String) {IO} File :`). It
  generalizes the read-only rule of contracts (CANON-24) and tells a
  future `forall`/`do` which calls are safe to run in parallel.
- **Requirement clause**: a static condition on types, checked at compile
  time, for generics (`Max<T>(A T, B T) T {T ordered} :`): a structural
  requirement, never a nominal trait or interface (FUTURE Generics). It
  answers decision 1 of `generics` (constraints) if constraints come.

Rejected in the brainstorm: `{ }` for blocks or bare scopes; using it as a
general attribute list. A third reading of the maintainer's "specialized
parametrization" is static value parameters (`TBuffer{256}`, like Ada generic
formal objects); generics with `<N Integer>` may cover it.

Weaker: Set literals `{A, B}` (CANON-13 says `[A, B]`), map literals.

### Annotations, separate from both

`@Test`, `@Deprecated("Use NewValue")`, later `@Serializable`, `@Property`
(GUI designers, ORMs, test runners): metadata for tools and libraries, so
that frameworks do not need new reserved words. Rules from the
brainstorm: what changes the meaning of a program belongs to the language
(`ref mut`, `Move`, `pre`/`pos`), never to an annotation; each annotation
says where it applies and whether it survives to run time (most do not, so
no RTTI is needed to start); beware of stacks of annotations hiding
behavior (Java frameworks). Delphi has the same idea as custom attributes
(`[Serializable]`, since Delphi 2010).

## Questions for the maintainer

1. `| |` as the capture clause of `do` (and later closures), with items
   `Name`, `Move(X)`, `Link(X)`?
2. `{ }`: effect clause, requirement clause, static value parameters, or
   kept reserved until generics and concurrency show the need?
3. Annotations with `@`, and when (before or after RTTI)?
