<!-- SPDX-License-Identifier: MPL-2.0 -->
# Canon migration map

Every line of `docs/INOX_CANONICAL.md` belongs to exactly one unit below.
Generated and checked by `tools/canon_trace.py` (run `update` after editing
the canon; CI runs `check`). Statuses: planned, mirrored, copied (verbatim,
checked), approved (law in OpenSpec, G7), duplicate. The canon is retired
when no unit is planned.

| Unit | Lines | Hash | Status | Destination |
|------|-------|------|--------|-------------|
| `PREAMBLE` | 1-2 | `cd5cbdcf026d` | structural | heading only |
| `BANNER-the-single-canonical-document` | 3-58 | `8e90f458e2ca` | planned | openspec/README.md (authority, maintainer, license, map) |
| `SECTION-00` | 59-63 | `a52f0279cfd3` | planned | AGENTS.md (governance) |
| `how-to-read-this-document` | 64-82 | `2da10040fa3b` | planned | AGENTS.md (governance) |
| `governance-rules-that-protect-this-docum` | 83-85 | `d7e7bb760361` | structural | heading only |
| `G1` | 86-90 | `5b3eb650bc3e` | planned | AGENTS.md (governance) |
| `G2` | 91-96 | `7e4916e2f072` | planned | AGENTS.md (governance) |
| `G3` | 97-101 | `bec8720b4966` | planned | AGENTS.md (governance) |
| `G4` | 102-107 | `d3cdf08f12d6` | planned | AGENTS.md (governance) |
| `G5` | 108-111 | `d30cc9be5c2d` | planned | AGENTS.md (governance) |
| `G6` | 112-117 | `a31bc426d896` | planned | AGENTS.md (governance) |
| `G7` | 118-133 | `cab622ba4233` | planned | AGENTS.md (governance) |
| `first-order-engineering-directive-modern` | 134-146 | `3dd9b1a61865` | planned | docs/ENGINEERING.md |
| `first-order-engineering-directive-modern/sources-of-influence` | 147-166 | `bbd91006fa00` | planned | docs/ENGINEERING.md |
| `E1` | 167-176 | `0a4f8d0bf8e6` | planned | docs/ENGINEERING.md |
| `E2` | 177-182 | `9ac4af877fe2` | planned | docs/ENGINEERING.md |
| `E3` | 183-195 | `3b3dc8f81aab` | planned | docs/ENGINEERING.md |
| `E4` | 196-205 | `b5b56a927866` | planned | docs/ENGINEERING.md |
| `E5` | 206-219 | `a1f2655f17a0` | planned | docs/ENGINEERING.md |
| `E6` | 220-228 | `8ea2cd3e52dd` | planned | docs/ENGINEERING.md |
| `E7` | 229-235 | `bc0e80bfa2d2` | planned | docs/ENGINEERING.md |
| `E8` | 236-242 | `c2079e95428c` | planned | docs/ENGINEERING.md |
| `E9` | 243-256 | `5ef640e73e9a` | planned | docs/ENGINEERING.md |
| `E10` | 257-262 | `946045224bbf` | planned | docs/ENGINEERING.md |
| `E11` | 263-289 | `e8a0a5fe161f` | planned | docs/ENGINEERING.md |
| `E12` | 290-305 | `bbc33f2d5937` | planned | docs/ENGINEERING.md |
| `E13` | 306-314 | `df40d606204e` | planned | docs/ENGINEERING.md |
| `E14` | 315-321 | `a4852c4ec3ad` | planned | docs/ENGINEERING.md |
| `E15` | 322-328 | `b573fb6af16e` | planned | docs/ENGINEERING.md |
| `E16` | 329-337 | `5b7e688474bd` | planned | docs/ENGINEERING.md |
| `E17` | 338-353 | `a61f45216f80` | planned | docs/ENGINEERING.md |
| `E18` | 354-360 | `0be9d22f9963` | planned | docs/ENGINEERING.md |
| `E19` | 361-398 | `0db23d3788c7` | planned | docs/ENGINEERING.md |
| `E20` | 399-407 | `3cbd923272e0` | planned | docs/ENGINEERING.md |
| `non-negotiable-rules` | 408-419 | `ace55d5cb612` | planned | AGENTS.md (governance) |
| `SECTION-01` | 420-422 | `8761a2a5882c` | structural | heading only |
| `CANON-1` | 423-431 | `db04e9c70c16` | planned | docs/VISION.md |
| `CANON-1/language-references-and-influences` | 432-450 | `f1408ec493d5` | planned | docs/VISION.md |
| `CANON-1/mission-critical-target-domain` | 451-463 | `453687b19f4a` | planned | docs/VISION.md |
| `CANON-1/core-safety-stance` | 464-478 | `1d21e14e00f1` | planned | docs/VISION.md |
| `CANON-1/post-object-oriented-design` | 479-486 | `184e39a5df22` | planned | docs/VISION.md |
| `CANON-1/performance-model` | 487-496 | `d6a3455f8387` | planned | docs/VISION.md |
| `CANON-1/agent-rule` | 497-503 | `7117205f7492` | planned | docs/VISION.md |
| `SECTION-02` | 504-506 | `b705068ef5b9` | structural | heading only |
| `versioning` | 507-524 | `fe425581df22` | planned | openspec/specs/language (capability to choose) |
| `change-log` | 525-527 | `0fab86654403` | structural | heading only |
| `LOG/r3.59` | 528-566 | `7f8ca7714463` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/r3.58` | 567-595 | `c9679947a1a0` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.57` | 596-616 | `3bc8f8242e98` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.56` | 617-638 | `a64911c477d4` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.55` | 639-669 | `f8b2eae4b70f` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.54` | 670-686 | `156e7b7bd306` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.53` | 687-717 | `7d280ce2326d` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.52` | 718-748 | `a7dbe03eaf28` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.51` | 749-755 | `7e14e9ffa7a9` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.50` | 756-780 | `0441fbff3bf5` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.49` | 781-807 | `be098b1b06bd` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.48` | 808-829 | `afa50d5ec69f` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.47` | 830-842 | `72a0e9ea7e82` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.46` | 843-856 | `07eee37e2b47` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.45` | 857-885 | `e8bf1bc74e25` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.44` | 886-908 | `1ed18d87a1f6` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.43` | 909-918 | `7cec90575743` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.42` | 919-934 | `e00cff8892ed` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.41` | 935-940 | `d9b78ef08a85` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.40` | 941-959 | `3c2103c842ec` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.39` | 960-989 | `ff8a1f06c34f` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.38` | 990-1000 | `d97e13c1b814` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.37` | 1001-1012 | `73f3f0239390` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.36` | 1013-1029 | `889932b86d67` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.35` | 1030-1047 | `a40c544968ff` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.34` | 1048-1060 | `fa31c1e6a95f` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.33` | 1061-1083 | `546628eeb9d8` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.32` | 1084-1105 | `01b8acd407b7` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.31` | 1106-1119 | `c6f79fbad05a` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.30` | 1120-1141 | `82fc2948b1b0` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.29` | 1142-1202 | `411404a57a61` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.28` | 1203-1238 | `f95a9348aa62` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.27` | 1239-1294 | `9a521127b697` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.26` | 1295-1314 | `d412b402bbfb` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.25` | 1315-1327 | `ac5b09d96e50` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.24` | 1328-1350 | `977e3c458a59` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.23` | 1351-1390 | `1bd9020135e7` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.22` | 1391-1405 | `c3cd869369d8` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.21` | 1406-1470 | `fa631814dc07` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.20` | 1471-1499 | `c178428e9e51` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.19` | 1500-1515 | `58a9449923cc` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.18` | 1516-1594 | `63b0850a3b94` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.17` | 1595-1615 | `6b82cfa2bfef` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.16` | 1616-1636 | `504ebe528c6f` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.15a` | 1637-1650 | `8a99974e3b15` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.15` | 1651-1665 | `db45cccf8f29` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.14` | 1666-1676 | `45d2e2e7b0f4` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.13` | 1677-1688 | `e78d99b40dce` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.12` | 1689-1698 | `adf42d99bf9c` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.11` | 1699-1708 | `bb084ddde131` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.10` | 1709-1721 | `bb0c5292dc58` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.8` | 1722-1732 | `e2021267fde4` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.7` | 1733-1743 | `4c50f899f80e` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.6` | 1744-1760 | `f5c6a45807b0` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.5` | 1761-1765 | `1e634d009b80` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.4` | 1766-1771 | `a02b7bf01817` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.3` | 1772-1778 | `b73b838ee71a` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.2` | 1779-1785 | `3aae34cdb8de` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.1` | 1786-1798 | `1c1bfe44fee1` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.0` | 1799-1808 | `01fee319f99c` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v2.2` | 1809-1811 | `947024e547a5` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v2.1` | 1812-1814 | `831353b88b26` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v2.0` | 1815-1820 | `92161592c6f6` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v1.0` | 1821-1822 | `bf9fe6205559` | structural | heading only |
| `DEV-2` | 1823-1859 | `6ff9fac762f9` | planned | docs/RELEASE.md |
| `GROUP-future` | 1860-1868 | `c0c3ec88bb92` | planned | docs/ROADMAP.md |
| `SECTION-03` | 1869-1871 | `8f4b634fd646` | structural | heading only |
| `agent-rules` | 1872-1905 | `f699b67648da` | planned | AGENTS.md (governance) |
| `SECTION-04` | 1906-1910 | `03786d4742d7` | planned | docs/ROADMAP.md |
| `B-OPEN` | 1911-1955 | `4e5f8d3984d1` | planned | openspec/changes (one change per open item) |
| `FUTURE-1` | 1956-1963 | `0a1e1448d12b` | planned | docs/ROADMAP.md |
| `FUTURE-2` | 1964-1968 | `eb05f449bfc6` | planned | docs/ROADMAP.md |
| `FUTURE-3` | 1969-1974 | `2c09d65cfded` | planned | docs/ROADMAP.md |
| `FUTURE-4` | 1975-1978 | `62d8fde09ca2` | planned | docs/ROADMAP.md |
| `FUTURE-5` | 1979-1983 | `041157e3cd11` | planned | docs/ROADMAP.md |
| `FUTURE-6` | 1984-1987 | `a070988b84e8` | planned | docs/ROADMAP.md |
| `FUTURE-7` | 1988-2000 | `116a4980e694` | planned | docs/ROADMAP.md |
| `GROUP-open-questions` | 2001-2024 | `9806de6ee55e` | planned | docs/ROADMAP.md, docs/STATUS.md |
| `BANNER-agent-rules` | 2025-2058 | `1dd939e2384f` | duplicate | `unit:agent-rules` |
| `BANNER-layer-b-implementation-status` | 2059-2079 | `cb181e7e1383` | planned | docs/STATUS.md |
| `SECTION-05` | 2080-2084 | `3950191b4348` | planned | openspec/README.md (history) |
| `ADR-0001` | 2085-2089 | `6597d424187c` | copied | `openspec/changes/archive/2026-05-28-adr-0001-llvm-backend` |
| `ADR-0002` | 2090-2095 | `24673434b56e` | copied | `openspec/changes/archive/2026-05-28-adr-0002-block-syntax` |
| `ADR-0003` | 2096-2099 | `91f21c8db59b` | copied | `openspec/changes/archive/2026-05-28-adr-0003-type-system` |
| `ADR-0004` | 2100-2108 | `a7ea289039f3` | copied | `openspec/changes/archive/2026-05-28-adr-0004-exceptions` |
| `ADR-0005` | 2109-2132 | `b3487abfdaa3` | copied | `openspec/changes/archive/2026-05-31-adr-0005-consolidated-0-1-language-decisions` |
| `ADR-0006` | 2133-2180 | `00635fb17fa5` | copied | `openspec/changes/archive/2026-05-31-adr-0006-inox-0-1-language-constitution` |
| `ADR-0017` | 2181-2206 | `99a0ae3ad662` | copied | `openspec/changes/archive/2026-10-11-adr-0017-read-only-slices` |
| `ADR-0016` | 2207-2238 | `ae224ec3acef` | copied | `openspec/changes/archive/2026-10-11-adr-0016-ref-and-ref-mut-parameters` |
| `ADR-0015` | 2239-2256 | `d138712bb9d2` | copied | `openspec/changes/archive/2026-10-10-adr-0015-storing-into-a-natural-checks-the-range` |
| `ADR-0014` | 2257-2284 | `8f83b1f7aa88` | copied | `openspec/changes/archive/2026-10-10-adr-0014-natural-byte-and-the-closed-numeric-set` |
| `ADR-0013` | 2285-2310 | `8b43a6f9c9e7` | copied | `openspec/changes/archive/2026-10-10-adr-0013-conditional-expression` |
| `ADR-0012` | 2311-2324 | `387bf21df2e0` | copied | `openspec/changes/archive/2026-10-10-adr-0012-compound-assignment` |
| `ADR-0011` | 2325-2357 | `8cd31c3b1d14` | copied | `openspec/changes/archive/2026-10-09-adr-0011-case-in-the-ada-spark-style` |
| `ADR-0010` | 2358-2369 | `01ebe198452d` | copied | `openspec/changes/archive/2026-10-09-adr-0010-step-in-the-for-header` |
| `ADR-0009` | 2370-2385 | `9fb0711acf2b` | copied | `openspec/changes/archive/2026-10-09-adr-0009-chained-assignment` |
| `ADR-0008` | 2386-2403 | `6cbc9717bb6c` | copied | `openspec/changes/archive/2026-10-09-adr-0008-precedence-of-and-in` |
| `ADR-0007` | 2404-2416 | `d8e65f343093` | copied | `openspec/changes/archive/2026-10-09-adr-0007-loop-exit-and-try-cleanup-keywords` |
| `GROUP-development` | 2417-2432 | `6f6455b69203` | structural | heading only |
| `SECTION-06` | 2433-2435 | `1f3fa370b3e3` | structural | heading only |
| `canonical-user-facing-cli-contract` | 2436-2451 | `45e0e5d66ff0` | planned | openspec/specs/compiler/cli |
| `current-observed-driver-status` | 2452-2457 | `15c15b4e82d8` | planned | docs/STATUS.md |
| `SECTION-07` | 2458-2460 | `f2dfbd93d1af` | structural | heading only |
| `source-file-structure` | 2461-2472 | `f905deb42055` | planned | openspec/specs/language/modules |
| `SECTION-08` | 2473-2475 | `83d42d4dfe69` | structural | heading only |
| `CANON-2` | 2476-2507 | `1784cd8f9c86` | planned | openspec/specs/language/lexical |
| `SECTION-09` | 2508-2510 | `151b987c27db` | structural | heading only |
| `CANON-3` | 2511-2531 | `86f97b768bdb` | planned | openspec/specs/language/modules |
| `SECTION-10` | 2532-2534 | `539a6368d8f5` | structural | heading only |
| `CANON-4` | 2535-2590 | `011a17049109` | planned | openspec/specs/language/block-syntax |
| `CANON-7` | 2591-2602 | `954891e7e64b` | planned | openspec/specs/language/block-syntax |
| `SECTION-11` | 2603-2605 | `cc93dd9983b8` | structural | heading only |
| `CANON-5` | 2606-2610 | `107db4bb6efc` | planned | openspec/specs/language/variables |
| `CANON-5/type-section` | 2611-2625 | `ee2b644e002f` | planned | openspec/specs/language/declaration-sections |
| `CANON-5/local-variables-three-valid-forms-only` | 2626-2633 | `7acb630f2600` | planned | openspec/specs/language/variables |
| `CANON-5/grouped-declarations` | 2634-2646 | `211d54982dd6` | planned | openspec/specs/language/variables |
| `CANON-5/compound-assignment` | 2647-2656 | `ac2d6220edcf` | planned | openspec/specs/language/assignment |
| `CANON-5/assignment-is-a-statement-chained-assign` | 2657-2716 | `4a3bbaf10235` | planned | openspec/specs/language/assignment |
| `CANON-5/safe-type-inference` | 2717-2725 | `8cabd03f259a` | planned | openspec/specs/language/variables |
| `CANON-5/const-and-state` | 2726-2735 | `c77d846e37bf` | planned | openspec/specs/language/declaration-sections, openspec/specs/language/module-state |
| `SECTION-12` | 2736-2738 | `21806ebd31bd` | structural | heading only |
| `CANON-8` | 2739-2740 | `44025c9660f2` | structural | heading only |
| `CANON-8/canonical-type-hierarchy` | 2741-2782 | `b64e68e42b5b` | planned | openspec/specs/language/numeric-types, openspec/specs/language/natural, openspec/specs/language/fixed-width-integers |
| `CANON-8/generics-use-angle-brackets` | 2783-2791 | `599f5b0a40cf` | planned | openspec/specs/language/generics |
| `CANON-8/conversions` | 2792-2804 | `233a06f0e0e2` | planned | openspec/specs/language/conversions |
| `CANON-8/decimal-literal-form` | 2805-2824 | `5d29e066b6b5` | planned | openspec/specs/language/decimal-types |
| `CANON-8/operators-and-numeric-law` | 2825-2852 | `4234270d384a` | planned | openspec/specs/language/numeric-types |
| `SECTION-13` | 2853-2863 | `2897fbbce2ae` | planned | openspec/specs/language/variables, openspec/specs/language/mutability |
| `SECTION-13/variable-declaration-rules` | 2864-2867 | `754e4e1781da` | planned | openspec/specs/language/variables |
| `CANON-10` | 2868-2892 | `9e51efaea2ef` | planned | openspec/specs/language/mutability |
| `SECTION-14` | 2893-2895 | `47360f55ede4` | structural | heading only |
| `scoping-and-no-shadowing` | 2896-2908 | `212122b8a58f` | planned | openspec/specs/language/scoping |
| `SECTION-15` | 2909-2911 | `60939e1901e7` | structural | heading only |
| `CANON-6` | 2912-2932 | `deea50879587` | planned | openspec/specs/language/routines |
| `SECTION-16` | 2933-2935 | `70008477d760` | structural | heading only |
| `control-flow-constructs` | 2936-2937 | `49a7482f73bc` | structural | heading only |
| `CANON-12` | 2938-2941 | `778fe3b1480b` | planned | openspec/specs/language/control-flow |
| `CANON-12/if-elif-else` | 2942-2952 | `3cf7facb9c42` | planned | openspec/specs/language/control-flow |
| `CANON-15` | 2953-3102 | `a52eb819fcce` | planned | openspec/specs/language/exceptions |
| `CANON-15/current-v3-17-lowering-note` | 3103-3113 | `0c522c2d339d` | planned | docs/STATUS.md |
| `CANON-11` | 3114-3145 | `ad57cb2fc650` | planned | openspec/specs/language/with-statement |
| `SECTION-17` | 3146-3148 | `f11baea07e11` | structural | heading only |
| `SECTION-17/while` | 3149-3154 | `ba94f83af4d4` | planned | openspec/specs/language/loops |
| `SECTION-17/repeat-until` | 3155-3164 | `45814b3807ec` | planned | openspec/specs/language/loops |
| `SECTION-17/for-in-range` | 3165-3195 | `363715f8a505` | planned | openspec/specs/language/loops |
| `SECTION-17/conditional-expression` | 3196-3204 | `2dceb1930154` | planned | openspec/specs/language/conditional-expression |
| `SECTION-17/case` | 3205-3230 | `6639c7cf4313` | mirrored | `openspec/specs/language/case-statement/spec.md` (Enum part: change enum-range-set) |
| `SECTION-17/unless` | 3231-3235 | `c75b89637df7` | mirrored | `openspec/specs/language/case-statement/spec.md` |
| `SECTION-18` | 3236-3238 | `e522f90c7379` | structural | heading only |
| `CANON-9` | 3239-3280 | `8a021bf1ff92` | planned | openspec/specs/language/structs, openspec/specs/language/associated-methods |
| `CANON-9/named-struct-construction` | 3281-3317 | `d0090df0b373` | planned | openspec/specs/language/structs |
| `SECTION-19` | 3318-3320 | `e0d3647383a1` | structural | heading only |
| `CANON-14` | 3321-3349 | `3bb68612b459` | planned | openspec/specs/language/strings |
| `SECTION-20` | 3350-3354 | `3b560adced11` | planned | openspec/specs/language/numeric-types |
| `numeric-semantics-summary` | 3355-3365 | `bda5b68d9240` | planned | openspec/specs/language/numeric-types |
| `CANON-20` | 3366-3373 | `b20c1473c6e9` | planned | openspec/specs/language/operator-precedence |
| `CANON-20/design-intent` | 3374-3382 | `b9c61f6ebef4` | planned | openspec/specs/language/operator-precedence |
| `CANON-20/precedence-table` | 3383-3419 | `606ca4e1d824` | planned | openspec/specs/language/operator-precedence |
| `CANON-20/mandatory-parentheses-rule` | 3420-3440 | `6d49217d2355` | planned | openspec/specs/language/operator-precedence |
| `CANON-20/verifiability` | 3441-3456 | `40b3313dba9d` | planned | openspec/specs/language/operator-precedence |
| `SECTION-21` | 3457-3459 | `ec02c6ed9865` | structural | heading only |
| `CANON-13` | 3460-3461 | `f59f57e57286` | structural | heading only |
| `CANON-13/array` | 3462-3469 | `f2d44c0dd686` | mirrored | `openspec/specs/language/fixed-arrays/spec.md` |
| `CANON-13/vector` | 3470-3476 | `c99a2607fa26` | planned | openspec/specs/language/vectors |
| `CANON-13/range` | 3477-3482 | `c7b61a41d8eb` | planned | openspec/specs/language/ranges |
| `CANON-13/enum` | 3483-3496 | `2d11b4cc7dc3` | planned | openspec/specs/language/enums |
| `CANON-13/set` | 3497-3508 | `240f92fbca14` | planned | openspec/specs/language/sets |
| `SECTION-22` | 3509-3511 | `f6a146590eb2` | structural | heading only |
| `CANON-18` | 3512-3555 | `3c45aca17928` | planned | openspec/specs/stdlib/standard-library |
| `CANON-17` | 3556-3591 | `476882e140b5` | planned | openspec/specs/stdlib/console-io |
| `CANON-17/files` | 3592-3626 | `9f1eb3536845` | planned | openspec/specs/language (capability to choose) |
| `SECTION-23` | 3627-3629 | `f29a82eefbab` | structural | heading only |
| `CANON-19` | 3630-3631 | `2682b6b517b1` | structural | heading only |
| `CANON-19/bootstrap-exception-runtime` | 3632-3667 | `3732c1eb5dfc` | planned | openspec/specs/runtime/exception-runtime |
| `CANON-19/checked-integer-arithmetic` | 3668-3744 | `8b1088d2777d` | planned | openspec/specs/runtime/runtime-faults |
| `CANON-19/compiler-implementation-limits` | 3745-3764 | `4564c0b0dffc` | planned | openspec/specs/compiler/limits |
| `temporary-console-i-o-abi` | 3765-3776 | `ee67f38f23b2` | planned | docs/STATUS.md |
| `SECTION-24` | 3777-3779 | `d83447a408b7` | structural | heading only |
| `CANON-20~2` | 3780-3810 | `89bc9490baa9` | planned | openspec/specs/compiler/llvm-backend |
| `current-backend-support-status` | 3811-3812 | `92e4d170aa16` | structural | heading only |
| `B-WORKS` | 3813-3832 | `05b04ae6d0f9` | planned | docs/STATUS.md |
| `B-PARSED` | 3833-3838 | `010d10c5ad64` | planned | docs/STATUS.md |
| `B-GAPS` | 3839-3909 | `376488d68493` | planned | docs/STATUS.md |
| `SECTION-25` | 3910-3912 | `561c278c3296` | structural | heading only |
| `examples-policy` | 3913-3922 | `1ef48ef87dbf` | planned | docs/ENGINEERING.md |
| `B-EXAMPLES` | 3923-3931 | `3d9b7dd4c5e3` | planned | docs/STATUS.md |
| `SECTION-26` | 3932-3934 | `9a88a697ae99` | structural | heading only |
| `CANON-21` | 3935-3968 | `afab1f61829e` | planned | docs/ENGINEERING.md |
| `required-regression-layers` | 3969-3978 | `6098f4656540` | planned | docs/ENGINEERING.md |
| `SECTION-27` | 3979-3981 | `62cc5be85e73` | structural | heading only |
| `release-validation-policy` | 3982-4000 | `c2bd42d65889` | planned | docs/RELEASE.md |
| `DEV-2~2` | 4001-4037 | `6ff9fac762f9` | duplicate | `unit:DEV-2` |
| `GROUP-future~2` | 4038-4046 | `c0c3ec88bb92` | duplicate | `unit:GROUP-future` |
| `SECTION-28` | 4047-4049 | `b97b11b56381` | structural | heading only |
| `documentation-generation-policy` | 4050-4062 | `1d33470c8cd5` | planned | docs/ENGINEERING.md |
| `SECTION-29` | 4063-4065 | `000dd38c7ed9` | structural | heading only |
| `DEV-1` | 4066-4103 | `48712f26b690` | planned | docs/RELEASE.md |
| `portability-targets` | 4104-4150 | `2422ef6e3b0a` | planned | docs/RELEASE.md |
| `SECTION-30` | 4151-4155 | `683f7b9b00c7` | planned | docs/STATUS.md |
| `B-WORKS~2` | 4156-4172 | `65db813fde18` | planned | docs/STATUS.md |
| `B-PARSED~2` | 4173-4178 | `010d10c5ad64` | planned | docs/STATUS.md |
| `B-GAPS~2` | 4179-4247 | `a5ea1b1eb65f` | planned | docs/STATUS.md |
| `B-CONFLICTS` | 4248-4255 | `fdc28764bc4a` | planned | docs/STATUS.md |
| `B-EXAMPLES~2` | 4256-4262 | `f85a9a4e4be5` | planned | docs/STATUS.md |
| `B-PROPOSALS` | 4263-4271 | `b5622600ecc3` | planned | openspec/changes (one change per open item) |
| `SECTION-31` | 4272-4274 | `0c7c92ea754c` | structural | heading only |
| `CANON-22` | 4275-4294 | `0fd628df7854` | planned | docs/ROADMAP.md |
| `GROUP-decisions` | 4295-4302 | `8b90cf0e6ca5` | planned | openspec/README.md (history) |
| `roadmap-to-1-0` | 4303-4317 | `de99ff09125d` | planned | docs/ROADMAP.md |
| `backend-strategy-directive` | 4318-4324 | `3e733152683f` | planned | docs/ENGINEERING.md |
| `backend-strategy-directive/core-principle` | 4325-4338 | `97d3972ccf1e` | planned | docs/ENGINEERING.md |
| `backend-strategy-directive/backend-maturation-sequence` | 4339-4354 | `ff2dbe76d57a` | planned | docs/ENGINEERING.md |
| `backend-strategy-directive/objective-criterion-for-leaving-clang` | 4355-4369 | `1cda38393426` | planned | docs/ENGINEERING.md |
| `backend-strategy-directive/scope-guard-for-backend-adjacent-work` | 4370-4376 | `859bab429005` | planned | docs/ENGINEERING.md |
| `compiler-module-architecture-directive` | 4377-4383 | `7dfc746e78b5` | planned | docs/ENGINEERING.md |
| `compiler-module-architecture-directive/principle-reactive-not-predictive` | 4384-4392 | `6bdbaef0d48c` | planned | docs/ENGINEERING.md |
| `compiler-module-architecture-directive/the-three-tests-for-extracting-a-module` | 4393-4408 | `9bdd4cb899e6` | planned | docs/ENGINEERING.md |
| `compiler-module-architecture-directive/anti-rules` | 4409-4417 | `c8700ead7c93` | planned | docs/ENGINEERING.md |
| `compiler-module-architecture-directive/working-rule-of-thumb` | 4418-4422 | `ec2c782d1c49` | planned | docs/ENGINEERING.md |
| `compiler-module-architecture-directive/timing` | 4423-4429 | `c4c0fef11f84` | planned | docs/ENGINEERING.md |
| `BANNER-end-of-inox-canonical-md-v3-8` | 4430-4435 | `784433b7be07` | planned | openspec/README.md (reading order) |
