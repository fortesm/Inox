# Inox platform support: bsd

This directory is reserved for platform-specific support code for the Inox compiler.
Do not add platform conditionals to parser, AST, semantic analysis, diagnostics, or ordinary backend logic.
When this platform is validated on a real host/runner, move the relevant implementation here or into the common POSIX/Windows support layer as appropriate.
