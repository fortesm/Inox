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
| `BANNER-the-single-canonical-document` | 3-59 | `ffd7dd6def7e` | planned | openspec/README.md (authority, maintainer, license, map) |
| `SECTION-00` | 60-64 | `a52f0279cfd3` | planned | AGENTS.md (governance) |
| `how-to-read-this-document` | 65-83 | `2da10040fa3b` | planned | AGENTS.md (governance) |
| `governance-rules-that-protect-this-docum` | 84-86 | `d7e7bb760361` | structural | heading only |
| `G1` | 87-91 | `5b3eb650bc3e` | planned | AGENTS.md (governance) |
| `G2` | 92-97 | `7e4916e2f072` | planned | AGENTS.md (governance) |
| `G3` | 98-102 | `bec8720b4966` | planned | AGENTS.md (governance) |
| `G4` | 103-108 | `d3cdf08f12d6` | planned | AGENTS.md (governance) |
| `G5` | 109-112 | `d30cc9be5c2d` | planned | AGENTS.md (governance) |
| `G6` | 113-118 | `a31bc426d896` | planned | AGENTS.md (governance) |
| `G7` | 119-134 | `cab622ba4233` | planned | AGENTS.md (governance) |
| `first-order-engineering-directive-modern` | 135-147 | `3dd9b1a61865` | planned | docs/ENGINEERING.md |
| `first-order-engineering-directive-modern/sources-of-influence` | 148-167 | `bbd91006fa00` | planned | docs/ENGINEERING.md |
| `E1` | 168-177 | `0a4f8d0bf8e6` | planned | docs/ENGINEERING.md |
| `E2` | 178-183 | `9ac4af877fe2` | planned | docs/ENGINEERING.md |
| `E3` | 184-196 | `3b3dc8f81aab` | planned | docs/ENGINEERING.md |
| `E4` | 197-206 | `b5b56a927866` | planned | docs/ENGINEERING.md |
| `E5` | 207-220 | `a1f2655f17a0` | planned | docs/ENGINEERING.md |
| `E6` | 221-229 | `8ea2cd3e52dd` | planned | docs/ENGINEERING.md |
| `E7` | 230-236 | `bc0e80bfa2d2` | planned | docs/ENGINEERING.md |
| `E8` | 237-243 | `c2079e95428c` | planned | docs/ENGINEERING.md |
| `E9` | 244-257 | `5ef640e73e9a` | planned | docs/ENGINEERING.md |
| `E10` | 258-263 | `946045224bbf` | planned | docs/ENGINEERING.md |
| `E11` | 264-290 | `e8a0a5fe161f` | planned | docs/ENGINEERING.md |
| `E12` | 291-306 | `bbc33f2d5937` | planned | docs/ENGINEERING.md |
| `E13` | 307-315 | `df40d606204e` | planned | docs/ENGINEERING.md |
| `E14` | 316-322 | `a4852c4ec3ad` | planned | docs/ENGINEERING.md |
| `E15` | 323-329 | `b573fb6af16e` | planned | docs/ENGINEERING.md |
| `E16` | 330-338 | `5b7e688474bd` | planned | docs/ENGINEERING.md |
| `E17` | 339-354 | `a61f45216f80` | planned | docs/ENGINEERING.md |
| `E18` | 355-361 | `0be9d22f9963` | planned | docs/ENGINEERING.md |
| `E19` | 362-399 | `0db23d3788c7` | planned | docs/ENGINEERING.md |
| `E20` | 400-408 | `3cbd923272e0` | planned | docs/ENGINEERING.md |
| `non-negotiable-rules` | 409-420 | `ace55d5cb612` | planned | AGENTS.md (governance) |
| `SECTION-01` | 421-423 | `8761a2a5882c` | structural | heading only |
| `CANON-1` | 424-432 | `db04e9c70c16` | planned | docs/VISION.md |
| `CANON-1/language-references-and-influences` | 433-451 | `f1408ec493d5` | planned | docs/VISION.md |
| `CANON-1/mission-critical-target-domain` | 452-464 | `453687b19f4a` | planned | docs/VISION.md |
| `CANON-1/core-safety-stance` | 465-479 | `1d21e14e00f1` | planned | docs/VISION.md |
| `CANON-1/post-object-oriented-design` | 480-487 | `184e39a5df22` | planned | docs/VISION.md |
| `CANON-1/performance-model` | 488-497 | `d6a3455f8387` | planned | docs/VISION.md |
| `CANON-1/agent-rule` | 498-504 | `7117205f7492` | planned | docs/VISION.md |
| `SECTION-02` | 505-507 | `b705068ef5b9` | structural | heading only |
| `versioning` | 508-543 | `454f088bfd87` | planned | openspec/specs/language (capability to choose) |
| `change-log` | 544-546 | `0fab86654403` | structural | heading only |
| `LOG/r3.60` | 547-567 | `c3c81ac94310` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/r3.59` | 568-606 | `7f8ca7714463` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/r3.58` | 607-635 | `c9679947a1a0` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.57` | 636-656 | `3bc8f8242e98` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.56` | 657-678 | `a64911c477d4` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.55` | 679-709 | `f8b2eae4b70f` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.54` | 710-726 | `156e7b7bd306` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.53` | 727-757 | `7d280ce2326d` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.52` | 758-788 | `a7dbe03eaf28` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.51` | 789-795 | `7e14e9ffa7a9` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.50` | 796-820 | `0441fbff3bf5` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.49` | 821-847 | `be098b1b06bd` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.48` | 848-869 | `afa50d5ec69f` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.47` | 870-882 | `72a0e9ea7e82` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.46` | 883-896 | `07eee37e2b47` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.45` | 897-925 | `e8bf1bc74e25` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.44` | 926-948 | `1ed18d87a1f6` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.43` | 949-958 | `7cec90575743` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.42` | 959-974 | `e00cff8892ed` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.41` | 975-980 | `d9b78ef08a85` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.40` | 981-999 | `3c2103c842ec` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.39` | 1000-1029 | `ff8a1f06c34f` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.38` | 1030-1040 | `d97e13c1b814` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.37` | 1041-1052 | `73f3f0239390` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.36` | 1053-1069 | `889932b86d67` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.35` | 1070-1087 | `a40c544968ff` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.34` | 1088-1100 | `fa31c1e6a95f` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.33` | 1101-1123 | `546628eeb9d8` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.32` | 1124-1145 | `01b8acd407b7` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.31` | 1146-1159 | `c6f79fbad05a` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.30` | 1160-1181 | `82fc2948b1b0` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.29` | 1182-1242 | `411404a57a61` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.28` | 1243-1278 | `f95a9348aa62` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.27` | 1279-1334 | `9a521127b697` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.26` | 1335-1354 | `d412b402bbfb` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.25` | 1355-1367 | `ac5b09d96e50` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.24` | 1368-1390 | `977e3c458a59` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.23` | 1391-1430 | `1bd9020135e7` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.22` | 1431-1445 | `c3cd869369d8` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.21` | 1446-1510 | `fa631814dc07` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.20` | 1511-1539 | `c178428e9e51` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.19` | 1540-1555 | `58a9449923cc` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.18` | 1556-1634 | `63b0850a3b94` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.17` | 1635-1655 | `6b82cfa2bfef` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.16` | 1656-1676 | `504ebe528c6f` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.15a` | 1677-1690 | `8a99974e3b15` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.15` | 1691-1705 | `db45cccf8f29` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.14` | 1706-1716 | `45d2e2e7b0f4` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.13` | 1717-1728 | `e78d99b40dce` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.12` | 1729-1738 | `adf42d99bf9c` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.11` | 1739-1748 | `bb084ddde131` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.10` | 1749-1761 | `bb0c5292dc58` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.8` | 1762-1772 | `e2021267fde4` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.7` | 1773-1783 | `4c50f899f80e` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.6` | 1784-1800 | `f5c6a45807b0` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.5` | 1801-1805 | `1e634d009b80` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.4` | 1806-1811 | `a02b7bf01817` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.3` | 1812-1818 | `b73b838ee71a` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.2` | 1819-1825 | `3aae34cdb8de` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.1` | 1826-1838 | `1c1bfe44fee1` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.0` | 1839-1848 | `01fee319f99c` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v2.2` | 1849-1851 | `947024e547a5` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v2.1` | 1852-1854 | `831353b88b26` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v2.0` | 1855-1860 | `92161592c6f6` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v1.0` | 1861-1862 | `bf9fe6205559` | structural | heading only |
| `DEV-2` | 1863-1899 | `6ff9fac762f9` | planned | docs/RELEASE.md |
| `GROUP-future` | 1900-1908 | `c0c3ec88bb92` | planned | docs/ROADMAP.md |
| `SECTION-03` | 1909-1911 | `8f4b634fd646` | structural | heading only |
| `agent-rules` | 1912-1945 | `f699b67648da` | planned | AGENTS.md (governance) |
| `SECTION-04` | 1946-1950 | `03786d4742d7` | planned | docs/ROADMAP.md |
| `B-OPEN` | 1951-1995 | `4e5f8d3984d1` | planned | openspec/changes (one change per open item) |
| `FUTURE-1` | 1996-2003 | `0a1e1448d12b` | planned | docs/ROADMAP.md |
| `FUTURE-2` | 2004-2008 | `eb05f449bfc6` | planned | docs/ROADMAP.md |
| `FUTURE-3` | 2009-2014 | `2c09d65cfded` | planned | docs/ROADMAP.md |
| `FUTURE-4` | 2015-2018 | `62d8fde09ca2` | planned | docs/ROADMAP.md |
| `FUTURE-5` | 2019-2023 | `041157e3cd11` | planned | docs/ROADMAP.md |
| `FUTURE-6` | 2024-2027 | `a070988b84e8` | planned | docs/ROADMAP.md |
| `FUTURE-7` | 2028-2040 | `116a4980e694` | planned | docs/ROADMAP.md |
| `GROUP-open-questions` | 2041-2064 | `9806de6ee55e` | planned | docs/ROADMAP.md, docs/STATUS.md |
| `BANNER-agent-rules` | 2065-2098 | `1dd939e2384f` | duplicate | `unit:agent-rules` |
| `BANNER-layer-b-implementation-status` | 2099-2119 | `cb181e7e1383` | planned | docs/STATUS.md |
| `SECTION-05` | 2120-2124 | `3950191b4348` | planned | openspec/README.md (history) |
| `ADR-0001` | 2125-2129 | `6597d424187c` | copied | `openspec/changes/archive/2026-05-28-adr-0001-llvm-backend` |
| `ADR-0002` | 2130-2135 | `24673434b56e` | copied | `openspec/changes/archive/2026-05-28-adr-0002-block-syntax` |
| `ADR-0003` | 2136-2139 | `91f21c8db59b` | copied | `openspec/changes/archive/2026-05-28-adr-0003-type-system` |
| `ADR-0004` | 2140-2148 | `a7ea289039f3` | copied | `openspec/changes/archive/2026-05-28-adr-0004-exceptions` |
| `ADR-0005` | 2149-2172 | `b3487abfdaa3` | copied | `openspec/changes/archive/2026-05-31-adr-0005-consolidated-0-1-language-decisions` |
| `ADR-0006` | 2173-2220 | `00635fb17fa5` | copied | `openspec/changes/archive/2026-05-31-adr-0006-inox-0-1-language-constitution` |
| `ADR-0017` | 2221-2246 | `99a0ae3ad662` | copied | `openspec/changes/archive/2026-10-11-adr-0017-read-only-slices` |
| `ADR-0016` | 2247-2278 | `ae224ec3acef` | copied | `openspec/changes/archive/2026-10-11-adr-0016-ref-and-ref-mut-parameters` |
| `ADR-0015` | 2279-2296 | `d138712bb9d2` | copied | `openspec/changes/archive/2026-10-10-adr-0015-storing-into-a-natural-checks-the-range` |
| `ADR-0014` | 2297-2324 | `8f83b1f7aa88` | copied | `openspec/changes/archive/2026-10-10-adr-0014-natural-byte-and-the-closed-numeric-set` |
| `ADR-0013` | 2325-2350 | `8b43a6f9c9e7` | copied | `openspec/changes/archive/2026-10-10-adr-0013-conditional-expression` |
| `ADR-0012` | 2351-2364 | `387bf21df2e0` | copied | `openspec/changes/archive/2026-10-10-adr-0012-compound-assignment` |
| `ADR-0011` | 2365-2397 | `8cd31c3b1d14` | copied | `openspec/changes/archive/2026-10-09-adr-0011-case-in-the-ada-spark-style` |
| `ADR-0010` | 2398-2409 | `01ebe198452d` | copied | `openspec/changes/archive/2026-10-09-adr-0010-step-in-the-for-header` |
| `ADR-0009` | 2410-2425 | `9fb0711acf2b` | copied | `openspec/changes/archive/2026-10-09-adr-0009-chained-assignment` |
| `ADR-0008` | 2426-2443 | `6cbc9717bb6c` | copied | `openspec/changes/archive/2026-10-09-adr-0008-precedence-of-and-in` |
| `ADR-0007` | 2444-2456 | `d8e65f343093` | copied | `openspec/changes/archive/2026-10-09-adr-0007-loop-exit-and-try-cleanup-keywords` |
| `GROUP-development` | 2457-2472 | `6f6455b69203` | structural | heading only |
| `SECTION-06` | 2473-2475 | `1f3fa370b3e3` | structural | heading only |
| `canonical-user-facing-cli-contract` | 2476-2491 | `45e0e5d66ff0` | planned | openspec/specs/compiler/cli |
| `current-observed-driver-status` | 2492-2497 | `15c15b4e82d8` | planned | docs/STATUS.md |
| `SECTION-07` | 2498-2500 | `f2dfbd93d1af` | structural | heading only |
| `source-file-structure` | 2501-2512 | `f905deb42055` | planned | openspec/specs/language/modules |
| `SECTION-08` | 2513-2515 | `83d42d4dfe69` | structural | heading only |
| `CANON-2` | 2516-2547 | `1784cd8f9c86` | planned | openspec/specs/language/lexical |
| `SECTION-09` | 2548-2550 | `151b987c27db` | structural | heading only |
| `CANON-3` | 2551-2571 | `86f97b768bdb` | planned | openspec/specs/language/modules |
| `SECTION-10` | 2572-2574 | `539a6368d8f5` | structural | heading only |
| `CANON-4` | 2575-2630 | `011a17049109` | planned | openspec/specs/language/block-syntax |
| `CANON-7` | 2631-2642 | `954891e7e64b` | planned | openspec/specs/language/block-syntax |
| `SECTION-11` | 2643-2645 | `cc93dd9983b8` | structural | heading only |
| `CANON-5` | 2646-2650 | `107db4bb6efc` | planned | openspec/specs/language/variables |
| `CANON-5/type-section` | 2651-2665 | `ee2b644e002f` | planned | openspec/specs/language/declaration-sections |
| `CANON-5/local-variables-three-valid-forms-only` | 2666-2673 | `7acb630f2600` | planned | openspec/specs/language/variables |
| `CANON-5/grouped-declarations` | 2674-2686 | `211d54982dd6` | planned | openspec/specs/language/variables |
| `CANON-5/compound-assignment` | 2687-2696 | `ac2d6220edcf` | planned | openspec/specs/language/assignment |
| `CANON-5/assignment-is-a-statement-chained-assign` | 2697-2756 | `4a3bbaf10235` | planned | openspec/specs/language/assignment |
| `CANON-5/safe-type-inference` | 2757-2765 | `8cabd03f259a` | planned | openspec/specs/language/variables |
| `CANON-5/const-and-state` | 2766-2775 | `c77d846e37bf` | planned | openspec/specs/language/declaration-sections, openspec/specs/language/module-state |
| `SECTION-12` | 2776-2778 | `21806ebd31bd` | structural | heading only |
| `CANON-8` | 2779-2780 | `44025c9660f2` | structural | heading only |
| `CANON-8/canonical-type-hierarchy` | 2781-2822 | `b64e68e42b5b` | planned | openspec/specs/language/numeric-types, openspec/specs/language/natural, openspec/specs/language/fixed-width-integers |
| `CANON-8/generics-use-angle-brackets` | 2823-2831 | `599f5b0a40cf` | planned | openspec/specs/language/generics |
| `CANON-8/conversions` | 2832-2844 | `233a06f0e0e2` | planned | openspec/specs/language/conversions |
| `CANON-8/decimal-literal-form` | 2845-2864 | `5d29e066b6b5` | planned | openspec/specs/language/decimal-types |
| `CANON-8/operators-and-numeric-law` | 2865-2892 | `4234270d384a` | planned | openspec/specs/language/numeric-types |
| `SECTION-13` | 2893-2903 | `2897fbbce2ae` | planned | openspec/specs/language/variables, openspec/specs/language/mutability |
| `SECTION-13/variable-declaration-rules` | 2904-2907 | `754e4e1781da` | planned | openspec/specs/language/variables |
| `CANON-10` | 2908-2932 | `9e51efaea2ef` | planned | openspec/specs/language/mutability |
| `SECTION-14` | 2933-2935 | `47360f55ede4` | structural | heading only |
| `scoping-and-no-shadowing` | 2936-2948 | `212122b8a58f` | planned | openspec/specs/language/scoping |
| `SECTION-15` | 2949-2951 | `60939e1901e7` | structural | heading only |
| `CANON-6` | 2952-2972 | `deea50879587` | planned | openspec/specs/language/routines |
| `SECTION-16` | 2973-2975 | `70008477d760` | structural | heading only |
| `control-flow-constructs` | 2976-2977 | `49a7482f73bc` | structural | heading only |
| `CANON-12` | 2978-2981 | `778fe3b1480b` | planned | openspec/specs/language/control-flow |
| `CANON-12/if-elif-else` | 2982-2992 | `3cf7facb9c42` | planned | openspec/specs/language/control-flow |
| `CANON-15` | 2993-3142 | `a52eb819fcce` | planned | openspec/specs/language/exceptions |
| `CANON-15/current-v3-17-lowering-note` | 3143-3153 | `0c522c2d339d` | planned | docs/STATUS.md |
| `CANON-11` | 3154-3185 | `ad57cb2fc650` | planned | openspec/specs/language/with-statement |
| `SECTION-17` | 3186-3188 | `f11baea07e11` | structural | heading only |
| `SECTION-17/while` | 3189-3194 | `ba94f83af4d4` | planned | openspec/specs/language/loops |
| `SECTION-17/repeat-until` | 3195-3204 | `45814b3807ec` | planned | openspec/specs/language/loops |
| `SECTION-17/for-in-range` | 3205-3235 | `363715f8a505` | planned | openspec/specs/language/loops |
| `SECTION-17/conditional-expression` | 3236-3244 | `2dceb1930154` | planned | openspec/specs/language/conditional-expression |
| `SECTION-17/case` | 3245-3270 | `6639c7cf4313` | mirrored | `openspec/specs/language/case-statement/spec.md` (Enum part: change enum-range-set) |
| `SECTION-17/unless` | 3271-3275 | `c75b89637df7` | mirrored | `openspec/specs/language/case-statement/spec.md` |
| `SECTION-18` | 3276-3278 | `e522f90c7379` | structural | heading only |
| `CANON-9` | 3279-3320 | `8a021bf1ff92` | planned | openspec/specs/language/structs, openspec/specs/language/associated-methods |
| `CANON-9/named-struct-construction` | 3321-3357 | `d0090df0b373` | planned | openspec/specs/language/structs |
| `SECTION-19` | 3358-3360 | `e0d3647383a1` | structural | heading only |
| `CANON-14` | 3361-3389 | `3bb68612b459` | planned | openspec/specs/language/strings |
| `SECTION-20` | 3390-3394 | `3b560adced11` | planned | openspec/specs/language/numeric-types |
| `numeric-semantics-summary` | 3395-3405 | `bda5b68d9240` | planned | openspec/specs/language/numeric-types |
| `CANON-20` | 3406-3413 | `b20c1473c6e9` | planned | openspec/specs/language/operator-precedence |
| `CANON-20/design-intent` | 3414-3422 | `b9c61f6ebef4` | planned | openspec/specs/language/operator-precedence |
| `CANON-20/precedence-table` | 3423-3459 | `606ca4e1d824` | planned | openspec/specs/language/operator-precedence |
| `CANON-20/mandatory-parentheses-rule` | 3460-3480 | `6d49217d2355` | planned | openspec/specs/language/operator-precedence |
| `CANON-20/verifiability` | 3481-3496 | `40b3313dba9d` | planned | openspec/specs/language/operator-precedence |
| `SECTION-21` | 3497-3499 | `ec02c6ed9865` | structural | heading only |
| `CANON-13` | 3500-3501 | `f59f57e57286` | structural | heading only |
| `CANON-13/array` | 3502-3509 | `f2d44c0dd686` | mirrored | `openspec/specs/language/fixed-arrays/spec.md` |
| `CANON-13/vector` | 3510-3516 | `c99a2607fa26` | planned | openspec/specs/language/vectors |
| `CANON-13/range` | 3517-3522 | `c7b61a41d8eb` | planned | openspec/specs/language/ranges |
| `CANON-13/enum` | 3523-3536 | `2d11b4cc7dc3` | planned | openspec/specs/language/enums |
| `CANON-13/set` | 3537-3548 | `240f92fbca14` | planned | openspec/specs/language/sets |
| `SECTION-22` | 3549-3551 | `f6a146590eb2` | structural | heading only |
| `CANON-18` | 3552-3595 | `3c45aca17928` | planned | openspec/specs/stdlib/standard-library |
| `CANON-17` | 3596-3631 | `476882e140b5` | planned | openspec/specs/stdlib/console-io |
| `CANON-17/files` | 3632-3666 | `9f1eb3536845` | planned | openspec/specs/language (capability to choose) |
| `SECTION-23` | 3667-3669 | `f29a82eefbab` | structural | heading only |
| `CANON-19` | 3670-3671 | `2682b6b517b1` | structural | heading only |
| `CANON-19/bootstrap-exception-runtime` | 3672-3707 | `3732c1eb5dfc` | planned | openspec/specs/runtime/exception-runtime |
| `CANON-19/checked-integer-arithmetic` | 3708-3784 | `8b1088d2777d` | planned | openspec/specs/runtime/runtime-faults |
| `CANON-19/compiler-implementation-limits` | 3785-3804 | `4564c0b0dffc` | planned | openspec/specs/compiler/limits |
| `temporary-console-i-o-abi` | 3805-3816 | `ee67f38f23b2` | planned | docs/STATUS.md |
| `SECTION-24` | 3817-3819 | `d83447a408b7` | structural | heading only |
| `CANON-20~2` | 3820-3850 | `89bc9490baa9` | planned | openspec/specs/compiler/llvm-backend |
| `current-backend-support-status` | 3851-3852 | `92e4d170aa16` | structural | heading only |
| `B-WORKS` | 3853-3872 | `05b04ae6d0f9` | planned | docs/STATUS.md |
| `B-PARSED` | 3873-3878 | `010d10c5ad64` | planned | docs/STATUS.md |
| `B-GAPS` | 3879-3949 | `376488d68493` | planned | docs/STATUS.md |
| `SECTION-25` | 3950-3952 | `561c278c3296` | structural | heading only |
| `examples-policy` | 3953-3962 | `1ef48ef87dbf` | planned | docs/ENGINEERING.md |
| `B-EXAMPLES` | 3963-3971 | `3d9b7dd4c5e3` | planned | docs/STATUS.md |
| `SECTION-26` | 3972-3974 | `9a88a697ae99` | structural | heading only |
| `CANON-21` | 3975-4008 | `afab1f61829e` | planned | docs/ENGINEERING.md |
| `required-regression-layers` | 4009-4018 | `6098f4656540` | planned | docs/ENGINEERING.md |
| `SECTION-27` | 4019-4021 | `62cc5be85e73` | structural | heading only |
| `release-validation-policy` | 4022-4040 | `c2bd42d65889` | planned | docs/RELEASE.md |
| `DEV-2~2` | 4041-4077 | `6ff9fac762f9` | duplicate | `unit:DEV-2` |
| `GROUP-future~2` | 4078-4086 | `c0c3ec88bb92` | duplicate | `unit:GROUP-future` |
| `SECTION-28` | 4087-4089 | `b97b11b56381` | structural | heading only |
| `documentation-generation-policy` | 4090-4102 | `1d33470c8cd5` | planned | docs/ENGINEERING.md |
| `SECTION-29` | 4103-4105 | `000dd38c7ed9` | structural | heading only |
| `DEV-1` | 4106-4143 | `48712f26b690` | planned | docs/RELEASE.md |
| `portability-targets` | 4144-4190 | `2422ef6e3b0a` | planned | docs/RELEASE.md |
| `SECTION-30` | 4191-4195 | `683f7b9b00c7` | planned | docs/STATUS.md |
| `B-WORKS~2` | 4196-4212 | `65db813fde18` | planned | docs/STATUS.md |
| `B-PARSED~2` | 4213-4218 | `010d10c5ad64` | planned | docs/STATUS.md |
| `B-GAPS~2` | 4219-4287 | `a5ea1b1eb65f` | planned | docs/STATUS.md |
| `B-CONFLICTS` | 4288-4295 | `fdc28764bc4a` | planned | docs/STATUS.md |
| `B-EXAMPLES~2` | 4296-4302 | `f85a9a4e4be5` | planned | docs/STATUS.md |
| `B-PROPOSALS` | 4303-4311 | `b5622600ecc3` | planned | openspec/changes (one change per open item) |
| `SECTION-31` | 4312-4314 | `0c7c92ea754c` | structural | heading only |
| `CANON-22` | 4315-4334 | `0fd628df7854` | planned | docs/ROADMAP.md |
| `GROUP-decisions` | 4335-4342 | `8b90cf0e6ca5` | planned | openspec/README.md (history) |
| `roadmap-to-1-0` | 4343-4357 | `de99ff09125d` | planned | docs/ROADMAP.md |
| `backend-strategy-directive` | 4358-4364 | `3e733152683f` | planned | docs/ENGINEERING.md |
| `backend-strategy-directive/core-principle` | 4365-4378 | `97d3972ccf1e` | planned | docs/ENGINEERING.md |
| `backend-strategy-directive/backend-maturation-sequence` | 4379-4394 | `ff2dbe76d57a` | planned | docs/ENGINEERING.md |
| `backend-strategy-directive/objective-criterion-for-leaving-clang` | 4395-4409 | `1cda38393426` | planned | docs/ENGINEERING.md |
| `backend-strategy-directive/scope-guard-for-backend-adjacent-work` | 4410-4416 | `859bab429005` | planned | docs/ENGINEERING.md |
| `compiler-module-architecture-directive` | 4417-4423 | `7dfc746e78b5` | planned | docs/ENGINEERING.md |
| `compiler-module-architecture-directive/principle-reactive-not-predictive` | 4424-4432 | `6bdbaef0d48c` | planned | docs/ENGINEERING.md |
| `compiler-module-architecture-directive/the-three-tests-for-extracting-a-module` | 4433-4448 | `9bdd4cb899e6` | planned | docs/ENGINEERING.md |
| `compiler-module-architecture-directive/anti-rules` | 4449-4457 | `c8700ead7c93` | planned | docs/ENGINEERING.md |
| `compiler-module-architecture-directive/working-rule-of-thumb` | 4458-4462 | `ec2c782d1c49` | planned | docs/ENGINEERING.md |
| `compiler-module-architecture-directive/timing` | 4463-4469 | `c4c0fef11f84` | planned | docs/ENGINEERING.md |
| `BANNER-end-of-inox-canonical-md-v3-8` | 4470-4475 | `784433b7be07` | planned | openspec/README.md (reading order) |
