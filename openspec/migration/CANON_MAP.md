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
| `BANNER-the-single-canonical-document` | 3-56 | `ba7d4aec5b8d` | planned | openspec/README.md (authority, maintainer, license, map) |
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
| `LOG/v3.52` | 504-534 | `a7dbe03eaf28` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.51` | 535-541 | `7e14e9ffa7a9` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.50` | 542-566 | `0441fbff3bf5` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.49` | 567-593 | `be098b1b06bd` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.48` | 594-615 | `afa50d5ec69f` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.47` | 616-628 | `72a0e9ea7e82` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.46` | 629-642 | `07eee37e2b47` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.45` | 643-671 | `e8bf1bc74e25` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.44` | 672-694 | `1ed18d87a1f6` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.43` | 695-704 | `7cec90575743` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.42` | 705-720 | `e00cff8892ed` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.41` | 721-726 | `d9b78ef08a85` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.40` | 727-745 | `3c2103c842ec` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.39` | 746-775 | `ff8a1f06c34f` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.38` | 776-786 | `d97e13c1b814` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.37` | 787-798 | `73f3f0239390` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.36` | 799-815 | `889932b86d67` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.35` | 816-833 | `a40c544968ff` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.34` | 834-846 | `fa31c1e6a95f` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.33` | 847-869 | `546628eeb9d8` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.32` | 870-891 | `01b8acd407b7` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.31` | 892-905 | `c6f79fbad05a` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.30` | 906-927 | `82fc2948b1b0` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.29` | 928-988 | `411404a57a61` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.28` | 989-1024 | `f95a9348aa62` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.27` | 1025-1080 | `9a521127b697` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.26` | 1081-1100 | `d412b402bbfb` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.25` | 1101-1113 | `ac5b09d96e50` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.24` | 1114-1136 | `977e3c458a59` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.23` | 1137-1176 | `1bd9020135e7` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.22` | 1177-1191 | `c3cd869369d8` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.21` | 1192-1256 | `fa631814dc07` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.20` | 1257-1285 | `c178428e9e51` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.19` | 1286-1301 | `58a9449923cc` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.18` | 1302-1380 | `63b0850a3b94` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.17` | 1381-1401 | `6b82cfa2bfef` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.16` | 1402-1422 | `504ebe528c6f` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.15a` | 1423-1436 | `8a99974e3b15` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.15` | 1437-1451 | `db45cccf8f29` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.14` | 1452-1462 | `45d2e2e7b0f4` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.13` | 1463-1474 | `e78d99b40dce` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.12` | 1475-1484 | `adf42d99bf9c` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.11` | 1485-1494 | `bb084ddde131` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.10` | 1495-1507 | `bb0c5292dc58` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.8` | 1508-1518 | `e2021267fde4` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.7` | 1519-1529 | `4c50f899f80e` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.6` | 1530-1546 | `f5c6a45807b0` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.5` | 1547-1551 | `1e634d009b80` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.4` | 1552-1557 | `a02b7bf01817` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.3` | 1558-1564 | `b73b838ee71a` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.2` | 1565-1571 | `3aae34cdb8de` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.1` | 1572-1584 | `1c1bfe44fee1` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.0` | 1585-1594 | `01fee319f99c` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v2.2` | 1595-1597 | `947024e547a5` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v2.1` | 1598-1600 | `831353b88b26` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v2.0` | 1601-1606 | `92161592c6f6` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v1.0` | 1607-1608 | `bf9fe6205559` | structural | heading only |
| `DEV-2` | 1609-1645 | `6ff9fac762f9` | planned | docs/RELEASE.md |
| `GROUP-future` | 1646-1654 | `c0c3ec88bb92` | planned | docs/ROADMAP.md |
| `SECTION-03` | 1655-1657 | `8f4b634fd646` | structural | heading only |
| `agent-rules` | 1658-1691 | `f699b67648da` | planned | AGENTS.md (governance) |
| `SECTION-04` | 1692-1696 | `03786d4742d7` | planned | docs/ROADMAP.md |
| `B-OPEN` | 1697-1741 | `4e5f8d3984d1` | planned | openspec/changes (one change per open item) |
| `FUTURE-1` | 1742-1749 | `0a1e1448d12b` | planned | docs/ROADMAP.md |
| `FUTURE-2` | 1750-1754 | `eb05f449bfc6` | planned | docs/ROADMAP.md |
| `FUTURE-3` | 1755-1760 | `2c09d65cfded` | planned | docs/ROADMAP.md |
| `FUTURE-4` | 1761-1764 | `62d8fde09ca2` | planned | docs/ROADMAP.md |
| `FUTURE-5` | 1765-1769 | `041157e3cd11` | planned | docs/ROADMAP.md |
| `FUTURE-6` | 1770-1773 | `a070988b84e8` | planned | docs/ROADMAP.md |
| `FUTURE-7` | 1774-1786 | `116a4980e694` | planned | docs/ROADMAP.md |
| `GROUP-open-questions` | 1787-1810 | `9806de6ee55e` | planned | docs/ROADMAP.md, docs/STATUS.md |
| `BANNER-agent-rules` | 1811-1844 | `1dd939e2384f` | duplicate | `unit:agent-rules` |
| `BANNER-layer-b-implementation-status` | 1845-1865 | `cb181e7e1383` | planned | docs/STATUS.md |
| `SECTION-05` | 1866-1870 | `3950191b4348` | planned | openspec/README.md (history) |
| `ADR-0001` | 1871-1875 | `6597d424187c` | copied | `openspec/changes/archive/2026-05-28-adr-0001-llvm-backend` |
| `ADR-0002` | 1876-1881 | `24673434b56e` | copied | `openspec/changes/archive/2026-05-28-adr-0002-block-syntax` |
| `ADR-0003` | 1882-1885 | `91f21c8db59b` | copied | `openspec/changes/archive/2026-05-28-adr-0003-type-system` |
| `ADR-0004` | 1886-1894 | `a7ea289039f3` | copied | `openspec/changes/archive/2026-05-28-adr-0004-exceptions` |
| `ADR-0005` | 1895-1918 | `b3487abfdaa3` | copied | `openspec/changes/archive/2026-05-31-adr-0005-consolidated-0-1-language-decisions` |
| `ADR-0006` | 1919-1966 | `00635fb17fa5` | copied | `openspec/changes/archive/2026-05-31-adr-0006-inox-0-1-language-constitution` |
| `ADR-0017` | 1967-1992 | `99a0ae3ad662` | copied | `openspec/changes/archive/2026-10-11-adr-0017-read-only-slices` |
| `ADR-0016` | 1993-2024 | `ae224ec3acef` | copied | `openspec/changes/archive/2026-10-11-adr-0016-ref-and-ref-mut-parameters` |
| `ADR-0015` | 2025-2042 | `d138712bb9d2` | copied | `openspec/changes/archive/2026-10-10-adr-0015-storing-into-a-natural-checks-the-range` |
| `ADR-0014` | 2043-2070 | `8f83b1f7aa88` | copied | `openspec/changes/archive/2026-10-10-adr-0014-natural-byte-and-the-closed-numeric-set` |
| `ADR-0013` | 2071-2096 | `8b43a6f9c9e7` | copied | `openspec/changes/archive/2026-10-10-adr-0013-conditional-expression` |
| `ADR-0012` | 2097-2110 | `387bf21df2e0` | copied | `openspec/changes/archive/2026-10-10-adr-0012-compound-assignment` |
| `ADR-0011` | 2111-2143 | `8cd31c3b1d14` | copied | `openspec/changes/archive/2026-10-09-adr-0011-case-in-the-ada-spark-style` |
| `ADR-0010` | 2144-2155 | `01ebe198452d` | copied | `openspec/changes/archive/2026-10-09-adr-0010-step-in-the-for-header` |
| `ADR-0009` | 2156-2171 | `9fb0711acf2b` | copied | `openspec/changes/archive/2026-10-09-adr-0009-chained-assignment` |
| `ADR-0008` | 2172-2189 | `6cbc9717bb6c` | copied | `openspec/changes/archive/2026-10-09-adr-0008-precedence-of-and-in` |
| `ADR-0007` | 2190-2202 | `d8e65f343093` | copied | `openspec/changes/archive/2026-10-09-adr-0007-loop-exit-and-try-cleanup-keywords` |
| `GROUP-development` | 2203-2218 | `6f6455b69203` | structural | heading only |
| `SECTION-06` | 2219-2221 | `1f3fa370b3e3` | structural | heading only |
| `canonical-user-facing-cli-contract` | 2222-2237 | `45e0e5d66ff0` | planned | openspec/specs/compiler/cli |
| `current-observed-driver-status` | 2238-2243 | `15c15b4e82d8` | planned | docs/STATUS.md |
| `SECTION-07` | 2244-2246 | `f2dfbd93d1af` | structural | heading only |
| `source-file-structure` | 2247-2258 | `f905deb42055` | planned | openspec/specs/language/modules |
| `SECTION-08` | 2259-2261 | `83d42d4dfe69` | structural | heading only |
| `CANON-2` | 2262-2293 | `1784cd8f9c86` | planned | openspec/specs/language/lexical |
| `SECTION-09` | 2294-2296 | `151b987c27db` | structural | heading only |
| `CANON-3` | 2297-2317 | `eb3765442875` | planned | openspec/specs/language/modules |
| `SECTION-10` | 2318-2320 | `539a6368d8f5` | structural | heading only |
| `CANON-4` | 2321-2376 | `011a17049109` | planned | openspec/specs/language/block-syntax |
| `CANON-7` | 2377-2388 | `954891e7e64b` | planned | openspec/specs/language/block-syntax |
| `SECTION-11` | 2389-2391 | `cc93dd9983b8` | structural | heading only |
| `CANON-5` | 2392-2396 | `107db4bb6efc` | planned | openspec/specs/language/variables |
| `CANON-5/type-section` | 2397-2411 | `ee2b644e002f` | planned | openspec/specs/language/declaration-sections |
| `CANON-5/local-variables-three-valid-forms-only` | 2412-2419 | `7acb630f2600` | planned | openspec/specs/language/variables |
| `CANON-5/grouped-declarations` | 2420-2432 | `211d54982dd6` | planned | openspec/specs/language/variables |
| `CANON-5/compound-assignment` | 2433-2442 | `ac2d6220edcf` | planned | openspec/specs/language/assignment |
| `CANON-5/assignment-is-a-statement-chained-assign` | 2443-2502 | `4a3bbaf10235` | planned | openspec/specs/language/assignment |
| `CANON-5/safe-type-inference` | 2503-2511 | `8cabd03f259a` | planned | openspec/specs/language/variables |
| `CANON-5/const-and-state` | 2512-2521 | `c77d846e37bf` | planned | openspec/specs/language/declaration-sections, openspec/specs/language/module-state |
| `SECTION-12` | 2522-2524 | `21806ebd31bd` | structural | heading only |
| `CANON-8` | 2525-2526 | `44025c9660f2` | structural | heading only |
| `CANON-8/canonical-type-hierarchy` | 2527-2567 | `3d044d5e00c7` | planned | openspec/specs/language/numeric-types, openspec/specs/language/natural, openspec/specs/language/fixed-width-integers |
| `CANON-8/generics-use-angle-brackets` | 2568-2576 | `599f5b0a40cf` | planned | openspec/specs/language/generics |
| `CANON-8/conversions` | 2577-2589 | `233a06f0e0e2` | planned | openspec/specs/language/conversions |
| `CANON-8/decimal-literal-form` | 2590-2609 | `5d29e066b6b5` | planned | openspec/specs/language/decimal-types |
| `CANON-8/operators-and-numeric-law` | 2610-2637 | `4234270d384a` | planned | openspec/specs/language/numeric-types |
| `SECTION-13` | 2638-2648 | `2897fbbce2ae` | planned | openspec/specs/language/variables, openspec/specs/language/mutability |
| `SECTION-13/variable-declaration-rules` | 2649-2652 | `754e4e1781da` | planned | openspec/specs/language/variables |
| `CANON-10` | 2653-2677 | `9e51efaea2ef` | planned | openspec/specs/language/mutability |
| `SECTION-14` | 2678-2680 | `47360f55ede4` | structural | heading only |
| `scoping-and-no-shadowing` | 2681-2693 | `212122b8a58f` | planned | openspec/specs/language/scoping |
| `SECTION-15` | 2694-2696 | `60939e1901e7` | structural | heading only |
| `CANON-6` | 2697-2717 | `deea50879587` | planned | openspec/specs/language/routines |
| `SECTION-16` | 2718-2720 | `70008477d760` | structural | heading only |
| `control-flow-constructs` | 2721-2722 | `49a7482f73bc` | structural | heading only |
| `CANON-12` | 2723-2726 | `778fe3b1480b` | planned | openspec/specs/language/control-flow |
| `CANON-12/if-elif-else` | 2727-2737 | `3cf7facb9c42` | planned | openspec/specs/language/control-flow |
| `CANON-15` | 2738-2887 | `a52eb819fcce` | planned | openspec/specs/language/exceptions |
| `CANON-15/current-v3-17-lowering-note` | 2888-2898 | `0c522c2d339d` | planned | docs/STATUS.md |
| `CANON-11` | 2899-2930 | `ad57cb2fc650` | planned | openspec/specs/language/with-statement |
| `SECTION-17` | 2931-2933 | `f11baea07e11` | structural | heading only |
| `SECTION-17/while` | 2934-2939 | `ba94f83af4d4` | planned | openspec/specs/language/loops |
| `SECTION-17/repeat-until` | 2940-2949 | `45814b3807ec` | planned | openspec/specs/language/loops |
| `SECTION-17/for-in-range` | 2950-2980 | `363715f8a505` | planned | openspec/specs/language/loops |
| `SECTION-17/conditional-expression` | 2981-2989 | `2dceb1930154` | planned | openspec/specs/language/conditional-expression |
| `SECTION-17/case` | 2990-3015 | `6639c7cf4313` | mirrored | `openspec/specs/language/case-statement/spec.md` (Enum part: change enum-range-set) |
| `SECTION-17/unless` | 3016-3020 | `c75b89637df7` | mirrored | `openspec/specs/language/case-statement/spec.md` |
| `SECTION-18` | 3021-3023 | `e522f90c7379` | structural | heading only |
| `CANON-9` | 3024-3063 | `d5eec8fbbc36` | planned | openspec/specs/language/structs, openspec/specs/language/associated-methods |
| `CANON-9/named-struct-construction` | 3064-3100 | `d0090df0b373` | planned | openspec/specs/language/structs |
| `SECTION-19` | 3101-3103 | `e0d3647383a1` | structural | heading only |
| `CANON-14` | 3104-3120 | `e5499ac61e11` | planned | openspec/specs/language/strings |
| `SECTION-20` | 3121-3125 | `3b560adced11` | planned | openspec/specs/language/numeric-types |
| `numeric-semantics-summary` | 3126-3136 | `bda5b68d9240` | planned | openspec/specs/language/numeric-types |
| `CANON-20` | 3137-3144 | `b20c1473c6e9` | planned | openspec/specs/language/operator-precedence |
| `CANON-20/design-intent` | 3145-3153 | `b9c61f6ebef4` | planned | openspec/specs/language/operator-precedence |
| `CANON-20/precedence-table` | 3154-3190 | `606ca4e1d824` | planned | openspec/specs/language/operator-precedence |
| `CANON-20/mandatory-parentheses-rule` | 3191-3211 | `6d49217d2355` | planned | openspec/specs/language/operator-precedence |
| `CANON-20/verifiability` | 3212-3227 | `40b3313dba9d` | planned | openspec/specs/language/operator-precedence |
| `SECTION-21` | 3228-3230 | `ec02c6ed9865` | structural | heading only |
| `CANON-13` | 3231-3232 | `f59f57e57286` | structural | heading only |
| `CANON-13/array` | 3233-3240 | `f2d44c0dd686` | mirrored | `openspec/specs/language/fixed-arrays/spec.md` |
| `CANON-13/vector` | 3241-3247 | `c99a2607fa26` | planned | openspec/specs/language/vectors |
| `CANON-13/range` | 3248-3253 | `c7b61a41d8eb` | planned | openspec/specs/language/ranges |
| `CANON-13/enum` | 3254-3267 | `2d11b4cc7dc3` | planned | openspec/specs/language/enums |
| `CANON-13/set` | 3268-3279 | `240f92fbca14` | planned | openspec/specs/language/sets |
| `SECTION-22` | 3280-3282 | `f6a146590eb2` | structural | heading only |
| `CANON-18` | 3283-3303 | `c798859f64fa` | planned | openspec/specs/stdlib/standard-library |
| `CANON-17` | 3304-3336 | `b58a10142c44` | planned | openspec/specs/stdlib/console-io |
| `SECTION-23` | 3337-3339 | `f29a82eefbab` | structural | heading only |
| `CANON-19` | 3340-3341 | `2682b6b517b1` | structural | heading only |
| `CANON-19/bootstrap-exception-runtime` | 3342-3377 | `3732c1eb5dfc` | planned | openspec/specs/runtime/exception-runtime |
| `CANON-19/checked-integer-arithmetic` | 3378-3452 | `76e58b94adb4` | planned | openspec/specs/runtime/runtime-faults |
| `CANON-19/compiler-implementation-limits` | 3453-3472 | `4564c0b0dffc` | planned | openspec/specs/compiler/limits |
| `temporary-console-i-o-abi` | 3473-3484 | `ee67f38f23b2` | planned | docs/STATUS.md |
| `SECTION-24` | 3485-3487 | `d83447a408b7` | structural | heading only |
| `CANON-20~2` | 3488-3518 | `89bc9490baa9` | planned | openspec/specs/compiler/llvm-backend |
| `current-backend-support-status` | 3519-3520 | `92e4d170aa16` | structural | heading only |
| `B-WORKS` | 3521-3540 | `05b04ae6d0f9` | planned | docs/STATUS.md |
| `B-PARSED` | 3541-3546 | `010d10c5ad64` | planned | docs/STATUS.md |
| `B-GAPS` | 3547-3616 | `7eafb434525d` | planned | docs/STATUS.md |
| `SECTION-25` | 3617-3619 | `561c278c3296` | structural | heading only |
| `examples-policy` | 3620-3629 | `1ef48ef87dbf` | planned | docs/ENGINEERING.md |
| `B-EXAMPLES` | 3630-3638 | `3d9b7dd4c5e3` | planned | docs/STATUS.md |
| `SECTION-26` | 3639-3641 | `9a88a697ae99` | structural | heading only |
| `CANON-21` | 3642-3675 | `afab1f61829e` | planned | docs/ENGINEERING.md |
| `required-regression-layers` | 3676-3685 | `6098f4656540` | planned | docs/ENGINEERING.md |
| `SECTION-27` | 3686-3688 | `62cc5be85e73` | structural | heading only |
| `release-validation-policy` | 3689-3707 | `c2bd42d65889` | planned | docs/RELEASE.md |
| `DEV-2~2` | 3708-3744 | `6ff9fac762f9` | duplicate | `unit:DEV-2` |
| `GROUP-future~2` | 3745-3753 | `c0c3ec88bb92` | duplicate | `unit:GROUP-future` |
| `SECTION-28` | 3754-3756 | `b97b11b56381` | structural | heading only |
| `documentation-generation-policy` | 3757-3769 | `1d33470c8cd5` | planned | docs/ENGINEERING.md |
| `SECTION-29` | 3770-3772 | `000dd38c7ed9` | structural | heading only |
| `DEV-1` | 3773-3810 | `48712f26b690` | planned | docs/RELEASE.md |
| `portability-targets` | 3811-3857 | `2422ef6e3b0a` | planned | docs/RELEASE.md |
| `SECTION-30` | 3858-3862 | `683f7b9b00c7` | planned | docs/STATUS.md |
| `B-WORKS~2` | 3863-3879 | `65db813fde18` | planned | docs/STATUS.md |
| `B-PARSED~2` | 3880-3885 | `010d10c5ad64` | planned | docs/STATUS.md |
| `B-GAPS~2` | 3886-3953 | `d6299051224a` | planned | docs/STATUS.md |
| `B-CONFLICTS` | 3954-3961 | `fdc28764bc4a` | planned | docs/STATUS.md |
| `B-EXAMPLES~2` | 3962-3968 | `f85a9a4e4be5` | planned | docs/STATUS.md |
| `B-PROPOSALS` | 3969-3977 | `b5622600ecc3` | planned | openspec/changes (one change per open item) |
| `SECTION-31` | 3978-3980 | `0c7c92ea754c` | structural | heading only |
| `CANON-22` | 3981-4000 | `0fd628df7854` | planned | docs/ROADMAP.md |
| `GROUP-decisions` | 4001-4008 | `8b90cf0e6ca5` | planned | openspec/README.md (history) |
| `roadmap-to-1-0` | 4009-4023 | `de99ff09125d` | planned | docs/ROADMAP.md |
| `backend-strategy-directive` | 4024-4030 | `3e733152683f` | planned | docs/ENGINEERING.md |
| `backend-strategy-directive/core-principle` | 4031-4044 | `97d3972ccf1e` | planned | docs/ENGINEERING.md |
| `backend-strategy-directive/backend-maturation-sequence` | 4045-4060 | `ff2dbe76d57a` | planned | docs/ENGINEERING.md |
| `backend-strategy-directive/objective-criterion-for-leaving-clang` | 4061-4075 | `1cda38393426` | planned | docs/ENGINEERING.md |
| `backend-strategy-directive/scope-guard-for-backend-adjacent-work` | 4076-4082 | `859bab429005` | planned | docs/ENGINEERING.md |
| `compiler-module-architecture-directive` | 4083-4089 | `7dfc746e78b5` | planned | docs/ENGINEERING.md |
| `compiler-module-architecture-directive/principle-reactive-not-predictive` | 4090-4098 | `6bdbaef0d48c` | planned | docs/ENGINEERING.md |
| `compiler-module-architecture-directive/the-three-tests-for-extracting-a-module` | 4099-4114 | `9bdd4cb899e6` | planned | docs/ENGINEERING.md |
| `compiler-module-architecture-directive/anti-rules` | 4115-4123 | `c8700ead7c93` | planned | docs/ENGINEERING.md |
| `compiler-module-architecture-directive/working-rule-of-thumb` | 4124-4128 | `ec2c782d1c49` | planned | docs/ENGINEERING.md |
| `compiler-module-architecture-directive/timing` | 4129-4135 | `c4c0fef11f84` | planned | docs/ENGINEERING.md |
| `BANNER-end-of-inox-canonical-md-v3-8` | 4136-4141 | `784433b7be07` | planned | openspec/README.md (reading order) |
