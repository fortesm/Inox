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
| `BANNER-the-single-canonical-document` | 3-56 | `d69aad8215ec` | planned | openspec/README.md (authority, maintainer, license, map) |
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
| `E19` | 359-392 | `a29f5aaa1da4` | planned | docs/ENGINEERING.md |
| `E20` | 393-401 | `3cbd923272e0` | planned | docs/ENGINEERING.md |
| `non-negotiable-rules` | 402-413 | `ace55d5cb612` | planned | AGENTS.md (governance) |
| `SECTION-01` | 414-416 | `8761a2a5882c` | structural | heading only |
| `CANON-1` | 417-425 | `db04e9c70c16` | planned | docs/VISION.md |
| `CANON-1/language-references-and-influences` | 426-444 | `f1408ec493d5` | planned | docs/VISION.md |
| `CANON-1/mission-critical-target-domain` | 445-457 | `453687b19f4a` | planned | docs/VISION.md |
| `CANON-1/core-safety-stance` | 458-472 | `1d21e14e00f1` | planned | docs/VISION.md |
| `CANON-1/post-object-oriented-design` | 473-480 | `184e39a5df22` | planned | docs/VISION.md |
| `CANON-1/performance-model` | 481-490 | `d6a3455f8387` | planned | docs/VISION.md |
| `CANON-1/agent-rule` | 491-497 | `7117205f7492` | planned | docs/VISION.md |
| `SECTION-02` | 498-500 | `b705068ef5b9` | structural | heading only |
| `change-log` | 501-503 | `0fab86654403` | structural | heading only |
| `LOG/v3.50` | 504-528 | `0441fbff3bf5` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.49` | 529-555 | `be098b1b06bd` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.48` | 556-577 | `afa50d5ec69f` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.47` | 578-590 | `72a0e9ea7e82` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.46` | 591-604 | `07eee37e2b47` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.45` | 605-633 | `e8bf1bc74e25` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.44` | 634-656 | `1ed18d87a1f6` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.43` | 657-666 | `7cec90575743` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.42` | 667-682 | `e00cff8892ed` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.41` | 683-688 | `d9b78ef08a85` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.40` | 689-707 | `3c2103c842ec` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.39` | 708-737 | `ff8a1f06c34f` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.38` | 738-748 | `d97e13c1b814` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.37` | 749-760 | `73f3f0239390` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.36` | 761-777 | `889932b86d67` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.35` | 778-795 | `a40c544968ff` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.34` | 796-808 | `fa31c1e6a95f` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.33` | 809-831 | `546628eeb9d8` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.32` | 832-853 | `01b8acd407b7` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.31` | 854-867 | `c6f79fbad05a` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.30` | 868-889 | `82fc2948b1b0` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.29` | 890-950 | `411404a57a61` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.28` | 951-986 | `f95a9348aa62` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.27` | 987-1042 | `9a521127b697` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.26` | 1043-1062 | `d412b402bbfb` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.25` | 1063-1075 | `ac5b09d96e50` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.24` | 1076-1098 | `977e3c458a59` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.23` | 1099-1138 | `1bd9020135e7` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.22` | 1139-1153 | `c3cd869369d8` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.21` | 1154-1218 | `fa631814dc07` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.20` | 1219-1247 | `c178428e9e51` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.19` | 1248-1263 | `58a9449923cc` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.18` | 1264-1342 | `63b0850a3b94` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.17` | 1343-1363 | `6b82cfa2bfef` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.16` | 1364-1384 | `504ebe528c6f` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.15a` | 1385-1398 | `8a99974e3b15` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.15` | 1399-1413 | `db45cccf8f29` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.14` | 1414-1424 | `45d2e2e7b0f4` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.13` | 1425-1436 | `e78d99b40dce` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.12` | 1437-1446 | `adf42d99bf9c` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.11` | 1447-1456 | `bb084ddde131` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.10` | 1457-1469 | `bb0c5292dc58` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.8` | 1470-1480 | `e2021267fde4` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.7` | 1481-1491 | `4c50f899f80e` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.6` | 1492-1508 | `f5c6a45807b0` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.5` | 1509-1513 | `1e634d009b80` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.4` | 1514-1519 | `a02b7bf01817` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.3` | 1520-1526 | `b73b838ee71a` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.2` | 1527-1533 | `3aae34cdb8de` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.1` | 1534-1546 | `1c1bfe44fee1` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.0` | 1547-1556 | `01fee319f99c` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v2.2` | 1557-1559 | `947024e547a5` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v2.1` | 1560-1562 | `831353b88b26` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v2.0` | 1563-1568 | `92161592c6f6` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v1.0` | 1569-1570 | `bf9fe6205559` | structural | heading only |
| `DEV-2` | 1571-1607 | `6ff9fac762f9` | planned | docs/RELEASE.md |
| `GROUP-future` | 1608-1616 | `c0c3ec88bb92` | planned | docs/ROADMAP.md |
| `SECTION-03` | 1617-1619 | `8f4b634fd646` | structural | heading only |
| `agent-rules` | 1620-1653 | `f699b67648da` | planned | AGENTS.md (governance) |
| `SECTION-04` | 1654-1658 | `03786d4742d7` | planned | docs/ROADMAP.md |
| `B-OPEN` | 1659-1703 | `4e5f8d3984d1` | planned | openspec/changes (one change per open item) |
| `FUTURE-1` | 1704-1711 | `0a1e1448d12b` | planned | docs/ROADMAP.md |
| `FUTURE-2` | 1712-1716 | `eb05f449bfc6` | planned | docs/ROADMAP.md |
| `FUTURE-3` | 1717-1722 | `2c09d65cfded` | planned | docs/ROADMAP.md |
| `FUTURE-4` | 1723-1726 | `62d8fde09ca2` | planned | docs/ROADMAP.md |
| `FUTURE-5` | 1727-1731 | `041157e3cd11` | planned | docs/ROADMAP.md |
| `FUTURE-6` | 1732-1735 | `a070988b84e8` | planned | docs/ROADMAP.md |
| `FUTURE-7` | 1736-1748 | `116a4980e694` | planned | docs/ROADMAP.md |
| `GROUP-open-questions` | 1749-1772 | `9806de6ee55e` | planned | docs/ROADMAP.md, docs/STATUS.md |
| `BANNER-agent-rules` | 1773-1806 | `1dd939e2384f` | duplicate | `unit:agent-rules` |
| `BANNER-layer-b-implementation-status` | 1807-1827 | `cb181e7e1383` | planned | docs/STATUS.md |
| `SECTION-05` | 1828-1832 | `3950191b4348` | planned | openspec/README.md (history) |
| `ADR-0001` | 1833-1837 | `6597d424187c` | copied | `openspec/changes/archive/2026-05-28-adr-0001-llvm-backend` |
| `ADR-0002` | 1838-1843 | `24673434b56e` | copied | `openspec/changes/archive/2026-05-28-adr-0002-block-syntax` |
| `ADR-0003` | 1844-1847 | `91f21c8db59b` | copied | `openspec/changes/archive/2026-05-28-adr-0003-type-system` |
| `ADR-0004` | 1848-1856 | `a7ea289039f3` | copied | `openspec/changes/archive/2026-05-28-adr-0004-exceptions` |
| `ADR-0005` | 1857-1880 | `b3487abfdaa3` | copied | `openspec/changes/archive/2026-05-31-adr-0005-consolidated-0-1-language-decisions` |
| `ADR-0006` | 1881-1928 | `00635fb17fa5` | copied | `openspec/changes/archive/2026-05-31-adr-0006-inox-0-1-language-constitution` |
| `ADR-0017` | 1929-1954 | `99a0ae3ad662` | copied | `openspec/changes/archive/2026-10-11-adr-0017-read-only-slices` |
| `ADR-0016` | 1955-1986 | `ae224ec3acef` | copied | `openspec/changes/archive/2026-10-11-adr-0016-ref-and-ref-mut-parameters` |
| `ADR-0015` | 1987-2004 | `d138712bb9d2` | copied | `openspec/changes/archive/2026-10-10-adr-0015-storing-into-a-natural-checks-the-range` |
| `ADR-0014` | 2005-2032 | `8f83b1f7aa88` | copied | `openspec/changes/archive/2026-10-10-adr-0014-natural-byte-and-the-closed-numeric-set` |
| `ADR-0013` | 2033-2058 | `8b43a6f9c9e7` | copied | `openspec/changes/archive/2026-10-10-adr-0013-conditional-expression` |
| `ADR-0012` | 2059-2072 | `387bf21df2e0` | copied | `openspec/changes/archive/2026-10-10-adr-0012-compound-assignment` |
| `ADR-0011` | 2073-2105 | `8cd31c3b1d14` | copied | `openspec/changes/archive/2026-10-09-adr-0011-case-in-the-ada-spark-style` |
| `ADR-0010` | 2106-2117 | `01ebe198452d` | copied | `openspec/changes/archive/2026-10-09-adr-0010-step-in-the-for-header` |
| `ADR-0009` | 2118-2133 | `9fb0711acf2b` | copied | `openspec/changes/archive/2026-10-09-adr-0009-chained-assignment` |
| `ADR-0008` | 2134-2151 | `6cbc9717bb6c` | copied | `openspec/changes/archive/2026-10-09-adr-0008-precedence-of-and-in` |
| `ADR-0007` | 2152-2164 | `d8e65f343093` | copied | `openspec/changes/archive/2026-10-09-adr-0007-loop-exit-and-try-cleanup-keywords` |
| `GROUP-development` | 2165-2180 | `6f6455b69203` | structural | heading only |
| `SECTION-06` | 2181-2183 | `1f3fa370b3e3` | structural | heading only |
| `canonical-user-facing-cli-contract` | 2184-2199 | `45e0e5d66ff0` | planned | openspec/specs/compiler/cli |
| `current-observed-driver-status` | 2200-2205 | `15c15b4e82d8` | planned | docs/STATUS.md |
| `SECTION-07` | 2206-2208 | `f2dfbd93d1af` | structural | heading only |
| `source-file-structure` | 2209-2220 | `f905deb42055` | planned | openspec/specs/language/modules |
| `SECTION-08` | 2221-2223 | `83d42d4dfe69` | structural | heading only |
| `CANON-2` | 2224-2255 | `1784cd8f9c86` | planned | openspec/specs/language/lexical |
| `SECTION-09` | 2256-2258 | `151b987c27db` | structural | heading only |
| `CANON-3` | 2259-2279 | `eb3765442875` | planned | openspec/specs/language/modules |
| `SECTION-10` | 2280-2282 | `539a6368d8f5` | structural | heading only |
| `CANON-4` | 2283-2338 | `011a17049109` | planned | openspec/specs/language/block-syntax |
| `CANON-7` | 2339-2350 | `954891e7e64b` | planned | openspec/specs/language/block-syntax |
| `SECTION-11` | 2351-2353 | `cc93dd9983b8` | structural | heading only |
| `CANON-5` | 2354-2358 | `107db4bb6efc` | planned | openspec/specs/language/variables |
| `CANON-5/type-section` | 2359-2373 | `ee2b644e002f` | planned | openspec/specs/language/declaration-sections |
| `CANON-5/local-variables-three-valid-forms-only` | 2374-2381 | `7acb630f2600` | planned | openspec/specs/language/variables |
| `CANON-5/grouped-declarations` | 2382-2394 | `211d54982dd6` | planned | openspec/specs/language/variables |
| `CANON-5/compound-assignment` | 2395-2404 | `ac2d6220edcf` | planned | openspec/specs/language/assignment |
| `CANON-5/assignment-is-a-statement-chained-assign` | 2405-2464 | `4a3bbaf10235` | planned | openspec/specs/language/assignment |
| `CANON-5/safe-type-inference` | 2465-2473 | `8cabd03f259a` | planned | openspec/specs/language/variables |
| `CANON-5/const-and-state` | 2474-2483 | `c77d846e37bf` | planned | openspec/specs/language/declaration-sections, openspec/specs/language/module-state |
| `SECTION-12` | 2484-2486 | `21806ebd31bd` | structural | heading only |
| `CANON-8` | 2487-2488 | `44025c9660f2` | structural | heading only |
| `CANON-8/canonical-type-hierarchy` | 2489-2529 | `3d044d5e00c7` | planned | openspec/specs/language/numeric-types, openspec/specs/language/natural, openspec/specs/language/fixed-width-integers |
| `CANON-8/generics-use-angle-brackets` | 2530-2538 | `599f5b0a40cf` | planned | openspec/specs/language/generics |
| `CANON-8/conversions` | 2539-2551 | `233a06f0e0e2` | planned | openspec/specs/language/conversions |
| `CANON-8/decimal-literal-form` | 2552-2571 | `5d29e066b6b5` | planned | openspec/specs/language/decimal-types |
| `CANON-8/operators-and-numeric-law` | 2572-2599 | `4234270d384a` | planned | openspec/specs/language/numeric-types |
| `SECTION-13` | 2600-2610 | `2897fbbce2ae` | planned | openspec/specs/language/variables, openspec/specs/language/mutability |
| `SECTION-13/variable-declaration-rules` | 2611-2614 | `754e4e1781da` | planned | openspec/specs/language/variables |
| `CANON-10` | 2615-2639 | `9e51efaea2ef` | planned | openspec/specs/language/mutability |
| `SECTION-14` | 2640-2642 | `47360f55ede4` | structural | heading only |
| `scoping-and-no-shadowing` | 2643-2655 | `212122b8a58f` | planned | openspec/specs/language/scoping |
| `SECTION-15` | 2656-2658 | `60939e1901e7` | structural | heading only |
| `CANON-6` | 2659-2679 | `deea50879587` | planned | openspec/specs/language/routines |
| `SECTION-16` | 2680-2682 | `70008477d760` | structural | heading only |
| `control-flow-constructs` | 2683-2684 | `49a7482f73bc` | structural | heading only |
| `CANON-12` | 2685-2688 | `778fe3b1480b` | planned | openspec/specs/language/control-flow |
| `CANON-12/if-elif-else` | 2689-2699 | `3cf7facb9c42` | planned | openspec/specs/language/control-flow |
| `CANON-15` | 2700-2849 | `a52eb819fcce` | planned | openspec/specs/language/exceptions |
| `CANON-15/current-v3-17-lowering-note` | 2850-2860 | `0c522c2d339d` | planned | docs/STATUS.md |
| `CANON-11` | 2861-2892 | `ad57cb2fc650` | planned | openspec/specs/language/with-statement |
| `SECTION-17` | 2893-2895 | `f11baea07e11` | structural | heading only |
| `SECTION-17/while` | 2896-2901 | `ba94f83af4d4` | planned | openspec/specs/language/loops |
| `SECTION-17/repeat-until` | 2902-2911 | `45814b3807ec` | planned | openspec/specs/language/loops |
| `SECTION-17/for-in-range` | 2912-2942 | `363715f8a505` | planned | openspec/specs/language/loops |
| `SECTION-17/conditional-expression` | 2943-2951 | `2dceb1930154` | planned | openspec/specs/language/conditional-expression |
| `SECTION-17/case` | 2952-2977 | `6639c7cf4313` | mirrored | `openspec/specs/language/case-statement/spec.md` (Enum part: change enum-range-set) |
| `SECTION-17/unless` | 2978-2982 | `c75b89637df7` | mirrored | `openspec/specs/language/case-statement/spec.md` |
| `SECTION-18` | 2983-2985 | `e522f90c7379` | structural | heading only |
| `CANON-9` | 2986-3025 | `d5eec8fbbc36` | planned | openspec/specs/language/structs, openspec/specs/language/associated-methods |
| `CANON-9/named-struct-construction` | 3026-3062 | `d0090df0b373` | planned | openspec/specs/language/structs |
| `SECTION-19` | 3063-3065 | `e0d3647383a1` | structural | heading only |
| `CANON-14` | 3066-3082 | `e5499ac61e11` | planned | openspec/specs/language/strings |
| `SECTION-20` | 3083-3087 | `3b560adced11` | planned | openspec/specs/language/numeric-types |
| `numeric-semantics-summary` | 3088-3098 | `bda5b68d9240` | planned | openspec/specs/language/numeric-types |
| `CANON-20` | 3099-3106 | `b20c1473c6e9` | planned | openspec/specs/language/operator-precedence |
| `CANON-20/design-intent` | 3107-3115 | `b9c61f6ebef4` | planned | openspec/specs/language/operator-precedence |
| `CANON-20/precedence-table` | 3116-3152 | `606ca4e1d824` | planned | openspec/specs/language/operator-precedence |
| `CANON-20/mandatory-parentheses-rule` | 3153-3173 | `6d49217d2355` | planned | openspec/specs/language/operator-precedence |
| `CANON-20/verifiability` | 3174-3189 | `40b3313dba9d` | planned | openspec/specs/language/operator-precedence |
| `SECTION-21` | 3190-3192 | `ec02c6ed9865` | structural | heading only |
| `CANON-13` | 3193-3194 | `f59f57e57286` | structural | heading only |
| `CANON-13/array` | 3195-3202 | `f2d44c0dd686` | mirrored | `openspec/specs/language/fixed-arrays/spec.md` |
| `CANON-13/vector` | 3203-3209 | `c99a2607fa26` | planned | openspec/specs/language/vectors |
| `CANON-13/range` | 3210-3215 | `c7b61a41d8eb` | planned | openspec/specs/language/ranges |
| `CANON-13/enum` | 3216-3229 | `2d11b4cc7dc3` | planned | openspec/specs/language/enums |
| `CANON-13/set` | 3230-3241 | `240f92fbca14` | planned | openspec/specs/language/sets |
| `SECTION-22` | 3242-3244 | `f6a146590eb2` | structural | heading only |
| `CANON-18` | 3245-3265 | `c798859f64fa` | planned | openspec/specs/stdlib/standard-library |
| `CANON-17` | 3266-3298 | `b58a10142c44` | planned | openspec/specs/stdlib/console-io |
| `SECTION-23` | 3299-3301 | `f29a82eefbab` | structural | heading only |
| `CANON-19` | 3302-3303 | `2682b6b517b1` | structural | heading only |
| `CANON-19/bootstrap-exception-runtime` | 3304-3339 | `3732c1eb5dfc` | planned | openspec/specs/runtime/exception-runtime |
| `CANON-19/checked-integer-arithmetic` | 3340-3414 | `76e58b94adb4` | planned | openspec/specs/runtime/runtime-faults |
| `CANON-19/compiler-implementation-limits` | 3415-3434 | `4564c0b0dffc` | planned | openspec/specs/compiler/limits |
| `temporary-console-i-o-abi` | 3435-3446 | `ee67f38f23b2` | planned | docs/STATUS.md |
| `SECTION-24` | 3447-3449 | `d83447a408b7` | structural | heading only |
| `CANON-20~2` | 3450-3480 | `89bc9490baa9` | planned | openspec/specs/compiler/llvm-backend |
| `current-backend-support-status` | 3481-3482 | `92e4d170aa16` | structural | heading only |
| `B-WORKS` | 3483-3502 | `05b04ae6d0f9` | planned | docs/STATUS.md |
| `B-PARSED` | 3503-3508 | `010d10c5ad64` | planned | docs/STATUS.md |
| `B-GAPS` | 3509-3578 | `f0ab0c6bcde9` | planned | docs/STATUS.md |
| `SECTION-25` | 3579-3581 | `561c278c3296` | structural | heading only |
| `examples-policy` | 3582-3591 | `1ef48ef87dbf` | planned | docs/ENGINEERING.md |
| `B-EXAMPLES` | 3592-3600 | `3d9b7dd4c5e3` | planned | docs/STATUS.md |
| `SECTION-26` | 3601-3603 | `9a88a697ae99` | structural | heading only |
| `CANON-21` | 3604-3637 | `afab1f61829e` | planned | docs/ENGINEERING.md |
| `required-regression-layers` | 3638-3647 | `6098f4656540` | planned | docs/ENGINEERING.md |
| `SECTION-27` | 3648-3650 | `62cc5be85e73` | structural | heading only |
| `release-validation-policy` | 3651-3669 | `c2bd42d65889` | planned | docs/RELEASE.md |
| `DEV-2~2` | 3670-3706 | `6ff9fac762f9` | duplicate | `unit:DEV-2` |
| `GROUP-future~2` | 3707-3715 | `c0c3ec88bb92` | duplicate | `unit:GROUP-future` |
| `SECTION-28` | 3716-3718 | `b97b11b56381` | structural | heading only |
| `documentation-generation-policy` | 3719-3731 | `1d33470c8cd5` | planned | docs/ENGINEERING.md |
| `SECTION-29` | 3732-3734 | `000dd38c7ed9` | structural | heading only |
| `DEV-1` | 3735-3772 | `48712f26b690` | planned | docs/RELEASE.md |
| `portability-targets` | 3773-3819 | `2422ef6e3b0a` | planned | docs/RELEASE.md |
| `SECTION-30` | 3820-3824 | `683f7b9b00c7` | planned | docs/STATUS.md |
| `B-WORKS~2` | 3825-3841 | `65db813fde18` | planned | docs/STATUS.md |
| `B-PARSED~2` | 3842-3847 | `010d10c5ad64` | planned | docs/STATUS.md |
| `B-GAPS~2` | 3848-3915 | `c43ed6cd6801` | planned | docs/STATUS.md |
| `B-CONFLICTS` | 3916-3923 | `fdc28764bc4a` | planned | docs/STATUS.md |
| `B-EXAMPLES~2` | 3924-3930 | `f85a9a4e4be5` | planned | docs/STATUS.md |
| `B-PROPOSALS` | 3931-3939 | `b5622600ecc3` | planned | openspec/changes (one change per open item) |
| `SECTION-31` | 3940-3942 | `0c7c92ea754c` | structural | heading only |
| `CANON-22` | 3943-3962 | `0fd628df7854` | planned | docs/ROADMAP.md |
| `GROUP-decisions` | 3963-3970 | `8b90cf0e6ca5` | planned | openspec/README.md (history) |
| `roadmap-to-1-0` | 3971-3985 | `de99ff09125d` | planned | docs/ROADMAP.md |
| `backend-strategy-directive` | 3986-3992 | `3e733152683f` | planned | docs/ENGINEERING.md |
| `backend-strategy-directive/core-principle` | 3993-4006 | `97d3972ccf1e` | planned | docs/ENGINEERING.md |
| `backend-strategy-directive/backend-maturation-sequence` | 4007-4022 | `ff2dbe76d57a` | planned | docs/ENGINEERING.md |
| `backend-strategy-directive/objective-criterion-for-leaving-clang` | 4023-4037 | `1cda38393426` | planned | docs/ENGINEERING.md |
| `backend-strategy-directive/scope-guard-for-backend-adjacent-work` | 4038-4044 | `859bab429005` | planned | docs/ENGINEERING.md |
| `compiler-module-architecture-directive` | 4045-4051 | `7dfc746e78b5` | planned | docs/ENGINEERING.md |
| `compiler-module-architecture-directive/principle-reactive-not-predictive` | 4052-4060 | `6bdbaef0d48c` | planned | docs/ENGINEERING.md |
| `compiler-module-architecture-directive/the-three-tests-for-extracting-a-module` | 4061-4076 | `9bdd4cb899e6` | planned | docs/ENGINEERING.md |
| `compiler-module-architecture-directive/anti-rules` | 4077-4085 | `c8700ead7c93` | planned | docs/ENGINEERING.md |
| `compiler-module-architecture-directive/working-rule-of-thumb` | 4086-4090 | `ec2c782d1c49` | planned | docs/ENGINEERING.md |
| `compiler-module-architecture-directive/timing` | 4091-4097 | `c4c0fef11f84` | planned | docs/ENGINEERING.md |
| `BANNER-end-of-inox-canonical-md-v3-8` | 4098-4103 | `784433b7be07` | planned | openspec/README.md (reading order) |
