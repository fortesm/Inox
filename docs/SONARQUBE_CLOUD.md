# SonarQube Cloud (Free): automatic analysis for Inox

This integration is **independent of the existing E19 quality gates**. It adds
code-analysis feedback for the C++20 compiler and runtime; it does not replace
or change `.github/workflows/ci.yml`, `.github/workflows/gates.yml`, the
OpenSpec checks, sanitizers, fuzzing, or the other project quality gates.

## Chosen method

Use **SonarQube Cloud Automatic Analysis**, not a GitHub Actions SonarScanner
job. SonarQube Cloud supports automatic C/C++ analysis for GitHub repositories
without a build command, toolchain, compilation database, or `SONAR_TOKEN`.
The Free plan can analyze a public repository with its built-in Sonar way
quality profile and gate; custom profiles and gates require a higher plan.

The root `.sonarcloud.properties` is the configuration file for **automatic**
analysis. **Do not add `sonar-project.properties` or a SonarScanner workflow**
while Automatic Analysis is enabled. SonarQube Cloud does not support using
automatic analysis and CI-based scanning simultaneously for one project.

## Initial analysis scope

The root `.sonarcloud.properties` sets:

```properties
sonar.sources=src,include,tools
sonar.sourceEncoding=UTF-8
sonar.cfamily.reportingCppStandardOverride=c++20
```

- `src/`: the C++ compiler and runtime implementation.
- `include/`: public C++ headers.
- `tools/`: supporting Python and C++ project tooling.
- `tests/`, `examples/`, `stdlib/` and `docs/` are outside the **initial**
  source scope. Inox `.inox` files are not understood as C++ by SonarQube
  Cloud. Tests remain subject to the existing E19 CI gates; reconsider
  including C++ test sources in Sonar after evaluating the initial report.
- Build outputs are outside these source directories and remain excluded.

The C/C++ analyzer does not currently support the `sonar.tests` setting, so
this initial integration does not pretend that C++ test sources are classified
or measured as tests. Automatic Analysis does **not** import coverage or
external analyzer reports.

## One-time activation in SonarQube Cloud (requires account authorization)

1. Open <https://sonarcloud.io/> and sign in with **GitHub**.
2. Create or select your SonarQube Cloud organization bound to the GitHub
   account or organization that owns `fortesm/Inox`. Choose the **Free** plan.
3. Authorize the official SonarQube Cloud GitHub application to access the
   `fortesm/Inox` repository (prefer access to that repository only).
4. Choose **Analyze new project**, select the GitHub repository `fortesm/Inox`,
   and import it as a **public** project. Follow the new-code baseline prompt.
5. Confirm **Administration > Analysis Method > Automatic Analysis** is on.
   The first analysis runs in the SonarQube Cloud service; subsequent pushes
   to `main` and pull requests are analyzed automatically.
6. On the project overview, verify that C++ sources under `src/` were
   discovered, and that SonarQube Cloud reports an analysis of `main`.
   Review any analysis warnings before trusting the findings.
7. After the configuration PR is merged, verify the source scope and C++20
   setting from `.sonarcloud.properties` are reflected in the next analysis.

Do **not** create a `SONAR_TOKEN` GitHub secret for this method. There is no
SonarScanner invocation from GitHub Actions.

## Interaction with the existing GitHub gates

- SonarQube Cloud may publish its own analysis status on pull requests. This
  status is **advisory** initially. Do not make it a required status check in
  GitHub branch protection or repository rulesets without a separate decision.
- The existing `CI` and `Gates` workflows continue unchanged. Sonar's gate is
  separate from the E19 acceptance gates, and a Sonar failure must not be
  interpreted as evidence that an E19 test failed.
- A SonarQube Cloud gate can still be displayed as failing, even when it is
  not a required GitHub check; monitor the dashboard and triage baseline
  issues first.
- If GitHub branch protection already uses a wildcard requiring every
  check, review that rule before enabling SonarQube Cloud so that the new
  check does not unexpectedly block merges.

## Limitations and future refinement

- Automatic analysis does not use Inox's exact Linux Clang or Windows MSVC
  build variants; it chooses its own C/C++ configuration. The explicit
  `c++20` setting helps avoid mismatches in the reported standard but does
  not replicate all compiler flags or platform-specific definitions.
- Do not claim test coverage from the Sonar dashboard in automatic mode;
  use E19 test results instead.
- If deeper analysis with a real compilation database or imported coverage
  becomes important, make a **separate proposal** to move to CI-based
  SonarScanner analysis. First turn Automatic Analysis **off** in SonarQube
  Cloud, then configure a new, isolated CI workflow. Never enable both
  methods simultaneously.
- Avoid modifying `.github/workflows/ci.yml` or `gates.yml` just to integrate
  Sonar; their reproducibility and E19 requirements are independent.

## Official references

- [SonarQube Cloud: Automatic analysis](https://docs.sonarsource.com/sonarqube-cloud/analyzing-source-code/automatic-analysis)
- [SonarQube Cloud: GitHub onboarding](https://docs.sonarsource.com/sonarqube-cloud/getting-started/github)
- [SonarQube Cloud: Quality gate customization](https://docs.sonarsource.com/sonarqube-cloud/standards/managing-quality-gates/managing-custom-quality-gates)
- [SonarQube Cloud: Conflict between automatic and CI-based analysis](https://docs.sonarsource.com/sonarqube-cloud/analyzing-source-code/ci-based-analysis/overview-of-integrated-cis)
