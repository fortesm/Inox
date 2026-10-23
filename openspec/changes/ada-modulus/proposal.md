## Why

With `/` as in Ada (r3.95), Inox's `mod` still had the sign of the dividend
(`-7 mod 2` was -1): Ada's `rem` and Delphi's `mod`. Ada's `mod` has the
sign of the divisor, which keeps `X mod N` in 0..N-1 for a positive N (a
weekday, a clock, a ring buffer) whatever the sign of X. The open point of
change `integer-slash` asked whether to follow Ada (`mod` as in Ada and a
new `rem`).

Approval (Marcelo Fortes, 2026-10-23): "Sim, siga a regra de Ada para o
modulus".

Risk: normal (`mod` changes value when the operands have different signs;
with non-negative operands, as every use in the repository, nothing
changes).

Status: **implemented** (canon r3.96).

## What Changes

1. **`mod`.** `A mod B` has the sign of B: `-7 mod 2` is 1, `7 mod -2` is
   -1. It lowers to the checked `srem` corrected by B when the signs differ.
2. **`rem`.** New reserved word (42 again): `A rem B` has the sign of A,
   `-7 rem 2` is -1, the remainder of `/`: A = (A / B) * B + A rem B.
3. **Both.** A constant zero divisor is a compile error, a run-time one
   traps; `Int64.Min mod -1` and `Int64.Min rem -1` are 0; on UInt64 they
   are the unsigned remainder; decimals take neither.
4. **Tests.** div-mod-signs and integer-slash print both; new mod-rem-edges,
   rem-by-zero (trap), const-rem-zero, decimal-rem, rem-reserved.
   calc_differential checks both against Python (`%` and the truncated
   remainder); mutation_fuzz knows `rem`.
5. **Law and docs.** Constitution CANON-2, CANON-8 (new requirement
   "Modulus and remainder"), CANON-19, CANON-20, CANON-23; capability specs
   decimals and fixed-width-integers; grammar; site, README, CHANGELOG,
   STATUS.
