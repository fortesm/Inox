## Why

Object Pascal's `case` takes ordinals only, and so does Ada's (a discrete
selector). A `case` on text (commands, menus, protocol words) otherwise
needs a chain of `if`/`elif`. Asked whether Inox should go beyond Ada, the
maintainer accepted a String selector with strict rules and kept Floats out.

Approval (Marcelo Fortes, 2026-10-19), on the proposal (String: literals and
String Consts only, no ranges, `else` always, a repeated String an error,
exact byte comparison; Float: no, equality of reals is unsafe): approved. On Ada's `others`: Inox's `else` is that arm (r3.74).

Risk: normal (analysis and lowering of one statement form).

Status: **implemented** (canon r3.85).

## What Changes

- A `case` selector may be a String. Each choice is a String literal or a
  String Const; several share an arm with `|`.
- Choices compare byte for byte, as `=` does (case and accents count; for a
  case-insensitive choice, normalize the selector, e.g. `LowerCase(S)`).
- A range of Strings, the same String twice (also through a Const), a
  variable or a value of another type as a choice, and a String case
  without `else`, are compile errors.
- A Float selector stays an error (`case-float-selector` replaces the old
  `case-string-selector` test, whose program is now valid).

## Capabilities

### Modified Capabilities
- `language/case-statement`: String selectors.

## Impact

- Analyzer: `analyzeStringCase`, `staticStringValue`;
  `support/StringLiteral.h` decodes a literal as the code generator writes
  it, so two spellings of one String are one choice.
- Emitter: `emitCase` compares a String selector with `__inox_str_equal`.
- Constitution CANON-12, `language/case-statement`, revisions.md, site,
  CHANGELOG.md, docs/STATUS.md, docs/ROADMAP.md.
