---
paths:
  - "src/compiler/lexer/**"
  - "src/compiler/scanner/**"
  - "src/compiler/parser/**"
  - "src/compiler/ast/**"
  - "src/compiler/normalization/**"
---

# Inox frontend: apply the existing law, do not invent syntax

- Authority: `docs/openspec/specs/constitution/spec.md` (CANON-20 and relevant syntax clauses), `docs/grammar/grammar.ebnf`, `docs/ARCHITECTURE.md` (phases 1, 2, 2b), E11/E14/E18. Read only the relevant clauses for the edited construct; do not duplicate the whole constitution here.
- The parser records the AST **as written**; desugaring belongs exclusively in `normalization/AstNormalizer`. No type or name resolution in lexer/parser/normalization.
- Use `ast::TypeExpr` structurally; never spell a type and parse it back or compare its textual spelling. Avoid large `TypeExpr` locals in recursive frames; Windows stack budget matters.
- Before changing grammar or keywords, obtain an approved OpenSpec syntax decision, update the grammar mirror and a valid plus invalid parser/diagnostic case. Preserve `:` for routine bodies, `;` for block closure and `==` comments.
- Diagnose at source line/column. Do not guess grammar from Rust/C++/Ada/Pascal.
