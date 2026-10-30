## Why

`docs/ROADMAP.md` kept the 0.1.x–0.9.x plan of 2026-06, written before the
version scheme of r3.60 (Inox is 0.2.0 pre-alpha, with arrays, Strings and
files already done). The maintainer decided (Marcelo Fortes, 2026-10-12):
the roadmap is old; renumber it.

Risk: normal (documentation).

## What Changes

- `docs/ROADMAP.md`: 0.1 done; 0.2 the imperative core (what is left:
  UInt64, Enum/Range/Set, Char values, Float conversions, Std.Strings);
  0.3 decimals, dates, system utilities, Assert, final console I/O; 0.4
  ownership (Vector, Option, Result); 0.5 modules; 0.6 backend objects and
  quality gates; 0.7 contracts and protocols; 0.8 concurrency; 0.9 alpha and
  beta; 1.0 stable. Later MINORs may still move; the 1.0 scope is the
  maintainer's.
- Constitution `engineering.md` E22 lists the backend steps in order without
  version numbers, pointing to the roadmap; relock.
