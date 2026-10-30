## Why

Std.DateTime named its parameters with single letters (`EncodeDate(Y, M,
D)`, `EncodeDateTime(Y, M, D, H, N, S)`). Decided by Marcelo Fortes,
2026-10-30:

- parameters follow the order of ISO 8601 (year, month, day) with full
  names, as in Delphi and Ada;
- ISO 8601 is the format of exchange;
- `dd/mm/yyyy` and other regional formats are available through
  `FormatDateTime`, never as a default.

Full names collide with the functions that read a part of a date
(`Year(T)`, `Month(T)` ...), because a parameter cannot hide a routine.
Of the three ways offered, he chose Delphi's DateUtils: those functions
take the suffix `Of`.

Risk: normal (names in one library module and its tests).

Status: **implemented** (canon r3.105).

## What Changes

1. `EncodeDate(Year, Month, Day)`, `EncodeDateTime(Year, Month, Day, Hour,
   Minute, Second)`, `DecodeDate(T, ref mut Year, ref mut Month, ref mut
   Day)`, `IsLeapYear(Year)`, `DaysInMonth(Year, Month)`,
   `DateTimeOf(Date)`.
2. `Year`, `Month`, `Day`, `Hour`, `Minute`, `Second`, `MicroSecond`
   become `YearOf`, `MonthOf`, `DayOf`, `HourOf`, `MinuteOf`, `SecondOf`,
   `MicroSecondOf`; Std.DateUtils and the tests use them.
3. The spec `stdlib/datetime` states the order and names, and a new
   requirement: ISO 8601 for exchange, regional formats only on request.
   The manual says the same.
4. The open question of mixing Range types is recorded in revisions.md.
