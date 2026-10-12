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
| `BANNER-the-single-canonical-document` | 3-56 | `f71cfd2d7206` | planned | openspec/README.md (authority, maintainer, license, map) |
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
| `LOG/v3.53` | 504-534 | `7d280ce2326d` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.52` | 535-565 | `a7dbe03eaf28` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.51` | 566-572 | `7e14e9ffa7a9` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.50` | 573-597 | `0441fbff3bf5` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.49` | 598-624 | `be098b1b06bd` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.48` | 625-646 | `afa50d5ec69f` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.47` | 647-659 | `72a0e9ea7e82` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.46` | 660-673 | `07eee37e2b47` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.45` | 674-702 | `e8bf1bc74e25` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.44` | 703-725 | `1ed18d87a1f6` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.43` | 726-735 | `7cec90575743` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.42` | 736-751 | `e00cff8892ed` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.41` | 752-757 | `d9b78ef08a85` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.40` | 758-776 | `3c2103c842ec` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.39` | 777-806 | `ff8a1f06c34f` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.38` | 807-817 | `d97e13c1b814` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.37` | 818-829 | `73f3f0239390` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.36` | 830-846 | `889932b86d67` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.35` | 847-864 | `a40c544968ff` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.34` | 865-877 | `fa31c1e6a95f` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.33` | 878-900 | `546628eeb9d8` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.32` | 901-922 | `01b8acd407b7` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.31` | 923-936 | `c6f79fbad05a` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.30` | 937-958 | `82fc2948b1b0` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.29` | 959-1019 | `411404a57a61` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.28` | 1020-1055 | `f95a9348aa62` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.27` | 1056-1111 | `9a521127b697` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.26` | 1112-1131 | `d412b402bbfb` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.25` | 1132-1144 | `ac5b09d96e50` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.24` | 1145-1167 | `977e3c458a59` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.23` | 1168-1207 | `1bd9020135e7` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.22` | 1208-1222 | `c3cd869369d8` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.21` | 1223-1287 | `fa631814dc07` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.20` | 1288-1316 | `c178428e9e51` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.19` | 1317-1332 | `58a9449923cc` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.18` | 1333-1411 | `63b0850a3b94` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.17` | 1412-1432 | `6b82cfa2bfef` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.16` | 1433-1453 | `504ebe528c6f` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.15a` | 1454-1467 | `8a99974e3b15` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.15` | 1468-1482 | `db45cccf8f29` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.14` | 1483-1493 | `45d2e2e7b0f4` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.13` | 1494-1505 | `e78d99b40dce` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.12` | 1506-1515 | `adf42d99bf9c` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.11` | 1516-1525 | `bb084ddde131` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.10` | 1526-1538 | `bb0c5292dc58` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.8` | 1539-1549 | `e2021267fde4` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.7` | 1550-1560 | `4c50f899f80e` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.6` | 1561-1577 | `f5c6a45807b0` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.5` | 1578-1582 | `1e634d009b80` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.4` | 1583-1588 | `a02b7bf01817` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.3` | 1589-1595 | `b73b838ee71a` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.2` | 1596-1602 | `3aae34cdb8de` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.1` | 1603-1615 | `1c1bfe44fee1` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.0` | 1616-1625 | `01fee319f99c` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v2.2` | 1626-1628 | `947024e547a5` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v2.1` | 1629-1631 | `831353b88b26` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v2.0` | 1632-1637 | `92161592c6f6` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v1.0` | 1638-1639 | `bf9fe6205559` | structural | heading only |
| `DEV-2` | 1640-1676 | `6ff9fac762f9` | planned | docs/RELEASE.md |
| `GROUP-future` | 1677-1685 | `c0c3ec88bb92` | planned | docs/ROADMAP.md |
| `SECTION-03` | 1686-1688 | `8f4b634fd646` | structural | heading only |
| `agent-rules` | 1689-1722 | `f699b67648da` | planned | AGENTS.md (governance) |
| `SECTION-04` | 1723-1727 | `03786d4742d7` | planned | docs/ROADMAP.md |
| `B-OPEN` | 1728-1772 | `4e5f8d3984d1` | planned | openspec/changes (one change per open item) |
| `FUTURE-1` | 1773-1780 | `0a1e1448d12b` | planned | docs/ROADMAP.md |
| `FUTURE-2` | 1781-1785 | `eb05f449bfc6` | planned | docs/ROADMAP.md |
| `FUTURE-3` | 1786-1791 | `2c09d65cfded` | planned | docs/ROADMAP.md |
| `FUTURE-4` | 1792-1795 | `62d8fde09ca2` | planned | docs/ROADMAP.md |
| `FUTURE-5` | 1796-1800 | `041157e3cd11` | planned | docs/ROADMAP.md |
| `FUTURE-6` | 1801-1804 | `a070988b84e8` | planned | docs/ROADMAP.md |
| `FUTURE-7` | 1805-1817 | `116a4980e694` | planned | docs/ROADMAP.md |
| `GROUP-open-questions` | 1818-1841 | `9806de6ee55e` | planned | docs/ROADMAP.md, docs/STATUS.md |
| `BANNER-agent-rules` | 1842-1875 | `1dd939e2384f` | duplicate | `unit:agent-rules` |
| `BANNER-layer-b-implementation-status` | 1876-1896 | `cb181e7e1383` | planned | docs/STATUS.md |
| `SECTION-05` | 1897-1901 | `3950191b4348` | planned | openspec/README.md (history) |
| `ADR-0001` | 1902-1906 | `6597d424187c` | copied | `openspec/changes/archive/2026-05-28-adr-0001-llvm-backend` |
| `ADR-0002` | 1907-1912 | `24673434b56e` | copied | `openspec/changes/archive/2026-05-28-adr-0002-block-syntax` |
| `ADR-0003` | 1913-1916 | `91f21c8db59b` | copied | `openspec/changes/archive/2026-05-28-adr-0003-type-system` |
| `ADR-0004` | 1917-1925 | `a7ea289039f3` | copied | `openspec/changes/archive/2026-05-28-adr-0004-exceptions` |
| `ADR-0005` | 1926-1949 | `b3487abfdaa3` | copied | `openspec/changes/archive/2026-05-31-adr-0005-consolidated-0-1-language-decisions` |
| `ADR-0006` | 1950-1997 | `00635fb17fa5` | copied | `openspec/changes/archive/2026-05-31-adr-0006-inox-0-1-language-constitution` |
| `ADR-0017` | 1998-2023 | `99a0ae3ad662` | copied | `openspec/changes/archive/2026-10-11-adr-0017-read-only-slices` |
| `ADR-0016` | 2024-2055 | `ae224ec3acef` | copied | `openspec/changes/archive/2026-10-11-adr-0016-ref-and-ref-mut-parameters` |
| `ADR-0015` | 2056-2073 | `d138712bb9d2` | copied | `openspec/changes/archive/2026-10-10-adr-0015-storing-into-a-natural-checks-the-range` |
| `ADR-0014` | 2074-2101 | `8f83b1f7aa88` | copied | `openspec/changes/archive/2026-10-10-adr-0014-natural-byte-and-the-closed-numeric-set` |
| `ADR-0013` | 2102-2127 | `8b43a6f9c9e7` | copied | `openspec/changes/archive/2026-10-10-adr-0013-conditional-expression` |
| `ADR-0012` | 2128-2141 | `387bf21df2e0` | copied | `openspec/changes/archive/2026-10-10-adr-0012-compound-assignment` |
| `ADR-0011` | 2142-2174 | `8cd31c3b1d14` | copied | `openspec/changes/archive/2026-10-09-adr-0011-case-in-the-ada-spark-style` |
| `ADR-0010` | 2175-2186 | `01ebe198452d` | copied | `openspec/changes/archive/2026-10-09-adr-0010-step-in-the-for-header` |
| `ADR-0009` | 2187-2202 | `9fb0711acf2b` | copied | `openspec/changes/archive/2026-10-09-adr-0009-chained-assignment` |
| `ADR-0008` | 2203-2220 | `6cbc9717bb6c` | copied | `openspec/changes/archive/2026-10-09-adr-0008-precedence-of-and-in` |
| `ADR-0007` | 2221-2233 | `d8e65f343093` | copied | `openspec/changes/archive/2026-10-09-adr-0007-loop-exit-and-try-cleanup-keywords` |
| `GROUP-development` | 2234-2249 | `6f6455b69203` | structural | heading only |
| `SECTION-06` | 2250-2252 | `1f3fa370b3e3` | structural | heading only |
| `canonical-user-facing-cli-contract` | 2253-2268 | `45e0e5d66ff0` | planned | openspec/specs/compiler/cli |
| `current-observed-driver-status` | 2269-2274 | `15c15b4e82d8` | planned | docs/STATUS.md |
| `SECTION-07` | 2275-2277 | `f2dfbd93d1af` | structural | heading only |
| `source-file-structure` | 2278-2289 | `f905deb42055` | planned | openspec/specs/language/modules |
| `SECTION-08` | 2290-2292 | `83d42d4dfe69` | structural | heading only |
| `CANON-2` | 2293-2324 | `1784cd8f9c86` | planned | openspec/specs/language/lexical |
| `SECTION-09` | 2325-2327 | `151b987c27db` | structural | heading only |
| `CANON-3` | 2328-2348 | `eb3765442875` | planned | openspec/specs/language/modules |
| `SECTION-10` | 2349-2351 | `539a6368d8f5` | structural | heading only |
| `CANON-4` | 2352-2407 | `011a17049109` | planned | openspec/specs/language/block-syntax |
| `CANON-7` | 2408-2419 | `954891e7e64b` | planned | openspec/specs/language/block-syntax |
| `SECTION-11` | 2420-2422 | `cc93dd9983b8` | structural | heading only |
| `CANON-5` | 2423-2427 | `107db4bb6efc` | planned | openspec/specs/language/variables |
| `CANON-5/type-section` | 2428-2442 | `ee2b644e002f` | planned | openspec/specs/language/declaration-sections |
| `CANON-5/local-variables-three-valid-forms-only` | 2443-2450 | `7acb630f2600` | planned | openspec/specs/language/variables |
| `CANON-5/grouped-declarations` | 2451-2463 | `211d54982dd6` | planned | openspec/specs/language/variables |
| `CANON-5/compound-assignment` | 2464-2473 | `ac2d6220edcf` | planned | openspec/specs/language/assignment |
| `CANON-5/assignment-is-a-statement-chained-assign` | 2474-2533 | `4a3bbaf10235` | planned | openspec/specs/language/assignment |
| `CANON-5/safe-type-inference` | 2534-2542 | `8cabd03f259a` | planned | openspec/specs/language/variables |
| `CANON-5/const-and-state` | 2543-2552 | `c77d846e37bf` | planned | openspec/specs/language/declaration-sections, openspec/specs/language/module-state |
| `SECTION-12` | 2553-2555 | `21806ebd31bd` | structural | heading only |
| `CANON-8` | 2556-2557 | `44025c9660f2` | structural | heading only |
| `CANON-8/canonical-type-hierarchy` | 2558-2598 | `3d044d5e00c7` | planned | openspec/specs/language/numeric-types, openspec/specs/language/natural, openspec/specs/language/fixed-width-integers |
| `CANON-8/generics-use-angle-brackets` | 2599-2607 | `599f5b0a40cf` | planned | openspec/specs/language/generics |
| `CANON-8/conversions` | 2608-2620 | `233a06f0e0e2` | planned | openspec/specs/language/conversions |
| `CANON-8/decimal-literal-form` | 2621-2640 | `5d29e066b6b5` | planned | openspec/specs/language/decimal-types |
| `CANON-8/operators-and-numeric-law` | 2641-2668 | `4234270d384a` | planned | openspec/specs/language/numeric-types |
| `SECTION-13` | 2669-2679 | `2897fbbce2ae` | planned | openspec/specs/language/variables, openspec/specs/language/mutability |
| `SECTION-13/variable-declaration-rules` | 2680-2683 | `754e4e1781da` | planned | openspec/specs/language/variables |
| `CANON-10` | 2684-2708 | `9e51efaea2ef` | planned | openspec/specs/language/mutability |
| `SECTION-14` | 2709-2711 | `47360f55ede4` | structural | heading only |
| `scoping-and-no-shadowing` | 2712-2724 | `212122b8a58f` | planned | openspec/specs/language/scoping |
| `SECTION-15` | 2725-2727 | `60939e1901e7` | structural | heading only |
| `CANON-6` | 2728-2748 | `deea50879587` | planned | openspec/specs/language/routines |
| `SECTION-16` | 2749-2751 | `70008477d760` | structural | heading only |
| `control-flow-constructs` | 2752-2753 | `49a7482f73bc` | structural | heading only |
| `CANON-12` | 2754-2757 | `778fe3b1480b` | planned | openspec/specs/language/control-flow |
| `CANON-12/if-elif-else` | 2758-2768 | `3cf7facb9c42` | planned | openspec/specs/language/control-flow |
| `CANON-15` | 2769-2918 | `a52eb819fcce` | planned | openspec/specs/language/exceptions |
| `CANON-15/current-v3-17-lowering-note` | 2919-2929 | `0c522c2d339d` | planned | docs/STATUS.md |
| `CANON-11` | 2930-2961 | `ad57cb2fc650` | planned | openspec/specs/language/with-statement |
| `SECTION-17` | 2962-2964 | `f11baea07e11` | structural | heading only |
| `SECTION-17/while` | 2965-2970 | `ba94f83af4d4` | planned | openspec/specs/language/loops |
| `SECTION-17/repeat-until` | 2971-2980 | `45814b3807ec` | planned | openspec/specs/language/loops |
| `SECTION-17/for-in-range` | 2981-3011 | `363715f8a505` | planned | openspec/specs/language/loops |
| `SECTION-17/conditional-expression` | 3012-3020 | `2dceb1930154` | planned | openspec/specs/language/conditional-expression |
| `SECTION-17/case` | 3021-3046 | `6639c7cf4313` | mirrored | `openspec/specs/language/case-statement/spec.md` (Enum part: change enum-range-set) |
| `SECTION-17/unless` | 3047-3051 | `c75b89637df7` | mirrored | `openspec/specs/language/case-statement/spec.md` |
| `SECTION-18` | 3052-3054 | `e522f90c7379` | structural | heading only |
| `CANON-9` | 3055-3094 | `d5eec8fbbc36` | planned | openspec/specs/language/structs, openspec/specs/language/associated-methods |
| `CANON-9/named-struct-construction` | 3095-3131 | `d0090df0b373` | planned | openspec/specs/language/structs |
| `SECTION-19` | 3132-3134 | `e0d3647383a1` | structural | heading only |
| `CANON-14` | 3135-3151 | `e5499ac61e11` | planned | openspec/specs/language/strings |
| `SECTION-20` | 3152-3156 | `3b560adced11` | planned | openspec/specs/language/numeric-types |
| `numeric-semantics-summary` | 3157-3167 | `bda5b68d9240` | planned | openspec/specs/language/numeric-types |
| `CANON-20` | 3168-3175 | `b20c1473c6e9` | planned | openspec/specs/language/operator-precedence |
| `CANON-20/design-intent` | 3176-3184 | `b9c61f6ebef4` | planned | openspec/specs/language/operator-precedence |
| `CANON-20/precedence-table` | 3185-3221 | `606ca4e1d824` | planned | openspec/specs/language/operator-precedence |
| `CANON-20/mandatory-parentheses-rule` | 3222-3242 | `6d49217d2355` | planned | openspec/specs/language/operator-precedence |
| `CANON-20/verifiability` | 3243-3258 | `40b3313dba9d` | planned | openspec/specs/language/operator-precedence |
| `SECTION-21` | 3259-3261 | `ec02c6ed9865` | structural | heading only |
| `CANON-13` | 3262-3263 | `f59f57e57286` | structural | heading only |
| `CANON-13/array` | 3264-3271 | `f2d44c0dd686` | mirrored | `openspec/specs/language/fixed-arrays/spec.md` |
| `CANON-13/vector` | 3272-3278 | `c99a2607fa26` | planned | openspec/specs/language/vectors |
| `CANON-13/range` | 3279-3284 | `c7b61a41d8eb` | planned | openspec/specs/language/ranges |
| `CANON-13/enum` | 3285-3298 | `2d11b4cc7dc3` | planned | openspec/specs/language/enums |
| `CANON-13/set` | 3299-3310 | `240f92fbca14` | planned | openspec/specs/language/sets |
| `SECTION-22` | 3311-3313 | `f6a146590eb2` | structural | heading only |
| `CANON-18` | 3314-3334 | `c798859f64fa` | planned | openspec/specs/stdlib/standard-library |
| `CANON-17` | 3335-3367 | `b58a10142c44` | planned | openspec/specs/stdlib/console-io |
| `SECTION-23` | 3368-3370 | `f29a82eefbab` | structural | heading only |
| `CANON-19` | 3371-3372 | `2682b6b517b1` | structural | heading only |
| `CANON-19/bootstrap-exception-runtime` | 3373-3408 | `3732c1eb5dfc` | planned | openspec/specs/runtime/exception-runtime |
| `CANON-19/checked-integer-arithmetic` | 3409-3483 | `76e58b94adb4` | planned | openspec/specs/runtime/runtime-faults |
| `CANON-19/compiler-implementation-limits` | 3484-3503 | `4564c0b0dffc` | planned | openspec/specs/compiler/limits |
| `temporary-console-i-o-abi` | 3504-3515 | `ee67f38f23b2` | planned | docs/STATUS.md |
| `SECTION-24` | 3516-3518 | `d83447a408b7` | structural | heading only |
| `CANON-20~2` | 3519-3549 | `89bc9490baa9` | planned | openspec/specs/compiler/llvm-backend |
| `current-backend-support-status` | 3550-3551 | `92e4d170aa16` | structural | heading only |
| `B-WORKS` | 3552-3571 | `05b04ae6d0f9` | planned | docs/STATUS.md |
| `B-PARSED` | 3572-3577 | `010d10c5ad64` | planned | docs/STATUS.md |
| `B-GAPS` | 3578-3648 | `376488d68493` | planned | docs/STATUS.md |
| `SECTION-25` | 3649-3651 | `561c278c3296` | structural | heading only |
| `examples-policy` | 3652-3661 | `1ef48ef87dbf` | planned | docs/ENGINEERING.md |
| `B-EXAMPLES` | 3662-3670 | `3d9b7dd4c5e3` | planned | docs/STATUS.md |
| `SECTION-26` | 3671-3673 | `9a88a697ae99` | structural | heading only |
| `CANON-21` | 3674-3707 | `afab1f61829e` | planned | docs/ENGINEERING.md |
| `required-regression-layers` | 3708-3717 | `6098f4656540` | planned | docs/ENGINEERING.md |
| `SECTION-27` | 3718-3720 | `62cc5be85e73` | structural | heading only |
| `release-validation-policy` | 3721-3739 | `c2bd42d65889` | planned | docs/RELEASE.md |
| `DEV-2~2` | 3740-3776 | `6ff9fac762f9` | duplicate | `unit:DEV-2` |
| `GROUP-future~2` | 3777-3785 | `c0c3ec88bb92` | duplicate | `unit:GROUP-future` |
| `SECTION-28` | 3786-3788 | `b97b11b56381` | structural | heading only |
| `documentation-generation-policy` | 3789-3801 | `1d33470c8cd5` | planned | docs/ENGINEERING.md |
| `SECTION-29` | 3802-3804 | `000dd38c7ed9` | structural | heading only |
| `DEV-1` | 3805-3842 | `48712f26b690` | planned | docs/RELEASE.md |
| `portability-targets` | 3843-3889 | `2422ef6e3b0a` | planned | docs/RELEASE.md |
| `SECTION-30` | 3890-3894 | `683f7b9b00c7` | planned | docs/STATUS.md |
| `B-WORKS~2` | 3895-3911 | `65db813fde18` | planned | docs/STATUS.md |
| `B-PARSED~2` | 3912-3917 | `010d10c5ad64` | planned | docs/STATUS.md |
| `B-GAPS~2` | 3918-3986 | `a5ea1b1eb65f` | planned | docs/STATUS.md |
| `B-CONFLICTS` | 3987-3994 | `fdc28764bc4a` | planned | docs/STATUS.md |
| `B-EXAMPLES~2` | 3995-4001 | `f85a9a4e4be5` | planned | docs/STATUS.md |
| `B-PROPOSALS` | 4002-4010 | `b5622600ecc3` | planned | openspec/changes (one change per open item) |
| `SECTION-31` | 4011-4013 | `0c7c92ea754c` | structural | heading only |
| `CANON-22` | 4014-4033 | `0fd628df7854` | planned | docs/ROADMAP.md |
| `GROUP-decisions` | 4034-4041 | `8b90cf0e6ca5` | planned | openspec/README.md (history) |
| `roadmap-to-1-0` | 4042-4056 | `de99ff09125d` | planned | docs/ROADMAP.md |
| `backend-strategy-directive` | 4057-4063 | `3e733152683f` | planned | docs/ENGINEERING.md |
| `backend-strategy-directive/core-principle` | 4064-4077 | `97d3972ccf1e` | planned | docs/ENGINEERING.md |
| `backend-strategy-directive/backend-maturation-sequence` | 4078-4093 | `ff2dbe76d57a` | planned | docs/ENGINEERING.md |
| `backend-strategy-directive/objective-criterion-for-leaving-clang` | 4094-4108 | `1cda38393426` | planned | docs/ENGINEERING.md |
| `backend-strategy-directive/scope-guard-for-backend-adjacent-work` | 4109-4115 | `859bab429005` | planned | docs/ENGINEERING.md |
| `compiler-module-architecture-directive` | 4116-4122 | `7dfc746e78b5` | planned | docs/ENGINEERING.md |
| `compiler-module-architecture-directive/principle-reactive-not-predictive` | 4123-4131 | `6bdbaef0d48c` | planned | docs/ENGINEERING.md |
| `compiler-module-architecture-directive/the-three-tests-for-extracting-a-module` | 4132-4147 | `9bdd4cb899e6` | planned | docs/ENGINEERING.md |
| `compiler-module-architecture-directive/anti-rules` | 4148-4156 | `c8700ead7c93` | planned | docs/ENGINEERING.md |
| `compiler-module-architecture-directive/working-rule-of-thumb` | 4157-4161 | `ec2c782d1c49` | planned | docs/ENGINEERING.md |
| `compiler-module-architecture-directive/timing` | 4162-4168 | `c4c0fef11f84` | planned | docs/ENGINEERING.md |
| `BANNER-end-of-inox-canonical-md-v3-8` | 4169-4174 | `784433b7be07` | planned | openspec/README.md (reading order) |
