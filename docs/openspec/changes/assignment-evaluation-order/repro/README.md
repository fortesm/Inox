The reproductions of change `assignment-evaluation-order` are runtime tests
now (`tests/runtime/`, named in CANON-5 and CANON-25).
`reference-argument-owner-replaced.inox` shows the same defect for a
reference argument (`Store(H.FChild.FV, Drop)` prints `1 10`); it moves to
the change of reference arguments (decision Q1), before F6 step 4.
