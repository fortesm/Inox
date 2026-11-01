## 1. Parser and tests

- [x] 1.1 `rejectReceiverType`: typed receiver and `Self owned`
- [x] 1.2 Tests `receiver-with-type`, `receiver-owned-reserved`

## 2. Documents

- [x] 2.1 CANON-9 scenario (relock), STATUS, site, revision r3.116, kCanonRevision, CHANGELOG

## 3. Gates

- [x] 3.1 Suite, IR snapshot against main (no difference: no test had this mistake), openspec, constitution_check, version_check
- [x] 3.2 IR snapshot: a program the change adds is not compared (fix to r3.115); self-test 13 checks; CI rehearsal: 1055 programs, 0 unexpected
- [x] 3.3 CI
