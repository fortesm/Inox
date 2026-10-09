# Diagnostic tests

Each `NAME.inox` must be rejected by `inox --emit-llvm` with a non-zero exit code, and the
compiler's error output must contain the text stored in `NAME.err`. Unlike
`tests/invalid`, these tests check the *reason* for the rejection.
