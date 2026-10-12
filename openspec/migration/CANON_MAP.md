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
| `BANNER-the-single-canonical-document` | 3-56 | `3b17d4a20e7d` | planned | openspec/README.md (authority, maintainer, license, map) |
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
| `LOG/v3.51` | 504-510 | `7e14e9ffa7a9` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.50` | 511-535 | `0441fbff3bf5` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.49` | 536-562 | `be098b1b06bd` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.48` | 563-584 | `afa50d5ec69f` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.47` | 585-597 | `72a0e9ea7e82` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.46` | 598-611 | `07eee37e2b47` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.45` | 612-640 | `e8bf1bc74e25` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.44` | 641-663 | `1ed18d87a1f6` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.43` | 664-673 | `7cec90575743` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.42` | 674-689 | `e00cff8892ed` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.41` | 690-695 | `d9b78ef08a85` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.40` | 696-714 | `3c2103c842ec` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.39` | 715-744 | `ff8a1f06c34f` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.38` | 745-755 | `d97e13c1b814` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.37` | 756-767 | `73f3f0239390` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.36` | 768-784 | `889932b86d67` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.35` | 785-802 | `a40c544968ff` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.34` | 803-815 | `fa31c1e6a95f` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.33` | 816-838 | `546628eeb9d8` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.32` | 839-860 | `01b8acd407b7` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.31` | 861-874 | `c6f79fbad05a` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.30` | 875-896 | `82fc2948b1b0` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.29` | 897-957 | `411404a57a61` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.28` | 958-993 | `f95a9348aa62` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.27` | 994-1049 | `9a521127b697` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.26` | 1050-1069 | `d412b402bbfb` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.25` | 1070-1082 | `ac5b09d96e50` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.24` | 1083-1105 | `977e3c458a59` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.23` | 1106-1145 | `1bd9020135e7` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.22` | 1146-1160 | `c3cd869369d8` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.21` | 1161-1225 | `fa631814dc07` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.20` | 1226-1254 | `c178428e9e51` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.19` | 1255-1270 | `58a9449923cc` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.18` | 1271-1349 | `63b0850a3b94` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.17` | 1350-1370 | `6b82cfa2bfef` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.16` | 1371-1391 | `504ebe528c6f` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.15a` | 1392-1405 | `8a99974e3b15` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.15` | 1406-1420 | `db45cccf8f29` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.14` | 1421-1431 | `45d2e2e7b0f4` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.13` | 1432-1443 | `e78d99b40dce` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.12` | 1444-1453 | `adf42d99bf9c` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.11` | 1454-1463 | `bb084ddde131` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.10` | 1464-1476 | `bb0c5292dc58` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.8` | 1477-1487 | `e2021267fde4` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.7` | 1488-1498 | `4c50f899f80e` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.6` | 1499-1515 | `f5c6a45807b0` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.5` | 1516-1520 | `1e634d009b80` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.4` | 1521-1526 | `a02b7bf01817` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.3` | 1527-1533 | `b73b838ee71a` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.2` | 1534-1540 | `3aae34cdb8de` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.1` | 1541-1553 | `1c1bfe44fee1` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.0` | 1554-1563 | `01fee319f99c` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v2.2` | 1564-1566 | `947024e547a5` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v2.1` | 1567-1569 | `831353b88b26` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v2.0` | 1570-1575 | `92161592c6f6` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v1.0` | 1576-1577 | `bf9fe6205559` | structural | heading only |
| `DEV-2` | 1578-1614 | `6ff9fac762f9` | planned | docs/RELEASE.md |
| `GROUP-future` | 1615-1623 | `c0c3ec88bb92` | planned | docs/ROADMAP.md |
| `SECTION-03` | 1624-1626 | `8f4b634fd646` | structural | heading only |
| `agent-rules` | 1627-1660 | `f699b67648da` | planned | AGENTS.md (governance) |
| `SECTION-04` | 1661-1665 | `03786d4742d7` | planned | docs/ROADMAP.md |
| `B-OPEN` | 1666-1710 | `4e5f8d3984d1` | planned | openspec/changes (one change per open item) |
| `FUTURE-1` | 1711-1718 | `0a1e1448d12b` | planned | docs/ROADMAP.md |
| `FUTURE-2` | 1719-1723 | `eb05f449bfc6` | planned | docs/ROADMAP.md |
| `FUTURE-3` | 1724-1729 | `2c09d65cfded` | planned | docs/ROADMAP.md |
| `FUTURE-4` | 1730-1733 | `62d8fde09ca2` | planned | docs/ROADMAP.md |
| `FUTURE-5` | 1734-1738 | `041157e3cd11` | planned | docs/ROADMAP.md |
| `FUTURE-6` | 1739-1742 | `a070988b84e8` | planned | docs/ROADMAP.md |
| `FUTURE-7` | 1743-1755 | `116a4980e694` | planned | docs/ROADMAP.md |
| `GROUP-open-questions` | 1756-1779 | `9806de6ee55e` | planned | docs/ROADMAP.md, docs/STATUS.md |
| `BANNER-agent-rules` | 1780-1813 | `1dd939e2384f` | duplicate | `unit:agent-rules` |
| `BANNER-layer-b-implementation-status` | 1814-1834 | `cb181e7e1383` | planned | docs/STATUS.md |
| `SECTION-05` | 1835-1839 | `3950191b4348` | planned | openspec/README.md (history) |
| `ADR-0001` | 1840-1844 | `6597d424187c` | copied | `openspec/changes/archive/2026-05-28-adr-0001-llvm-backend` |
| `ADR-0002` | 1845-1850 | `24673434b56e` | copied | `openspec/changes/archive/2026-05-28-adr-0002-block-syntax` |
| `ADR-0003` | 1851-1854 | `91f21c8db59b` | copied | `openspec/changes/archive/2026-05-28-adr-0003-type-system` |
| `ADR-0004` | 1855-1863 | `a7ea289039f3` | copied | `openspec/changes/archive/2026-05-28-adr-0004-exceptions` |
| `ADR-0005` | 1864-1887 | `b3487abfdaa3` | copied | `openspec/changes/archive/2026-05-31-adr-0005-consolidated-0-1-language-decisions` |
| `ADR-0006` | 1888-1935 | `00635fb17fa5` | copied | `openspec/changes/archive/2026-05-31-adr-0006-inox-0-1-language-constitution` |
| `ADR-0017` | 1936-1961 | `99a0ae3ad662` | copied | `openspec/changes/archive/2026-10-11-adr-0017-read-only-slices` |
| `ADR-0016` | 1962-1993 | `ae224ec3acef` | copied | `openspec/changes/archive/2026-10-11-adr-0016-ref-and-ref-mut-parameters` |
| `ADR-0015` | 1994-2011 | `d138712bb9d2` | copied | `openspec/changes/archive/2026-10-10-adr-0015-storing-into-a-natural-checks-the-range` |
| `ADR-0014` | 2012-2039 | `8f83b1f7aa88` | copied | `openspec/changes/archive/2026-10-10-adr-0014-natural-byte-and-the-closed-numeric-set` |
| `ADR-0013` | 2040-2065 | `8b43a6f9c9e7` | copied | `openspec/changes/archive/2026-10-10-adr-0013-conditional-expression` |
| `ADR-0012` | 2066-2079 | `387bf21df2e0` | copied | `openspec/changes/archive/2026-10-10-adr-0012-compound-assignment` |
| `ADR-0011` | 2080-2112 | `8cd31c3b1d14` | copied | `openspec/changes/archive/2026-10-09-adr-0011-case-in-the-ada-spark-style` |
| `ADR-0010` | 2113-2124 | `01ebe198452d` | copied | `openspec/changes/archive/2026-10-09-adr-0010-step-in-the-for-header` |
| `ADR-0009` | 2125-2140 | `9fb0711acf2b` | copied | `openspec/changes/archive/2026-10-09-adr-0009-chained-assignment` |
| `ADR-0008` | 2141-2158 | `6cbc9717bb6c` | copied | `openspec/changes/archive/2026-10-09-adr-0008-precedence-of-and-in` |
| `ADR-0007` | 2159-2171 | `d8e65f343093` | copied | `openspec/changes/archive/2026-10-09-adr-0007-loop-exit-and-try-cleanup-keywords` |
| `GROUP-development` | 2172-2187 | `6f6455b69203` | structural | heading only |
| `SECTION-06` | 2188-2190 | `1f3fa370b3e3` | structural | heading only |
| `canonical-user-facing-cli-contract` | 2191-2206 | `45e0e5d66ff0` | planned | openspec/specs/compiler/cli |
| `current-observed-driver-status` | 2207-2212 | `15c15b4e82d8` | planned | docs/STATUS.md |
| `SECTION-07` | 2213-2215 | `f2dfbd93d1af` | structural | heading only |
| `source-file-structure` | 2216-2227 | `f905deb42055` | planned | openspec/specs/language/modules |
| `SECTION-08` | 2228-2230 | `83d42d4dfe69` | structural | heading only |
| `CANON-2` | 2231-2262 | `1784cd8f9c86` | planned | openspec/specs/language/lexical |
| `SECTION-09` | 2263-2265 | `151b987c27db` | structural | heading only |
| `CANON-3` | 2266-2286 | `eb3765442875` | planned | openspec/specs/language/modules |
| `SECTION-10` | 2287-2289 | `539a6368d8f5` | structural | heading only |
| `CANON-4` | 2290-2345 | `011a17049109` | planned | openspec/specs/language/block-syntax |
| `CANON-7` | 2346-2357 | `954891e7e64b` | planned | openspec/specs/language/block-syntax |
| `SECTION-11` | 2358-2360 | `cc93dd9983b8` | structural | heading only |
| `CANON-5` | 2361-2365 | `107db4bb6efc` | planned | openspec/specs/language/variables |
| `CANON-5/type-section` | 2366-2380 | `ee2b644e002f` | planned | openspec/specs/language/declaration-sections |
| `CANON-5/local-variables-three-valid-forms-only` | 2381-2388 | `7acb630f2600` | planned | openspec/specs/language/variables |
| `CANON-5/grouped-declarations` | 2389-2401 | `211d54982dd6` | planned | openspec/specs/language/variables |
| `CANON-5/compound-assignment` | 2402-2411 | `ac2d6220edcf` | planned | openspec/specs/language/assignment |
| `CANON-5/assignment-is-a-statement-chained-assign` | 2412-2471 | `4a3bbaf10235` | planned | openspec/specs/language/assignment |
| `CANON-5/safe-type-inference` | 2472-2480 | `8cabd03f259a` | planned | openspec/specs/language/variables |
| `CANON-5/const-and-state` | 2481-2490 | `c77d846e37bf` | planned | openspec/specs/language/declaration-sections, openspec/specs/language/module-state |
| `SECTION-12` | 2491-2493 | `21806ebd31bd` | structural | heading only |
| `CANON-8` | 2494-2495 | `44025c9660f2` | structural | heading only |
| `CANON-8/canonical-type-hierarchy` | 2496-2536 | `3d044d5e00c7` | planned | openspec/specs/language/numeric-types, openspec/specs/language/natural, openspec/specs/language/fixed-width-integers |
| `CANON-8/generics-use-angle-brackets` | 2537-2545 | `599f5b0a40cf` | planned | openspec/specs/language/generics |
| `CANON-8/conversions` | 2546-2558 | `233a06f0e0e2` | planned | openspec/specs/language/conversions |
| `CANON-8/decimal-literal-form` | 2559-2578 | `5d29e066b6b5` | planned | openspec/specs/language/decimal-types |
| `CANON-8/operators-and-numeric-law` | 2579-2606 | `4234270d384a` | planned | openspec/specs/language/numeric-types |
| `SECTION-13` | 2607-2617 | `2897fbbce2ae` | planned | openspec/specs/language/variables, openspec/specs/language/mutability |
| `SECTION-13/variable-declaration-rules` | 2618-2621 | `754e4e1781da` | planned | openspec/specs/language/variables |
| `CANON-10` | 2622-2646 | `9e51efaea2ef` | planned | openspec/specs/language/mutability |
| `SECTION-14` | 2647-2649 | `47360f55ede4` | structural | heading only |
| `scoping-and-no-shadowing` | 2650-2662 | `212122b8a58f` | planned | openspec/specs/language/scoping |
| `SECTION-15` | 2663-2665 | `60939e1901e7` | structural | heading only |
| `CANON-6` | 2666-2686 | `deea50879587` | planned | openspec/specs/language/routines |
| `SECTION-16` | 2687-2689 | `70008477d760` | structural | heading only |
| `control-flow-constructs` | 2690-2691 | `49a7482f73bc` | structural | heading only |
| `CANON-12` | 2692-2695 | `778fe3b1480b` | planned | openspec/specs/language/control-flow |
| `CANON-12/if-elif-else` | 2696-2706 | `3cf7facb9c42` | planned | openspec/specs/language/control-flow |
| `CANON-15` | 2707-2856 | `a52eb819fcce` | planned | openspec/specs/language/exceptions |
| `CANON-15/current-v3-17-lowering-note` | 2857-2867 | `0c522c2d339d` | planned | docs/STATUS.md |
| `CANON-11` | 2868-2899 | `ad57cb2fc650` | planned | openspec/specs/language/with-statement |
| `SECTION-17` | 2900-2902 | `f11baea07e11` | structural | heading only |
| `SECTION-17/while` | 2903-2908 | `ba94f83af4d4` | planned | openspec/specs/language/loops |
| `SECTION-17/repeat-until` | 2909-2918 | `45814b3807ec` | planned | openspec/specs/language/loops |
| `SECTION-17/for-in-range` | 2919-2949 | `363715f8a505` | planned | openspec/specs/language/loops |
| `SECTION-17/conditional-expression` | 2950-2958 | `2dceb1930154` | planned | openspec/specs/language/conditional-expression |
| `SECTION-17/case` | 2959-2984 | `6639c7cf4313` | mirrored | `openspec/specs/language/case-statement/spec.md` (Enum part: change enum-range-set) |
| `SECTION-17/unless` | 2985-2989 | `c75b89637df7` | mirrored | `openspec/specs/language/case-statement/spec.md` |
| `SECTION-18` | 2990-2992 | `e522f90c7379` | structural | heading only |
| `CANON-9` | 2993-3032 | `d5eec8fbbc36` | planned | openspec/specs/language/structs, openspec/specs/language/associated-methods |
| `CANON-9/named-struct-construction` | 3033-3069 | `d0090df0b373` | planned | openspec/specs/language/structs |
| `SECTION-19` | 3070-3072 | `e0d3647383a1` | structural | heading only |
| `CANON-14` | 3073-3089 | `e5499ac61e11` | planned | openspec/specs/language/strings |
| `SECTION-20` | 3090-3094 | `3b560adced11` | planned | openspec/specs/language/numeric-types |
| `numeric-semantics-summary` | 3095-3105 | `bda5b68d9240` | planned | openspec/specs/language/numeric-types |
| `CANON-20` | 3106-3113 | `b20c1473c6e9` | planned | openspec/specs/language/operator-precedence |
| `CANON-20/design-intent` | 3114-3122 | `b9c61f6ebef4` | planned | openspec/specs/language/operator-precedence |
| `CANON-20/precedence-table` | 3123-3159 | `606ca4e1d824` | planned | openspec/specs/language/operator-precedence |
| `CANON-20/mandatory-parentheses-rule` | 3160-3180 | `6d49217d2355` | planned | openspec/specs/language/operator-precedence |
| `CANON-20/verifiability` | 3181-3196 | `40b3313dba9d` | planned | openspec/specs/language/operator-precedence |
| `SECTION-21` | 3197-3199 | `ec02c6ed9865` | structural | heading only |
| `CANON-13` | 3200-3201 | `f59f57e57286` | structural | heading only |
| `CANON-13/array` | 3202-3209 | `f2d44c0dd686` | mirrored | `openspec/specs/language/fixed-arrays/spec.md` |
| `CANON-13/vector` | 3210-3216 | `c99a2607fa26` | planned | openspec/specs/language/vectors |
| `CANON-13/range` | 3217-3222 | `c7b61a41d8eb` | planned | openspec/specs/language/ranges |
| `CANON-13/enum` | 3223-3236 | `2d11b4cc7dc3` | planned | openspec/specs/language/enums |
| `CANON-13/set` | 3237-3248 | `240f92fbca14` | planned | openspec/specs/language/sets |
| `SECTION-22` | 3249-3251 | `f6a146590eb2` | structural | heading only |
| `CANON-18` | 3252-3272 | `c798859f64fa` | planned | openspec/specs/stdlib/standard-library |
| `CANON-17` | 3273-3305 | `b58a10142c44` | planned | openspec/specs/stdlib/console-io |
| `SECTION-23` | 3306-3308 | `f29a82eefbab` | structural | heading only |
| `CANON-19` | 3309-3310 | `2682b6b517b1` | structural | heading only |
| `CANON-19/bootstrap-exception-runtime` | 3311-3346 | `3732c1eb5dfc` | planned | openspec/specs/runtime/exception-runtime |
| `CANON-19/checked-integer-arithmetic` | 3347-3421 | `76e58b94adb4` | planned | openspec/specs/runtime/runtime-faults |
| `CANON-19/compiler-implementation-limits` | 3422-3441 | `4564c0b0dffc` | planned | openspec/specs/compiler/limits |
| `temporary-console-i-o-abi` | 3442-3453 | `ee67f38f23b2` | planned | docs/STATUS.md |
| `SECTION-24` | 3454-3456 | `d83447a408b7` | structural | heading only |
| `CANON-20~2` | 3457-3487 | `89bc9490baa9` | planned | openspec/specs/compiler/llvm-backend |
| `current-backend-support-status` | 3488-3489 | `92e4d170aa16` | structural | heading only |
| `B-WORKS` | 3490-3509 | `05b04ae6d0f9` | planned | docs/STATUS.md |
| `B-PARSED` | 3510-3515 | `010d10c5ad64` | planned | docs/STATUS.md |
| `B-GAPS` | 3516-3585 | `f0ab0c6bcde9` | planned | docs/STATUS.md |
| `SECTION-25` | 3586-3588 | `561c278c3296` | structural | heading only |
| `examples-policy` | 3589-3598 | `1ef48ef87dbf` | planned | docs/ENGINEERING.md |
| `B-EXAMPLES` | 3599-3607 | `3d9b7dd4c5e3` | planned | docs/STATUS.md |
| `SECTION-26` | 3608-3610 | `9a88a697ae99` | structural | heading only |
| `CANON-21` | 3611-3644 | `afab1f61829e` | planned | docs/ENGINEERING.md |
| `required-regression-layers` | 3645-3654 | `6098f4656540` | planned | docs/ENGINEERING.md |
| `SECTION-27` | 3655-3657 | `62cc5be85e73` | structural | heading only |
| `release-validation-policy` | 3658-3676 | `c2bd42d65889` | planned | docs/RELEASE.md |
| `DEV-2~2` | 3677-3713 | `6ff9fac762f9` | duplicate | `unit:DEV-2` |
| `GROUP-future~2` | 3714-3722 | `c0c3ec88bb92` | duplicate | `unit:GROUP-future` |
| `SECTION-28` | 3723-3725 | `b97b11b56381` | structural | heading only |
| `documentation-generation-policy` | 3726-3738 | `1d33470c8cd5` | planned | docs/ENGINEERING.md |
| `SECTION-29` | 3739-3741 | `000dd38c7ed9` | structural | heading only |
| `DEV-1` | 3742-3779 | `48712f26b690` | planned | docs/RELEASE.md |
| `portability-targets` | 3780-3826 | `2422ef6e3b0a` | planned | docs/RELEASE.md |
| `SECTION-30` | 3827-3831 | `683f7b9b00c7` | planned | docs/STATUS.md |
| `B-WORKS~2` | 3832-3848 | `65db813fde18` | planned | docs/STATUS.md |
| `B-PARSED~2` | 3849-3854 | `010d10c5ad64` | planned | docs/STATUS.md |
| `B-GAPS~2` | 3855-3922 | `c43ed6cd6801` | planned | docs/STATUS.md |
| `B-CONFLICTS` | 3923-3930 | `fdc28764bc4a` | planned | docs/STATUS.md |
| `B-EXAMPLES~2` | 3931-3937 | `f85a9a4e4be5` | planned | docs/STATUS.md |
| `B-PROPOSALS` | 3938-3946 | `b5622600ecc3` | planned | openspec/changes (one change per open item) |
| `SECTION-31` | 3947-3949 | `0c7c92ea754c` | structural | heading only |
| `CANON-22` | 3950-3969 | `0fd628df7854` | planned | docs/ROADMAP.md |
| `GROUP-decisions` | 3970-3977 | `8b90cf0e6ca5` | planned | openspec/README.md (history) |
| `roadmap-to-1-0` | 3978-3992 | `de99ff09125d` | planned | docs/ROADMAP.md |
| `backend-strategy-directive` | 3993-3999 | `3e733152683f` | planned | docs/ENGINEERING.md |
| `backend-strategy-directive/core-principle` | 4000-4013 | `97d3972ccf1e` | planned | docs/ENGINEERING.md |
| `backend-strategy-directive/backend-maturation-sequence` | 4014-4029 | `ff2dbe76d57a` | planned | docs/ENGINEERING.md |
| `backend-strategy-directive/objective-criterion-for-leaving-clang` | 4030-4044 | `1cda38393426` | planned | docs/ENGINEERING.md |
| `backend-strategy-directive/scope-guard-for-backend-adjacent-work` | 4045-4051 | `859bab429005` | planned | docs/ENGINEERING.md |
| `compiler-module-architecture-directive` | 4052-4058 | `7dfc746e78b5` | planned | docs/ENGINEERING.md |
| `compiler-module-architecture-directive/principle-reactive-not-predictive` | 4059-4067 | `6bdbaef0d48c` | planned | docs/ENGINEERING.md |
| `compiler-module-architecture-directive/the-three-tests-for-extracting-a-module` | 4068-4083 | `9bdd4cb899e6` | planned | docs/ENGINEERING.md |
| `compiler-module-architecture-directive/anti-rules` | 4084-4092 | `c8700ead7c93` | planned | docs/ENGINEERING.md |
| `compiler-module-architecture-directive/working-rule-of-thumb` | 4093-4097 | `ec2c782d1c49` | planned | docs/ENGINEERING.md |
| `compiler-module-architecture-directive/timing` | 4098-4104 | `c4c0fef11f84` | planned | docs/ENGINEERING.md |
| `BANNER-end-of-inox-canonical-md-v3-8` | 4105-4110 | `784433b7be07` | planned | openspec/README.md (reading order) |
