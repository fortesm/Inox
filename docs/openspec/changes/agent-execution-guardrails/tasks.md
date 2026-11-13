## 1. OpenSpec execution contract

- [x] 1.1 Record approved workflow changes as OpenSpec proposal, design and test-named engineering-agent-execution delta; leave constitutional law untouched.
- [x] 1.2 Add short `operations.apply.guidance` and focused task/spec rules without duplicating constitution in every agent prompt.

## 2. Claude integration

- [x] 2.1 Add phase-scoped rules that reference existing CANON and E requirements.
- [x] 2.2 Add a non-generated Inox apply skill with task-type-specific evidence and low-token output.
- [ ] 2.3 Add a bounded-result local verification command and Stop hook, with no hardcoded Windows-only environment.
- [x] 2.4 Add verifier self-tests and a user/operator guide documenting explicit NOT_RUN and SKIP.

## 3. Evidence and review

Local evidence for the verifier: `python -m unittest discover -s tests -q`
in the isolated source test folder, **6/6 PASS** on Linux; the Git blob
SHAs of the Python script and its tests match the files tested locally.
This is not evidence of a compiler build, full E19 or Windows execution.
Those remain for CI/review. The generated OpenSpec apply skill was not edited.


- [ ] 3.1 Review changed files for G1/G4/G7/G9 and E11/E16/E19/E22 alignment, no unrelated changes, no compiler semantics changed.
- [ ] 3.2 Validate OpenSpec YAML and JSON syntax, self-tests and `openspec validate --all --strict` where the CLI is available.
- [ ] 3.3 Preserve full logs and summarize exact checks run, their platform, results, and anything NOT_RUN; do not claim Windows/Linux compiler gates without actually executing them.
- [ ] 3.4 Open a PR against `main`, request independent review and do not merge.
