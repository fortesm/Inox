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
| `BANNER-the-single-canonical-document` | 3-56 | `a310d56c0344` | planned | openspec/README.md (authority, maintainer, license, map) |
| `SECTION-00` | 57-61 | `a52f0279cfd3` | planned | AGENTS.md (governance) |
| `how-to-read-this-document` | 62-80 | `2da10040fa3b` | planned | AGENTS.md (governance) |
| `governance-rules-that-protect-this-docum` | 81-83 | `d7e7bb760361` | structural | heading only |
| `G1` | 84-88 | `5b3eb650bc3e` | planned | AGENTS.md (governance) |
| `G2` | 89-94 | `7e4916e2f072` | planned | AGENTS.md (governance) |
| `G3` | 95-99 | `bec8720b4966` | planned | AGENTS.md (governance) |
| `G4` | 100-105 | `d3cdf08f12d6` | planned | AGENTS.md (governance) |
| `G5` | 106-109 | `d30cc9be5c2d` | planned | AGENTS.md (governance) |
| `G6` | 110-115 | `a31bc426d896` | planned | AGENTS.md (governance) |
| `G7` | 116-131 | `cab622ba4233` | planned | AGENTS.md (governance) |
| `first-order-engineering-directive-modern` | 132-144 | `3dd9b1a61865` | planned | docs/ENGINEERING.md |
| `first-order-engineering-directive-modern/sources-of-influence` | 145-164 | `bbd91006fa00` | planned | docs/ENGINEERING.md |
| `E1` | 165-174 | `0a4f8d0bf8e6` | planned | docs/ENGINEERING.md |
| `E2` | 175-180 | `9ac4af877fe2` | planned | docs/ENGINEERING.md |
| `E3` | 181-193 | `3b3dc8f81aab` | planned | docs/ENGINEERING.md |
| `E4` | 194-203 | `b5b56a927866` | planned | docs/ENGINEERING.md |
| `E5` | 204-217 | `a1f2655f17a0` | planned | docs/ENGINEERING.md |
| `E6` | 218-226 | `8ea2cd3e52dd` | planned | docs/ENGINEERING.md |
| `E7` | 227-233 | `bc0e80bfa2d2` | planned | docs/ENGINEERING.md |
| `E8` | 234-240 | `c2079e95428c` | planned | docs/ENGINEERING.md |
| `E9` | 241-254 | `5ef640e73e9a` | planned | docs/ENGINEERING.md |
| `E10` | 255-260 | `946045224bbf` | planned | docs/ENGINEERING.md |
| `E11` | 261-287 | `e8a0a5fe161f` | planned | docs/ENGINEERING.md |
| `E12` | 288-303 | `bbc33f2d5937` | planned | docs/ENGINEERING.md |
| `E13` | 304-312 | `df40d606204e` | planned | docs/ENGINEERING.md |
| `E14` | 313-319 | `a4852c4ec3ad` | planned | docs/ENGINEERING.md |
| `E15` | 320-326 | `b573fb6af16e` | planned | docs/ENGINEERING.md |
| `E16` | 327-335 | `5b7e688474bd` | planned | docs/ENGINEERING.md |
| `E17` | 336-351 | `a61f45216f80` | planned | docs/ENGINEERING.md |
| `E18` | 352-358 | `0be9d22f9963` | planned | docs/ENGINEERING.md |
| `E19` | 359-396 | `0db23d3788c7` | planned | docs/ENGINEERING.md |
| `E20` | 397-405 | `3cbd923272e0` | planned | docs/ENGINEERING.md |
| `non-negotiable-rules` | 406-417 | `ace55d5cb612` | planned | AGENTS.md (governance) |
| `SECTION-01` | 418-420 | `8761a2a5882c` | structural | heading only |
| `CANON-1` | 421-429 | `db04e9c70c16` | planned | docs/VISION.md |
| `CANON-1/language-references-and-influences` | 430-448 | `f1408ec493d5` | planned | docs/VISION.md |
| `CANON-1/mission-critical-target-domain` | 449-461 | `453687b19f4a` | planned | docs/VISION.md |
| `CANON-1/core-safety-stance` | 462-476 | `1d21e14e00f1` | planned | docs/VISION.md |
| `CANON-1/post-object-oriented-design` | 477-484 | `184e39a5df22` | planned | docs/VISION.md |
| `CANON-1/performance-model` | 485-494 | `d6a3455f8387` | planned | docs/VISION.md |
| `CANON-1/agent-rule` | 495-501 | `7117205f7492` | planned | docs/VISION.md |
| `SECTION-02` | 502-504 | `b705068ef5b9` | structural | heading only |
| `change-log` | 505-507 | `0fab86654403` | structural | heading only |
| `LOG/v3.54` | 508-524 | `156e7b7bd306` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.53` | 525-555 | `7d280ce2326d` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.52` | 556-586 | `a7dbe03eaf28` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.51` | 587-593 | `7e14e9ffa7a9` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.50` | 594-618 | `0441fbff3bf5` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.49` | 619-645 | `be098b1b06bd` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.48` | 646-667 | `afa50d5ec69f` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.47` | 668-680 | `72a0e9ea7e82` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.46` | 681-694 | `07eee37e2b47` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.45` | 695-723 | `e8bf1bc74e25` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.44` | 724-746 | `1ed18d87a1f6` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.43` | 747-756 | `7cec90575743` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.42` | 757-772 | `e00cff8892ed` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.41` | 773-778 | `d9b78ef08a85` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.40` | 779-797 | `3c2103c842ec` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.39` | 798-827 | `ff8a1f06c34f` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.38` | 828-838 | `d97e13c1b814` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.37` | 839-850 | `73f3f0239390` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.36` | 851-867 | `889932b86d67` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.35` | 868-885 | `a40c544968ff` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.34` | 886-898 | `fa31c1e6a95f` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.33` | 899-921 | `546628eeb9d8` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.32` | 922-943 | `01b8acd407b7` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.31` | 944-957 | `c6f79fbad05a` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.30` | 958-979 | `82fc2948b1b0` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.29` | 980-1040 | `411404a57a61` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.28` | 1041-1076 | `f95a9348aa62` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.27` | 1077-1132 | `9a521127b697` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.26` | 1133-1152 | `d412b402bbfb` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.25` | 1153-1165 | `ac5b09d96e50` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.24` | 1166-1188 | `977e3c458a59` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.23` | 1189-1228 | `1bd9020135e7` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.22` | 1229-1243 | `c3cd869369d8` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.21` | 1244-1308 | `fa631814dc07` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.20` | 1309-1337 | `c178428e9e51` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.19` | 1338-1353 | `58a9449923cc` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.18` | 1354-1432 | `63b0850a3b94` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.17` | 1433-1453 | `6b82cfa2bfef` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.16` | 1454-1474 | `504ebe528c6f` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.15a` | 1475-1488 | `8a99974e3b15` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.15` | 1489-1503 | `db45cccf8f29` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.14` | 1504-1514 | `45d2e2e7b0f4` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.13` | 1515-1526 | `e78d99b40dce` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.12` | 1527-1536 | `adf42d99bf9c` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.11` | 1537-1546 | `bb084ddde131` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.10` | 1547-1559 | `bb0c5292dc58` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.8` | 1560-1570 | `e2021267fde4` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.7` | 1571-1581 | `4c50f899f80e` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.6` | 1582-1598 | `f5c6a45807b0` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.5` | 1599-1603 | `1e634d009b80` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.4` | 1604-1609 | `a02b7bf01817` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.3` | 1610-1616 | `b73b838ee71a` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.2` | 1617-1623 | `3aae34cdb8de` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.1` | 1624-1636 | `1c1bfe44fee1` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.0` | 1637-1646 | `01fee319f99c` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v2.2` | 1647-1649 | `947024e547a5` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v2.1` | 1650-1652 | `831353b88b26` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v2.0` | 1653-1658 | `92161592c6f6` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v1.0` | 1659-1660 | `bf9fe6205559` | structural | heading only |
| `DEV-2` | 1661-1697 | `6ff9fac762f9` | planned | docs/RELEASE.md |
| `GROUP-future` | 1698-1706 | `c0c3ec88bb92` | planned | docs/ROADMAP.md |
| `SECTION-03` | 1707-1709 | `8f4b634fd646` | structural | heading only |
| `agent-rules` | 1710-1743 | `f699b67648da` | planned | AGENTS.md (governance) |
| `SECTION-04` | 1744-1748 | `03786d4742d7` | planned | docs/ROADMAP.md |
| `B-OPEN` | 1749-1793 | `4e5f8d3984d1` | planned | openspec/changes (one change per open item) |
| `FUTURE-1` | 1794-1801 | `0a1e1448d12b` | planned | docs/ROADMAP.md |
| `FUTURE-2` | 1802-1806 | `eb05f449bfc6` | planned | docs/ROADMAP.md |
| `FUTURE-3` | 1807-1812 | `2c09d65cfded` | planned | docs/ROADMAP.md |
| `FUTURE-4` | 1813-1816 | `62d8fde09ca2` | planned | docs/ROADMAP.md |
| `FUTURE-5` | 1817-1821 | `041157e3cd11` | planned | docs/ROADMAP.md |
| `FUTURE-6` | 1822-1825 | `a070988b84e8` | planned | docs/ROADMAP.md |
| `FUTURE-7` | 1826-1838 | `116a4980e694` | planned | docs/ROADMAP.md |
| `GROUP-open-questions` | 1839-1862 | `9806de6ee55e` | planned | docs/ROADMAP.md, docs/STATUS.md |
| `BANNER-agent-rules` | 1863-1896 | `1dd939e2384f` | duplicate | `unit:agent-rules` |
| `BANNER-layer-b-implementation-status` | 1897-1917 | `cb181e7e1383` | planned | docs/STATUS.md |
| `SECTION-05` | 1918-1922 | `3950191b4348` | planned | openspec/README.md (history) |
| `ADR-0001` | 1923-1927 | `6597d424187c` | copied | `openspec/changes/archive/2026-05-28-adr-0001-llvm-backend` |
| `ADR-0002` | 1928-1933 | `24673434b56e` | copied | `openspec/changes/archive/2026-05-28-adr-0002-block-syntax` |
| `ADR-0003` | 1934-1937 | `91f21c8db59b` | copied | `openspec/changes/archive/2026-05-28-adr-0003-type-system` |
| `ADR-0004` | 1938-1946 | `a7ea289039f3` | copied | `openspec/changes/archive/2026-05-28-adr-0004-exceptions` |
| `ADR-0005` | 1947-1970 | `b3487abfdaa3` | copied | `openspec/changes/archive/2026-05-31-adr-0005-consolidated-0-1-language-decisions` |
| `ADR-0006` | 1971-2018 | `00635fb17fa5` | copied | `openspec/changes/archive/2026-05-31-adr-0006-inox-0-1-language-constitution` |
| `ADR-0017` | 2019-2044 | `99a0ae3ad662` | copied | `openspec/changes/archive/2026-10-11-adr-0017-read-only-slices` |
| `ADR-0016` | 2045-2076 | `ae224ec3acef` | copied | `openspec/changes/archive/2026-10-11-adr-0016-ref-and-ref-mut-parameters` |
| `ADR-0015` | 2077-2094 | `d138712bb9d2` | copied | `openspec/changes/archive/2026-10-10-adr-0015-storing-into-a-natural-checks-the-range` |
| `ADR-0014` | 2095-2122 | `8f83b1f7aa88` | copied | `openspec/changes/archive/2026-10-10-adr-0014-natural-byte-and-the-closed-numeric-set` |
| `ADR-0013` | 2123-2148 | `8b43a6f9c9e7` | copied | `openspec/changes/archive/2026-10-10-adr-0013-conditional-expression` |
| `ADR-0012` | 2149-2162 | `387bf21df2e0` | copied | `openspec/changes/archive/2026-10-10-adr-0012-compound-assignment` |
| `ADR-0011` | 2163-2195 | `8cd31c3b1d14` | copied | `openspec/changes/archive/2026-10-09-adr-0011-case-in-the-ada-spark-style` |
| `ADR-0010` | 2196-2207 | `01ebe198452d` | copied | `openspec/changes/archive/2026-10-09-adr-0010-step-in-the-for-header` |
| `ADR-0009` | 2208-2223 | `9fb0711acf2b` | copied | `openspec/changes/archive/2026-10-09-adr-0009-chained-assignment` |
| `ADR-0008` | 2224-2241 | `6cbc9717bb6c` | copied | `openspec/changes/archive/2026-10-09-adr-0008-precedence-of-and-in` |
| `ADR-0007` | 2242-2254 | `d8e65f343093` | copied | `openspec/changes/archive/2026-10-09-adr-0007-loop-exit-and-try-cleanup-keywords` |
| `GROUP-development` | 2255-2270 | `6f6455b69203` | structural | heading only |
| `SECTION-06` | 2271-2273 | `1f3fa370b3e3` | structural | heading only |
| `canonical-user-facing-cli-contract` | 2274-2289 | `45e0e5d66ff0` | planned | openspec/specs/compiler/cli |
| `current-observed-driver-status` | 2290-2295 | `15c15b4e82d8` | planned | docs/STATUS.md |
| `SECTION-07` | 2296-2298 | `f2dfbd93d1af` | structural | heading only |
| `source-file-structure` | 2299-2310 | `f905deb42055` | planned | openspec/specs/language/modules |
| `SECTION-08` | 2311-2313 | `83d42d4dfe69` | structural | heading only |
| `CANON-2` | 2314-2345 | `1784cd8f9c86` | planned | openspec/specs/language/lexical |
| `SECTION-09` | 2346-2348 | `151b987c27db` | structural | heading only |
| `CANON-3` | 2349-2369 | `eb3765442875` | planned | openspec/specs/language/modules |
| `SECTION-10` | 2370-2372 | `539a6368d8f5` | structural | heading only |
| `CANON-4` | 2373-2428 | `011a17049109` | planned | openspec/specs/language/block-syntax |
| `CANON-7` | 2429-2440 | `954891e7e64b` | planned | openspec/specs/language/block-syntax |
| `SECTION-11` | 2441-2443 | `cc93dd9983b8` | structural | heading only |
| `CANON-5` | 2444-2448 | `107db4bb6efc` | planned | openspec/specs/language/variables |
| `CANON-5/type-section` | 2449-2463 | `ee2b644e002f` | planned | openspec/specs/language/declaration-sections |
| `CANON-5/local-variables-three-valid-forms-only` | 2464-2471 | `7acb630f2600` | planned | openspec/specs/language/variables |
| `CANON-5/grouped-declarations` | 2472-2484 | `211d54982dd6` | planned | openspec/specs/language/variables |
| `CANON-5/compound-assignment` | 2485-2494 | `ac2d6220edcf` | planned | openspec/specs/language/assignment |
| `CANON-5/assignment-is-a-statement-chained-assign` | 2495-2554 | `4a3bbaf10235` | planned | openspec/specs/language/assignment |
| `CANON-5/safe-type-inference` | 2555-2563 | `8cabd03f259a` | planned | openspec/specs/language/variables |
| `CANON-5/const-and-state` | 2564-2573 | `c77d846e37bf` | planned | openspec/specs/language/declaration-sections, openspec/specs/language/module-state |
| `SECTION-12` | 2574-2576 | `21806ebd31bd` | structural | heading only |
| `CANON-8` | 2577-2578 | `44025c9660f2` | structural | heading only |
| `CANON-8/canonical-type-hierarchy` | 2579-2619 | `3d044d5e00c7` | planned | openspec/specs/language/numeric-types, openspec/specs/language/natural, openspec/specs/language/fixed-width-integers |
| `CANON-8/generics-use-angle-brackets` | 2620-2628 | `599f5b0a40cf` | planned | openspec/specs/language/generics |
| `CANON-8/conversions` | 2629-2641 | `233a06f0e0e2` | planned | openspec/specs/language/conversions |
| `CANON-8/decimal-literal-form` | 2642-2661 | `5d29e066b6b5` | planned | openspec/specs/language/decimal-types |
| `CANON-8/operators-and-numeric-law` | 2662-2689 | `4234270d384a` | planned | openspec/specs/language/numeric-types |
| `SECTION-13` | 2690-2700 | `2897fbbce2ae` | planned | openspec/specs/language/variables, openspec/specs/language/mutability |
| `SECTION-13/variable-declaration-rules` | 2701-2704 | `754e4e1781da` | planned | openspec/specs/language/variables |
| `CANON-10` | 2705-2729 | `9e51efaea2ef` | planned | openspec/specs/language/mutability |
| `SECTION-14` | 2730-2732 | `47360f55ede4` | structural | heading only |
| `scoping-and-no-shadowing` | 2733-2745 | `212122b8a58f` | planned | openspec/specs/language/scoping |
| `SECTION-15` | 2746-2748 | `60939e1901e7` | structural | heading only |
| `CANON-6` | 2749-2769 | `deea50879587` | planned | openspec/specs/language/routines |
| `SECTION-16` | 2770-2772 | `70008477d760` | structural | heading only |
| `control-flow-constructs` | 2773-2774 | `49a7482f73bc` | structural | heading only |
| `CANON-12` | 2775-2778 | `778fe3b1480b` | planned | openspec/specs/language/control-flow |
| `CANON-12/if-elif-else` | 2779-2789 | `3cf7facb9c42` | planned | openspec/specs/language/control-flow |
| `CANON-15` | 2790-2939 | `a52eb819fcce` | planned | openspec/specs/language/exceptions |
| `CANON-15/current-v3-17-lowering-note` | 2940-2950 | `0c522c2d339d` | planned | docs/STATUS.md |
| `CANON-11` | 2951-2982 | `ad57cb2fc650` | planned | openspec/specs/language/with-statement |
| `SECTION-17` | 2983-2985 | `f11baea07e11` | structural | heading only |
| `SECTION-17/while` | 2986-2991 | `ba94f83af4d4` | planned | openspec/specs/language/loops |
| `SECTION-17/repeat-until` | 2992-3001 | `45814b3807ec` | planned | openspec/specs/language/loops |
| `SECTION-17/for-in-range` | 3002-3032 | `363715f8a505` | planned | openspec/specs/language/loops |
| `SECTION-17/conditional-expression` | 3033-3041 | `2dceb1930154` | planned | openspec/specs/language/conditional-expression |
| `SECTION-17/case` | 3042-3067 | `6639c7cf4313` | mirrored | `openspec/specs/language/case-statement/spec.md` (Enum part: change enum-range-set) |
| `SECTION-17/unless` | 3068-3072 | `c75b89637df7` | mirrored | `openspec/specs/language/case-statement/spec.md` |
| `SECTION-18` | 3073-3075 | `e522f90c7379` | structural | heading only |
| `CANON-9` | 3076-3115 | `d5eec8fbbc36` | planned | openspec/specs/language/structs, openspec/specs/language/associated-methods |
| `CANON-9/named-struct-construction` | 3116-3152 | `d0090df0b373` | planned | openspec/specs/language/structs |
| `SECTION-19` | 3153-3155 | `e0d3647383a1` | structural | heading only |
| `CANON-14` | 3156-3172 | `e5499ac61e11` | planned | openspec/specs/language/strings |
| `SECTION-20` | 3173-3177 | `3b560adced11` | planned | openspec/specs/language/numeric-types |
| `numeric-semantics-summary` | 3178-3188 | `bda5b68d9240` | planned | openspec/specs/language/numeric-types |
| `CANON-20` | 3189-3196 | `b20c1473c6e9` | planned | openspec/specs/language/operator-precedence |
| `CANON-20/design-intent` | 3197-3205 | `b9c61f6ebef4` | planned | openspec/specs/language/operator-precedence |
| `CANON-20/precedence-table` | 3206-3242 | `606ca4e1d824` | planned | openspec/specs/language/operator-precedence |
| `CANON-20/mandatory-parentheses-rule` | 3243-3263 | `6d49217d2355` | planned | openspec/specs/language/operator-precedence |
| `CANON-20/verifiability` | 3264-3279 | `40b3313dba9d` | planned | openspec/specs/language/operator-precedence |
| `SECTION-21` | 3280-3282 | `ec02c6ed9865` | structural | heading only |
| `CANON-13` | 3283-3284 | `f59f57e57286` | structural | heading only |
| `CANON-13/array` | 3285-3292 | `f2d44c0dd686` | mirrored | `openspec/specs/language/fixed-arrays/spec.md` |
| `CANON-13/vector` | 3293-3299 | `c99a2607fa26` | planned | openspec/specs/language/vectors |
| `CANON-13/range` | 3300-3305 | `c7b61a41d8eb` | planned | openspec/specs/language/ranges |
| `CANON-13/enum` | 3306-3319 | `2d11b4cc7dc3` | planned | openspec/specs/language/enums |
| `CANON-13/set` | 3320-3331 | `240f92fbca14` | planned | openspec/specs/language/sets |
| `SECTION-22` | 3332-3334 | `f6a146590eb2` | structural | heading only |
| `CANON-18` | 3335-3355 | `c798859f64fa` | planned | openspec/specs/stdlib/standard-library |
| `CANON-17` | 3356-3388 | `b58a10142c44` | planned | openspec/specs/stdlib/console-io |
| `SECTION-23` | 3389-3391 | `f29a82eefbab` | structural | heading only |
| `CANON-19` | 3392-3393 | `2682b6b517b1` | structural | heading only |
| `CANON-19/bootstrap-exception-runtime` | 3394-3429 | `3732c1eb5dfc` | planned | openspec/specs/runtime/exception-runtime |
| `CANON-19/checked-integer-arithmetic` | 3430-3504 | `76e58b94adb4` | planned | openspec/specs/runtime/runtime-faults |
| `CANON-19/compiler-implementation-limits` | 3505-3524 | `4564c0b0dffc` | planned | openspec/specs/compiler/limits |
| `temporary-console-i-o-abi` | 3525-3536 | `ee67f38f23b2` | planned | docs/STATUS.md |
| `SECTION-24` | 3537-3539 | `d83447a408b7` | structural | heading only |
| `CANON-20~2` | 3540-3570 | `89bc9490baa9` | planned | openspec/specs/compiler/llvm-backend |
| `current-backend-support-status` | 3571-3572 | `92e4d170aa16` | structural | heading only |
| `B-WORKS` | 3573-3592 | `05b04ae6d0f9` | planned | docs/STATUS.md |
| `B-PARSED` | 3593-3598 | `010d10c5ad64` | planned | docs/STATUS.md |
| `B-GAPS` | 3599-3669 | `376488d68493` | planned | docs/STATUS.md |
| `SECTION-25` | 3670-3672 | `561c278c3296` | structural | heading only |
| `examples-policy` | 3673-3682 | `1ef48ef87dbf` | planned | docs/ENGINEERING.md |
| `B-EXAMPLES` | 3683-3691 | `3d9b7dd4c5e3` | planned | docs/STATUS.md |
| `SECTION-26` | 3692-3694 | `9a88a697ae99` | structural | heading only |
| `CANON-21` | 3695-3728 | `afab1f61829e` | planned | docs/ENGINEERING.md |
| `required-regression-layers` | 3729-3738 | `6098f4656540` | planned | docs/ENGINEERING.md |
| `SECTION-27` | 3739-3741 | `62cc5be85e73` | structural | heading only |
| `release-validation-policy` | 3742-3760 | `c2bd42d65889` | planned | docs/RELEASE.md |
| `DEV-2~2` | 3761-3797 | `6ff9fac762f9` | duplicate | `unit:DEV-2` |
| `GROUP-future~2` | 3798-3806 | `c0c3ec88bb92` | duplicate | `unit:GROUP-future` |
| `SECTION-28` | 3807-3809 | `b97b11b56381` | structural | heading only |
| `documentation-generation-policy` | 3810-3822 | `1d33470c8cd5` | planned | docs/ENGINEERING.md |
| `SECTION-29` | 3823-3825 | `000dd38c7ed9` | structural | heading only |
| `DEV-1` | 3826-3863 | `48712f26b690` | planned | docs/RELEASE.md |
| `portability-targets` | 3864-3910 | `2422ef6e3b0a` | planned | docs/RELEASE.md |
| `SECTION-30` | 3911-3915 | `683f7b9b00c7` | planned | docs/STATUS.md |
| `B-WORKS~2` | 3916-3932 | `65db813fde18` | planned | docs/STATUS.md |
| `B-PARSED~2` | 3933-3938 | `010d10c5ad64` | planned | docs/STATUS.md |
| `B-GAPS~2` | 3939-4007 | `a5ea1b1eb65f` | planned | docs/STATUS.md |
| `B-CONFLICTS` | 4008-4015 | `fdc28764bc4a` | planned | docs/STATUS.md |
| `B-EXAMPLES~2` | 4016-4022 | `f85a9a4e4be5` | planned | docs/STATUS.md |
| `B-PROPOSALS` | 4023-4031 | `b5622600ecc3` | planned | openspec/changes (one change per open item) |
| `SECTION-31` | 4032-4034 | `0c7c92ea754c` | structural | heading only |
| `CANON-22` | 4035-4054 | `0fd628df7854` | planned | docs/ROADMAP.md |
| `GROUP-decisions` | 4055-4062 | `8b90cf0e6ca5` | planned | openspec/README.md (history) |
| `roadmap-to-1-0` | 4063-4077 | `de99ff09125d` | planned | docs/ROADMAP.md |
| `backend-strategy-directive` | 4078-4084 | `3e733152683f` | planned | docs/ENGINEERING.md |
| `backend-strategy-directive/core-principle` | 4085-4098 | `97d3972ccf1e` | planned | docs/ENGINEERING.md |
| `backend-strategy-directive/backend-maturation-sequence` | 4099-4114 | `ff2dbe76d57a` | planned | docs/ENGINEERING.md |
| `backend-strategy-directive/objective-criterion-for-leaving-clang` | 4115-4129 | `1cda38393426` | planned | docs/ENGINEERING.md |
| `backend-strategy-directive/scope-guard-for-backend-adjacent-work` | 4130-4136 | `859bab429005` | planned | docs/ENGINEERING.md |
| `compiler-module-architecture-directive` | 4137-4143 | `7dfc746e78b5` | planned | docs/ENGINEERING.md |
| `compiler-module-architecture-directive/principle-reactive-not-predictive` | 4144-4152 | `6bdbaef0d48c` | planned | docs/ENGINEERING.md |
| `compiler-module-architecture-directive/the-three-tests-for-extracting-a-module` | 4153-4168 | `9bdd4cb899e6` | planned | docs/ENGINEERING.md |
| `compiler-module-architecture-directive/anti-rules` | 4169-4177 | `c8700ead7c93` | planned | docs/ENGINEERING.md |
| `compiler-module-architecture-directive/working-rule-of-thumb` | 4178-4182 | `ec2c782d1c49` | planned | docs/ENGINEERING.md |
| `compiler-module-architecture-directive/timing` | 4183-4189 | `c4c0fef11f84` | planned | docs/ENGINEERING.md |
| `BANNER-end-of-inox-canonical-md-v3-8` | 4190-4195 | `784433b7be07` | planned | openspec/README.md (reading order) |
