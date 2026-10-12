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
| `BANNER-the-single-canonical-document` | 3-58 | `cdd93882e8c4` | planned | openspec/README.md (authority, maintainer, license, map) |
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
| `LOG/r3.58` | 528-556 | `c9679947a1a0` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.57` | 557-577 | `3bc8f8242e98` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.56` | 578-599 | `a64911c477d4` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.55` | 600-630 | `f8b2eae4b70f` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.54` | 631-647 | `156e7b7bd306` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.53` | 648-678 | `7d280ce2326d` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.52` | 679-709 | `a7dbe03eaf28` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.51` | 710-716 | `7e14e9ffa7a9` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.50` | 717-741 | `0441fbff3bf5` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.49` | 742-768 | `be098b1b06bd` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.48` | 769-790 | `afa50d5ec69f` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.47` | 791-803 | `72a0e9ea7e82` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.46` | 804-817 | `07eee37e2b47` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.45` | 818-846 | `e8bf1bc74e25` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.44` | 847-869 | `1ed18d87a1f6` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.43` | 870-879 | `7cec90575743` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.42` | 880-895 | `e00cff8892ed` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.41` | 896-901 | `d9b78ef08a85` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.40` | 902-920 | `3c2103c842ec` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.39` | 921-950 | `ff8a1f06c34f` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.38` | 951-961 | `d97e13c1b814` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.37` | 962-973 | `73f3f0239390` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.36` | 974-990 | `889932b86d67` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.35` | 991-1008 | `a40c544968ff` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.34` | 1009-1021 | `fa31c1e6a95f` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.33` | 1022-1044 | `546628eeb9d8` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.32` | 1045-1066 | `01b8acd407b7` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.31` | 1067-1080 | `c6f79fbad05a` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.30` | 1081-1102 | `82fc2948b1b0` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.29` | 1103-1163 | `411404a57a61` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.28` | 1164-1199 | `f95a9348aa62` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.27` | 1200-1255 | `9a521127b697` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.26` | 1256-1275 | `d412b402bbfb` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.25` | 1276-1288 | `ac5b09d96e50` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.24` | 1289-1311 | `977e3c458a59` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.23` | 1312-1351 | `1bd9020135e7` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.22` | 1352-1366 | `c3cd869369d8` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.21` | 1367-1431 | `fa631814dc07` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.20` | 1432-1460 | `c178428e9e51` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.19` | 1461-1476 | `58a9449923cc` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.18` | 1477-1555 | `63b0850a3b94` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.17` | 1556-1576 | `6b82cfa2bfef` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.16` | 1577-1597 | `504ebe528c6f` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.15a` | 1598-1611 | `8a99974e3b15` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.15` | 1612-1626 | `db45cccf8f29` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.14` | 1627-1637 | `45d2e2e7b0f4` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.13` | 1638-1649 | `e78d99b40dce` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.12` | 1650-1659 | `adf42d99bf9c` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.11` | 1660-1669 | `bb084ddde131` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.10` | 1670-1682 | `bb0c5292dc58` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.8` | 1683-1693 | `e2021267fde4` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.7` | 1694-1704 | `4c50f899f80e` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.6` | 1705-1721 | `f5c6a45807b0` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.5` | 1722-1726 | `1e634d009b80` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.4` | 1727-1732 | `a02b7bf01817` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.3` | 1733-1739 | `b73b838ee71a` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.2` | 1740-1746 | `3aae34cdb8de` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.1` | 1747-1759 | `1c1bfe44fee1` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.0` | 1760-1769 | `01fee319f99c` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v2.2` | 1770-1772 | `947024e547a5` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v2.1` | 1773-1775 | `831353b88b26` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v2.0` | 1776-1781 | `92161592c6f6` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v1.0` | 1782-1783 | `bf9fe6205559` | structural | heading only |
| `DEV-2` | 1784-1820 | `6ff9fac762f9` | planned | docs/RELEASE.md |
| `GROUP-future` | 1821-1829 | `c0c3ec88bb92` | planned | docs/ROADMAP.md |
| `SECTION-03` | 1830-1832 | `8f4b634fd646` | structural | heading only |
| `agent-rules` | 1833-1866 | `f699b67648da` | planned | AGENTS.md (governance) |
| `SECTION-04` | 1867-1871 | `03786d4742d7` | planned | docs/ROADMAP.md |
| `B-OPEN` | 1872-1916 | `4e5f8d3984d1` | planned | openspec/changes (one change per open item) |
| `FUTURE-1` | 1917-1924 | `0a1e1448d12b` | planned | docs/ROADMAP.md |
| `FUTURE-2` | 1925-1929 | `eb05f449bfc6` | planned | docs/ROADMAP.md |
| `FUTURE-3` | 1930-1935 | `2c09d65cfded` | planned | docs/ROADMAP.md |
| `FUTURE-4` | 1936-1939 | `62d8fde09ca2` | planned | docs/ROADMAP.md |
| `FUTURE-5` | 1940-1944 | `041157e3cd11` | planned | docs/ROADMAP.md |
| `FUTURE-6` | 1945-1948 | `a070988b84e8` | planned | docs/ROADMAP.md |
| `FUTURE-7` | 1949-1961 | `116a4980e694` | planned | docs/ROADMAP.md |
| `GROUP-open-questions` | 1962-1985 | `9806de6ee55e` | planned | docs/ROADMAP.md, docs/STATUS.md |
| `BANNER-agent-rules` | 1986-2019 | `1dd939e2384f` | duplicate | `unit:agent-rules` |
| `BANNER-layer-b-implementation-status` | 2020-2040 | `cb181e7e1383` | planned | docs/STATUS.md |
| `SECTION-05` | 2041-2045 | `3950191b4348` | planned | openspec/README.md (history) |
| `ADR-0001` | 2046-2050 | `6597d424187c` | copied | `openspec/changes/archive/2026-05-28-adr-0001-llvm-backend` |
| `ADR-0002` | 2051-2056 | `24673434b56e` | copied | `openspec/changes/archive/2026-05-28-adr-0002-block-syntax` |
| `ADR-0003` | 2057-2060 | `91f21c8db59b` | copied | `openspec/changes/archive/2026-05-28-adr-0003-type-system` |
| `ADR-0004` | 2061-2069 | `a7ea289039f3` | copied | `openspec/changes/archive/2026-05-28-adr-0004-exceptions` |
| `ADR-0005` | 2070-2093 | `b3487abfdaa3` | copied | `openspec/changes/archive/2026-05-31-adr-0005-consolidated-0-1-language-decisions` |
| `ADR-0006` | 2094-2141 | `00635fb17fa5` | copied | `openspec/changes/archive/2026-05-31-adr-0006-inox-0-1-language-constitution` |
| `ADR-0017` | 2142-2167 | `99a0ae3ad662` | copied | `openspec/changes/archive/2026-10-11-adr-0017-read-only-slices` |
| `ADR-0016` | 2168-2199 | `ae224ec3acef` | copied | `openspec/changes/archive/2026-10-11-adr-0016-ref-and-ref-mut-parameters` |
| `ADR-0015` | 2200-2217 | `d138712bb9d2` | copied | `openspec/changes/archive/2026-10-10-adr-0015-storing-into-a-natural-checks-the-range` |
| `ADR-0014` | 2218-2245 | `8f83b1f7aa88` | copied | `openspec/changes/archive/2026-10-10-adr-0014-natural-byte-and-the-closed-numeric-set` |
| `ADR-0013` | 2246-2271 | `8b43a6f9c9e7` | copied | `openspec/changes/archive/2026-10-10-adr-0013-conditional-expression` |
| `ADR-0012` | 2272-2285 | `387bf21df2e0` | copied | `openspec/changes/archive/2026-10-10-adr-0012-compound-assignment` |
| `ADR-0011` | 2286-2318 | `8cd31c3b1d14` | copied | `openspec/changes/archive/2026-10-09-adr-0011-case-in-the-ada-spark-style` |
| `ADR-0010` | 2319-2330 | `01ebe198452d` | copied | `openspec/changes/archive/2026-10-09-adr-0010-step-in-the-for-header` |
| `ADR-0009` | 2331-2346 | `9fb0711acf2b` | copied | `openspec/changes/archive/2026-10-09-adr-0009-chained-assignment` |
| `ADR-0008` | 2347-2364 | `6cbc9717bb6c` | copied | `openspec/changes/archive/2026-10-09-adr-0008-precedence-of-and-in` |
| `ADR-0007` | 2365-2377 | `d8e65f343093` | copied | `openspec/changes/archive/2026-10-09-adr-0007-loop-exit-and-try-cleanup-keywords` |
| `GROUP-development` | 2378-2393 | `6f6455b69203` | structural | heading only |
| `SECTION-06` | 2394-2396 | `1f3fa370b3e3` | structural | heading only |
| `canonical-user-facing-cli-contract` | 2397-2412 | `45e0e5d66ff0` | planned | openspec/specs/compiler/cli |
| `current-observed-driver-status` | 2413-2418 | `15c15b4e82d8` | planned | docs/STATUS.md |
| `SECTION-07` | 2419-2421 | `f2dfbd93d1af` | structural | heading only |
| `source-file-structure` | 2422-2433 | `f905deb42055` | planned | openspec/specs/language/modules |
| `SECTION-08` | 2434-2436 | `83d42d4dfe69` | structural | heading only |
| `CANON-2` | 2437-2468 | `1784cd8f9c86` | planned | openspec/specs/language/lexical |
| `SECTION-09` | 2469-2471 | `151b987c27db` | structural | heading only |
| `CANON-3` | 2472-2492 | `86f97b768bdb` | planned | openspec/specs/language/modules |
| `SECTION-10` | 2493-2495 | `539a6368d8f5` | structural | heading only |
| `CANON-4` | 2496-2551 | `011a17049109` | planned | openspec/specs/language/block-syntax |
| `CANON-7` | 2552-2563 | `954891e7e64b` | planned | openspec/specs/language/block-syntax |
| `SECTION-11` | 2564-2566 | `cc93dd9983b8` | structural | heading only |
| `CANON-5` | 2567-2571 | `107db4bb6efc` | planned | openspec/specs/language/variables |
| `CANON-5/type-section` | 2572-2586 | `ee2b644e002f` | planned | openspec/specs/language/declaration-sections |
| `CANON-5/local-variables-three-valid-forms-only` | 2587-2594 | `7acb630f2600` | planned | openspec/specs/language/variables |
| `CANON-5/grouped-declarations` | 2595-2607 | `211d54982dd6` | planned | openspec/specs/language/variables |
| `CANON-5/compound-assignment` | 2608-2617 | `ac2d6220edcf` | planned | openspec/specs/language/assignment |
| `CANON-5/assignment-is-a-statement-chained-assign` | 2618-2677 | `4a3bbaf10235` | planned | openspec/specs/language/assignment |
| `CANON-5/safe-type-inference` | 2678-2686 | `8cabd03f259a` | planned | openspec/specs/language/variables |
| `CANON-5/const-and-state` | 2687-2696 | `c77d846e37bf` | planned | openspec/specs/language/declaration-sections, openspec/specs/language/module-state |
| `SECTION-12` | 2697-2699 | `21806ebd31bd` | structural | heading only |
| `CANON-8` | 2700-2701 | `44025c9660f2` | structural | heading only |
| `CANON-8/canonical-type-hierarchy` | 2702-2743 | `b64e68e42b5b` | planned | openspec/specs/language/numeric-types, openspec/specs/language/natural, openspec/specs/language/fixed-width-integers |
| `CANON-8/generics-use-angle-brackets` | 2744-2752 | `599f5b0a40cf` | planned | openspec/specs/language/generics |
| `CANON-8/conversions` | 2753-2765 | `233a06f0e0e2` | planned | openspec/specs/language/conversions |
| `CANON-8/decimal-literal-form` | 2766-2785 | `5d29e066b6b5` | planned | openspec/specs/language/decimal-types |
| `CANON-8/operators-and-numeric-law` | 2786-2813 | `4234270d384a` | planned | openspec/specs/language/numeric-types |
| `SECTION-13` | 2814-2824 | `2897fbbce2ae` | planned | openspec/specs/language/variables, openspec/specs/language/mutability |
| `SECTION-13/variable-declaration-rules` | 2825-2828 | `754e4e1781da` | planned | openspec/specs/language/variables |
| `CANON-10` | 2829-2853 | `9e51efaea2ef` | planned | openspec/specs/language/mutability |
| `SECTION-14` | 2854-2856 | `47360f55ede4` | structural | heading only |
| `scoping-and-no-shadowing` | 2857-2869 | `212122b8a58f` | planned | openspec/specs/language/scoping |
| `SECTION-15` | 2870-2872 | `60939e1901e7` | structural | heading only |
| `CANON-6` | 2873-2893 | `deea50879587` | planned | openspec/specs/language/routines |
| `SECTION-16` | 2894-2896 | `70008477d760` | structural | heading only |
| `control-flow-constructs` | 2897-2898 | `49a7482f73bc` | structural | heading only |
| `CANON-12` | 2899-2902 | `778fe3b1480b` | planned | openspec/specs/language/control-flow |
| `CANON-12/if-elif-else` | 2903-2913 | `3cf7facb9c42` | planned | openspec/specs/language/control-flow |
| `CANON-15` | 2914-3063 | `a52eb819fcce` | planned | openspec/specs/language/exceptions |
| `CANON-15/current-v3-17-lowering-note` | 3064-3074 | `0c522c2d339d` | planned | docs/STATUS.md |
| `CANON-11` | 3075-3106 | `ad57cb2fc650` | planned | openspec/specs/language/with-statement |
| `SECTION-17` | 3107-3109 | `f11baea07e11` | structural | heading only |
| `SECTION-17/while` | 3110-3115 | `ba94f83af4d4` | planned | openspec/specs/language/loops |
| `SECTION-17/repeat-until` | 3116-3125 | `45814b3807ec` | planned | openspec/specs/language/loops |
| `SECTION-17/for-in-range` | 3126-3156 | `363715f8a505` | planned | openspec/specs/language/loops |
| `SECTION-17/conditional-expression` | 3157-3165 | `2dceb1930154` | planned | openspec/specs/language/conditional-expression |
| `SECTION-17/case` | 3166-3191 | `6639c7cf4313` | mirrored | `openspec/specs/language/case-statement/spec.md` (Enum part: change enum-range-set) |
| `SECTION-17/unless` | 3192-3196 | `c75b89637df7` | mirrored | `openspec/specs/language/case-statement/spec.md` |
| `SECTION-18` | 3197-3199 | `e522f90c7379` | structural | heading only |
| `CANON-9` | 3200-3241 | `8a021bf1ff92` | planned | openspec/specs/language/structs, openspec/specs/language/associated-methods |
| `CANON-9/named-struct-construction` | 3242-3278 | `d0090df0b373` | planned | openspec/specs/language/structs |
| `SECTION-19` | 3279-3281 | `e0d3647383a1` | structural | heading only |
| `CANON-14` | 3282-3310 | `3bb68612b459` | planned | openspec/specs/language/strings |
| `SECTION-20` | 3311-3315 | `3b560adced11` | planned | openspec/specs/language/numeric-types |
| `numeric-semantics-summary` | 3316-3326 | `bda5b68d9240` | planned | openspec/specs/language/numeric-types |
| `CANON-20` | 3327-3334 | `b20c1473c6e9` | planned | openspec/specs/language/operator-precedence |
| `CANON-20/design-intent` | 3335-3343 | `b9c61f6ebef4` | planned | openspec/specs/language/operator-precedence |
| `CANON-20/precedence-table` | 3344-3380 | `606ca4e1d824` | planned | openspec/specs/language/operator-precedence |
| `CANON-20/mandatory-parentheses-rule` | 3381-3401 | `6d49217d2355` | planned | openspec/specs/language/operator-precedence |
| `CANON-20/verifiability` | 3402-3417 | `40b3313dba9d` | planned | openspec/specs/language/operator-precedence |
| `SECTION-21` | 3418-3420 | `ec02c6ed9865` | structural | heading only |
| `CANON-13` | 3421-3422 | `f59f57e57286` | structural | heading only |
| `CANON-13/array` | 3423-3430 | `f2d44c0dd686` | mirrored | `openspec/specs/language/fixed-arrays/spec.md` |
| `CANON-13/vector` | 3431-3437 | `c99a2607fa26` | planned | openspec/specs/language/vectors |
| `CANON-13/range` | 3438-3443 | `c7b61a41d8eb` | planned | openspec/specs/language/ranges |
| `CANON-13/enum` | 3444-3457 | `2d11b4cc7dc3` | planned | openspec/specs/language/enums |
| `CANON-13/set` | 3458-3469 | `240f92fbca14` | planned | openspec/specs/language/sets |
| `SECTION-22` | 3470-3472 | `f6a146590eb2` | structural | heading only |
| `CANON-18` | 3473-3516 | `9ee704ec19b4` | planned | openspec/specs/stdlib/standard-library |
| `CANON-17` | 3517-3554 | `05f9d6b3d917` | planned | openspec/specs/stdlib/console-io |
| `SECTION-23` | 3555-3557 | `f29a82eefbab` | structural | heading only |
| `CANON-19` | 3558-3559 | `2682b6b517b1` | structural | heading only |
| `CANON-19/bootstrap-exception-runtime` | 3560-3595 | `3732c1eb5dfc` | planned | openspec/specs/runtime/exception-runtime |
| `CANON-19/checked-integer-arithmetic` | 3596-3672 | `8b1088d2777d` | planned | openspec/specs/runtime/runtime-faults |
| `CANON-19/compiler-implementation-limits` | 3673-3692 | `4564c0b0dffc` | planned | openspec/specs/compiler/limits |
| `temporary-console-i-o-abi` | 3693-3704 | `ee67f38f23b2` | planned | docs/STATUS.md |
| `SECTION-24` | 3705-3707 | `d83447a408b7` | structural | heading only |
| `CANON-20~2` | 3708-3738 | `89bc9490baa9` | planned | openspec/specs/compiler/llvm-backend |
| `current-backend-support-status` | 3739-3740 | `92e4d170aa16` | structural | heading only |
| `B-WORKS` | 3741-3760 | `05b04ae6d0f9` | planned | docs/STATUS.md |
| `B-PARSED` | 3761-3766 | `010d10c5ad64` | planned | docs/STATUS.md |
| `B-GAPS` | 3767-3837 | `376488d68493` | planned | docs/STATUS.md |
| `SECTION-25` | 3838-3840 | `561c278c3296` | structural | heading only |
| `examples-policy` | 3841-3850 | `1ef48ef87dbf` | planned | docs/ENGINEERING.md |
| `B-EXAMPLES` | 3851-3859 | `3d9b7dd4c5e3` | planned | docs/STATUS.md |
| `SECTION-26` | 3860-3862 | `9a88a697ae99` | structural | heading only |
| `CANON-21` | 3863-3896 | `afab1f61829e` | planned | docs/ENGINEERING.md |
| `required-regression-layers` | 3897-3906 | `6098f4656540` | planned | docs/ENGINEERING.md |
| `SECTION-27` | 3907-3909 | `62cc5be85e73` | structural | heading only |
| `release-validation-policy` | 3910-3928 | `c2bd42d65889` | planned | docs/RELEASE.md |
| `DEV-2~2` | 3929-3965 | `6ff9fac762f9` | duplicate | `unit:DEV-2` |
| `GROUP-future~2` | 3966-3974 | `c0c3ec88bb92` | duplicate | `unit:GROUP-future` |
| `SECTION-28` | 3975-3977 | `b97b11b56381` | structural | heading only |
| `documentation-generation-policy` | 3978-3990 | `1d33470c8cd5` | planned | docs/ENGINEERING.md |
| `SECTION-29` | 3991-3993 | `000dd38c7ed9` | structural | heading only |
| `DEV-1` | 3994-4031 | `48712f26b690` | planned | docs/RELEASE.md |
| `portability-targets` | 4032-4078 | `2422ef6e3b0a` | planned | docs/RELEASE.md |
| `SECTION-30` | 4079-4083 | `683f7b9b00c7` | planned | docs/STATUS.md |
| `B-WORKS~2` | 4084-4100 | `65db813fde18` | planned | docs/STATUS.md |
| `B-PARSED~2` | 4101-4106 | `010d10c5ad64` | planned | docs/STATUS.md |
| `B-GAPS~2` | 4107-4175 | `a5ea1b1eb65f` | planned | docs/STATUS.md |
| `B-CONFLICTS` | 4176-4183 | `fdc28764bc4a` | planned | docs/STATUS.md |
| `B-EXAMPLES~2` | 4184-4190 | `f85a9a4e4be5` | planned | docs/STATUS.md |
| `B-PROPOSALS` | 4191-4199 | `b5622600ecc3` | planned | openspec/changes (one change per open item) |
| `SECTION-31` | 4200-4202 | `0c7c92ea754c` | structural | heading only |
| `CANON-22` | 4203-4222 | `0fd628df7854` | planned | docs/ROADMAP.md |
| `GROUP-decisions` | 4223-4230 | `8b90cf0e6ca5` | planned | openspec/README.md (history) |
| `roadmap-to-1-0` | 4231-4245 | `de99ff09125d` | planned | docs/ROADMAP.md |
| `backend-strategy-directive` | 4246-4252 | `3e733152683f` | planned | docs/ENGINEERING.md |
| `backend-strategy-directive/core-principle` | 4253-4266 | `97d3972ccf1e` | planned | docs/ENGINEERING.md |
| `backend-strategy-directive/backend-maturation-sequence` | 4267-4282 | `ff2dbe76d57a` | planned | docs/ENGINEERING.md |
| `backend-strategy-directive/objective-criterion-for-leaving-clang` | 4283-4297 | `1cda38393426` | planned | docs/ENGINEERING.md |
| `backend-strategy-directive/scope-guard-for-backend-adjacent-work` | 4298-4304 | `859bab429005` | planned | docs/ENGINEERING.md |
| `compiler-module-architecture-directive` | 4305-4311 | `7dfc746e78b5` | planned | docs/ENGINEERING.md |
| `compiler-module-architecture-directive/principle-reactive-not-predictive` | 4312-4320 | `6bdbaef0d48c` | planned | docs/ENGINEERING.md |
| `compiler-module-architecture-directive/the-three-tests-for-extracting-a-module` | 4321-4336 | `9bdd4cb899e6` | planned | docs/ENGINEERING.md |
| `compiler-module-architecture-directive/anti-rules` | 4337-4345 | `c8700ead7c93` | planned | docs/ENGINEERING.md |
| `compiler-module-architecture-directive/working-rule-of-thumb` | 4346-4350 | `ec2c782d1c49` | planned | docs/ENGINEERING.md |
| `compiler-module-architecture-directive/timing` | 4351-4357 | `c4c0fef11f84` | planned | docs/ENGINEERING.md |
| `BANNER-end-of-inox-canonical-md-v3-8` | 4358-4363 | `784433b7be07` | planned | openspec/README.md (reading order) |
