## ADDED Requirements

### Requirement: Instants, dates and durations
A `TDateTime` SHALL be the microseconds since 0001-01-01T00:00:00 of the
proleptic Gregorian calendar, to 9999-12-31T23:59:59.999999; a `TDate` the
days since 0001-01-01; a `TDuration` microseconds (D1). They are Ranges of
Integer: compared with `= # < <= > >=`, stored checked ("value out of
range").

#### Scenario: Ends of the calendar
- **WHEN** a program encodes 0001-01-01 and 9999-12-31 and asks their weekdays
- **THEN** they are days 0 and 3652058, a Monday and a Friday (tests/runtime/datetime.inox)

### Requirement: Encoding and decoding
`EncodeDate(Y, M, D)` and `EncodeDateTime(Y, M, D, H, N, S)` SHALL check
their parts with preconditions (a day that does not exist stops the
program); `DecodeDate`, `Year`, `Month`, `Day`, `Hour`, `Minute`, `Second`,
`MicroSecond`, `DayOfWeek` (a `TDayOfWeek`, Monday first), `DateOf`,
`DateTimeOf`, `IsLeapYear` and `DaysInMonth` read them back (D3).

#### Scenario: A date and a wrong one
- **WHEN** a program encodes and decodes 2026-10-22 05:57:30, then encodes 2026-02-29
- **THEN** every part comes back and it is a Thursday (tests/runtime/datetime.inox); the wrong date breaks a precondition (tests/runtime/datetime-invalid.inox)

### Requirement: Arithmetic and the clock
`AddDays`, `AddSeconds` and `DaysBetween` (whole days, either order)
SHALL compute with instants; `NowUTC`, `Now`, `ToLocal` and `ToUTC` know
two zones, UTC and the system's (D2).

#### Scenario: Days and zones
- **WHEN** a program adds 70 days, measures them both ways and converts a winter instant to local time and back
- **THEN** it gets 2026-12-31, 70 and 70, and the same instant (tests/runtime/datetime.inox)

### Requirement: Text
Std.DateUtils SHALL give `ToISO8601(T)`, `FromISO8601(S)` (date, optional
time, seconds, fraction and `Z`; other text or a date that does not exist
raises ConversionError) and `FormatDateTime(T, Pattern)` with Delphi's
codes `yyyy yy mm m dd d hh h nn n ss s zzz` and quoted text (D3).

#### Scenario: Formats
- **WHEN** a program writes instants in ISO 8601 and three patterns, and reads valid and invalid texts
- **THEN** each text is as stated and the invalid ones raise ConversionError (tests/runtime/dateutils.inox)

### Requirement: Visible with Use
The names of Std.DateTime SHALL exist only in a program that uses it or
Std.DateUtils; as no name may be shadowed (CANON-10), such a program does
not name its own variables Year, Month, Day, Hour, Minute or Second.

#### Scenario: Without the Use
- **WHEN** a program calls NowUTC without the Use
- **THEN** it is rejected (tests/diagnostics/datetime-without-use.inox)
