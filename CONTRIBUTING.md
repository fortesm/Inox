# Contributing to Inox

Inox is free software. Contributions to the compiler, standard library, runtime,
tests, examples, tools and documentation are welcome when they preserve the
language constitution and the project's safety-oriented design.

## License of contributions

By contributing to Inox, you agree that your contribution is licensed under the
Mozilla Public License Version 2.0.

Do not contribute code that you do not have the right to license under MPL 2.0.
Do not copy code from projects with incompatible licenses.

## Developer Certificate of Origin

Contributions should follow the Developer Certificate of Origin model. Each
commit should be made with a real identity and should certify that you have the
right to submit the contribution under the project's license.

A suitable sign-off line is:

```text
Signed-off-by: Your Name <your.email@example.com>
```

## Language rules are not optional

The canonical language rules are documented in:

- `docs/decisions/ADR-0006-inox-0.1-constitution.md`
- `docs/canonical/language-reference.md`
- `grammar/grammar.ebnf`
- `AGENTS.md`

Do not reintroduce deprecated or forbidden syntax such as `End`, `then`, `do`,
`of`, `Var :`, `Type :`, empty parentheses in declarations or calls, or `/` as
Integer division.

## Testing

Before submitting changes, run:

```powershell
cmake --build build --config Debug
pwsh -ExecutionPolicy Bypass -File .\scripts\run-tests.ps1
```

On Unix-like systems, run the equivalent CMake build and:

```sh
bash ./scripts/run-tests.sh ./build/inox
```
