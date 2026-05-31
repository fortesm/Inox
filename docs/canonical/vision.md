# Inox 0.1 pre-alpha - Vision

This document is the canonical language vision for Inox 0.1 pre-alpha.

## Post-Object-Oriented Language

Inox is post-object-oriented.

Inox does not have classes, classical inheritance, Java-style interfaces,
mixins, duck typing, mandatory class taxonomies, or duplicated method
signatures inside structs.

Inox has structs for data, associated methods declared outside structs, struct
composition, strong static nominal typing, and `Object.Method(args)`
ergonomics without inheritance.

The fundamental principle is explicit separation between data modeling and
behavior reuse. Behavior reuse does not depend on inheritance. Not every
problem is taxonomic.

The language aims to provide reuse, polymorphism, and composition without
classes, inheritance, or classical interfaces. It is deliberately DRY and
averse to boilerplate.

## Structs And Methods

`Struct` declares fields.

`Struct` does not declare or repeat method signatures.

Associated methods are defined outside the struct:

```inox
Type.Method(args) ReturnType :
    ...
;
```

## Future Behavior Abstractions

Future behavior reuse abstractions will be based on contracts, protocols, and
behaviors rather than type hierarchies.

Contracts, protocols, behaviors, callbacks, method references,
visibility/export, property-like syntax, advanced string runtime behavior, and
LLVM implementation work are not implemented as part of this update.

## Consolidated 0.1 Identity

The complete human-readable language overview is maintained in `docs/canonical/language-reference.md` and rendered as `docs/site/index.html`. The identity of Inox 0.1 is now fixed around post-object-oriented programming: structs are data, associated methods are behavior, and future contracts/protocols/behaviors must not recreate classical inheritance or Java-style interfaces.
