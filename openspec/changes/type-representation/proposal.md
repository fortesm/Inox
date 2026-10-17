## Why

Before generics, the compiler must stop carrying types as text. Today a type
is a string such as `"Array[1..5] TContato"` or `"Own<TNo>"`: the analyzer
and the code generator each re-read it with their own small parsers
(`support/ArrayType`, `support/PointerType`), compare it with literals
(`== "Int64"`, `== "String"`), and keep separate lists of the same facts
(the integer ranges are written in three places). The code generator also
re-parses routine signatures from their tokens and rebuilds struct layouts
from the AST, duplicating the analyzer. Generic types (`TPar<String,
Own<TNo>>`) and the substitution of `T` would make this fragile.

Marcelo Fortes, 2026-10-17: "Vamos seguir o plano. Primeira etapa Primeiro
prepara o compilador com a refatoracao. Tipos de verdade, a regra é clara e
simples mapear os tipos que o llvm já oferece. Clonar comandos. Code gen lê
os tipos da análise. Lembre-se use as boas práticas. Evite erros clássicos
de arquitetura e implementação de compiladores. Não repita erros e más
decisões de outras linguagens o código-fonte não pode ter alta acoplagem,
não pode ser propenso à regressões, prefira isolar features de forma
modular que grandes monólitos. Só finalizar partimos para etapa 2".

Status: approved (above). Risk: high (rewrites how a compiler phase
represents types). No language change: every program compiles to the same
LLVM IR, the same types and the same diagnostics, proven with
`tools/ir_snapshot.py` against a snapshot recorded before the change.

## What Changes

- A new module `src/compiler/types/`, depending on nothing else in the
  compiler: interned type objects whose kinds are the ones LLVM offers
  (integer, floating point, pointer, array, struct), with the Inox meaning
  on top (Natural's range, a String is a pointer to a runtime block, a Link
  is a struct of three values). Type identity is pointer identity.
- One reader of written type syntax (`Array[...]`, `Own<T>`, ...) replaces
  the two string helpers.
- The analyzer works with type objects; it records them per expression and
  per declaration, and exports routine signatures and struct layouts.
- The code generator reads those types, signatures and layouts instead of
  re-deriving them from text, tokens and the AST.
- `ast::cloneStatement`, the counterpart of `cloneExpression`, for the
  routine instances of generics.
- A compiler unit-test executable (`inox-compiler-tests`) for the modules
  that can be tested without a whole program.
