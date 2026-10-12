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
| `BANNER-the-single-canonical-document` | 3-56 | `e2d1aceb1e55` | planned | openspec/README.md (authority, maintainer, license, map) |
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
| `LOG/v3.56` | 508-529 | `a64911c477d4` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.55` | 530-560 | `f8b2eae4b70f` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.54` | 561-577 | `156e7b7bd306` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.53` | 578-608 | `7d280ce2326d` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.52` | 609-639 | `a7dbe03eaf28` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.51` | 640-646 | `7e14e9ffa7a9` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.50` | 647-671 | `0441fbff3bf5` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.49` | 672-698 | `be098b1b06bd` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.48` | 699-720 | `afa50d5ec69f` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.47` | 721-733 | `72a0e9ea7e82` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.46` | 734-747 | `07eee37e2b47` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.45` | 748-776 | `e8bf1bc74e25` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.44` | 777-799 | `1ed18d87a1f6` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.43` | 800-809 | `7cec90575743` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.42` | 810-825 | `e00cff8892ed` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.41` | 826-831 | `d9b78ef08a85` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.40` | 832-850 | `3c2103c842ec` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.39` | 851-880 | `ff8a1f06c34f` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.38` | 881-891 | `d97e13c1b814` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.37` | 892-903 | `73f3f0239390` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.36` | 904-920 | `889932b86d67` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.35` | 921-938 | `a40c544968ff` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.34` | 939-951 | `fa31c1e6a95f` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.33` | 952-974 | `546628eeb9d8` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.32` | 975-996 | `01b8acd407b7` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.31` | 997-1010 | `c6f79fbad05a` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.30` | 1011-1032 | `82fc2948b1b0` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.29` | 1033-1093 | `411404a57a61` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.28` | 1094-1129 | `f95a9348aa62` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.27` | 1130-1185 | `9a521127b697` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.26` | 1186-1205 | `d412b402bbfb` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.25` | 1206-1218 | `ac5b09d96e50` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.24` | 1219-1241 | `977e3c458a59` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.23` | 1242-1281 | `1bd9020135e7` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.22` | 1282-1296 | `c3cd869369d8` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.21` | 1297-1361 | `fa631814dc07` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.20` | 1362-1390 | `c178428e9e51` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.19` | 1391-1406 | `58a9449923cc` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.18` | 1407-1485 | `63b0850a3b94` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.17` | 1486-1506 | `6b82cfa2bfef` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.16` | 1507-1527 | `504ebe528c6f` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.15a` | 1528-1541 | `8a99974e3b15` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.15` | 1542-1556 | `db45cccf8f29` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.14` | 1557-1567 | `45d2e2e7b0f4` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.13` | 1568-1579 | `e78d99b40dce` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.12` | 1580-1589 | `adf42d99bf9c` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.11` | 1590-1599 | `bb084ddde131` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.10` | 1600-1612 | `bb0c5292dc58` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.8` | 1613-1623 | `e2021267fde4` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.7` | 1624-1634 | `4c50f899f80e` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.6` | 1635-1651 | `f5c6a45807b0` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.5` | 1652-1656 | `1e634d009b80` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.4` | 1657-1662 | `a02b7bf01817` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.3` | 1663-1669 | `b73b838ee71a` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.2` | 1670-1676 | `3aae34cdb8de` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.1` | 1677-1689 | `1c1bfe44fee1` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v3.0` | 1690-1699 | `01fee319f99c` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v2.2` | 1700-1702 | `947024e547a5` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v2.1` | 1703-1705 | `831353b88b26` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v2.0` | 1706-1711 | `92161592c6f6` | planned | docs/history/canon-changelog.md (frozen at retirement) |
| `LOG/v1.0` | 1712-1713 | `bf9fe6205559` | structural | heading only |
| `DEV-2` | 1714-1750 | `6ff9fac762f9` | planned | docs/RELEASE.md |
| `GROUP-future` | 1751-1759 | `c0c3ec88bb92` | planned | docs/ROADMAP.md |
| `SECTION-03` | 1760-1762 | `8f4b634fd646` | structural | heading only |
| `agent-rules` | 1763-1796 | `f699b67648da` | planned | AGENTS.md (governance) |
| `SECTION-04` | 1797-1801 | `03786d4742d7` | planned | docs/ROADMAP.md |
| `B-OPEN` | 1802-1846 | `4e5f8d3984d1` | planned | openspec/changes (one change per open item) |
| `FUTURE-1` | 1847-1854 | `0a1e1448d12b` | planned | docs/ROADMAP.md |
| `FUTURE-2` | 1855-1859 | `eb05f449bfc6` | planned | docs/ROADMAP.md |
| `FUTURE-3` | 1860-1865 | `2c09d65cfded` | planned | docs/ROADMAP.md |
| `FUTURE-4` | 1866-1869 | `62d8fde09ca2` | planned | docs/ROADMAP.md |
| `FUTURE-5` | 1870-1874 | `041157e3cd11` | planned | docs/ROADMAP.md |
| `FUTURE-6` | 1875-1878 | `a070988b84e8` | planned | docs/ROADMAP.md |
| `FUTURE-7` | 1879-1891 | `116a4980e694` | planned | docs/ROADMAP.md |
| `GROUP-open-questions` | 1892-1915 | `9806de6ee55e` | planned | docs/ROADMAP.md, docs/STATUS.md |
| `BANNER-agent-rules` | 1916-1949 | `1dd939e2384f` | duplicate | `unit:agent-rules` |
| `BANNER-layer-b-implementation-status` | 1950-1970 | `cb181e7e1383` | planned | docs/STATUS.md |
| `SECTION-05` | 1971-1975 | `3950191b4348` | planned | openspec/README.md (history) |
| `ADR-0001` | 1976-1980 | `6597d424187c` | copied | `openspec/changes/archive/2026-05-28-adr-0001-llvm-backend` |
| `ADR-0002` | 1981-1986 | `24673434b56e` | copied | `openspec/changes/archive/2026-05-28-adr-0002-block-syntax` |
| `ADR-0003` | 1987-1990 | `91f21c8db59b` | copied | `openspec/changes/archive/2026-05-28-adr-0003-type-system` |
| `ADR-0004` | 1991-1999 | `a7ea289039f3` | copied | `openspec/changes/archive/2026-05-28-adr-0004-exceptions` |
| `ADR-0005` | 2000-2023 | `b3487abfdaa3` | copied | `openspec/changes/archive/2026-05-31-adr-0005-consolidated-0-1-language-decisions` |
| `ADR-0006` | 2024-2071 | `00635fb17fa5` | copied | `openspec/changes/archive/2026-05-31-adr-0006-inox-0-1-language-constitution` |
| `ADR-0017` | 2072-2097 | `99a0ae3ad662` | copied | `openspec/changes/archive/2026-10-11-adr-0017-read-only-slices` |
| `ADR-0016` | 2098-2129 | `ae224ec3acef` | copied | `openspec/changes/archive/2026-10-11-adr-0016-ref-and-ref-mut-parameters` |
| `ADR-0015` | 2130-2147 | `d138712bb9d2` | copied | `openspec/changes/archive/2026-10-10-adr-0015-storing-into-a-natural-checks-the-range` |
| `ADR-0014` | 2148-2175 | `8f83b1f7aa88` | copied | `openspec/changes/archive/2026-10-10-adr-0014-natural-byte-and-the-closed-numeric-set` |
| `ADR-0013` | 2176-2201 | `8b43a6f9c9e7` | copied | `openspec/changes/archive/2026-10-10-adr-0013-conditional-expression` |
| `ADR-0012` | 2202-2215 | `387bf21df2e0` | copied | `openspec/changes/archive/2026-10-10-adr-0012-compound-assignment` |
| `ADR-0011` | 2216-2248 | `8cd31c3b1d14` | copied | `openspec/changes/archive/2026-10-09-adr-0011-case-in-the-ada-spark-style` |
| `ADR-0010` | 2249-2260 | `01ebe198452d` | copied | `openspec/changes/archive/2026-10-09-adr-0010-step-in-the-for-header` |
| `ADR-0009` | 2261-2276 | `9fb0711acf2b` | copied | `openspec/changes/archive/2026-10-09-adr-0009-chained-assignment` |
| `ADR-0008` | 2277-2294 | `6cbc9717bb6c` | copied | `openspec/changes/archive/2026-10-09-adr-0008-precedence-of-and-in` |
| `ADR-0007` | 2295-2307 | `d8e65f343093` | copied | `openspec/changes/archive/2026-10-09-adr-0007-loop-exit-and-try-cleanup-keywords` |
| `GROUP-development` | 2308-2323 | `6f6455b69203` | structural | heading only |
| `SECTION-06` | 2324-2326 | `1f3fa370b3e3` | structural | heading only |
| `canonical-user-facing-cli-contract` | 2327-2342 | `45e0e5d66ff0` | planned | openspec/specs/compiler/cli |
| `current-observed-driver-status` | 2343-2348 | `15c15b4e82d8` | planned | docs/STATUS.md |
| `SECTION-07` | 2349-2351 | `f2dfbd93d1af` | structural | heading only |
| `source-file-structure` | 2352-2363 | `f905deb42055` | planned | openspec/specs/language/modules |
| `SECTION-08` | 2364-2366 | `83d42d4dfe69` | structural | heading only |
| `CANON-2` | 2367-2398 | `1784cd8f9c86` | planned | openspec/specs/language/lexical |
| `SECTION-09` | 2399-2401 | `151b987c27db` | structural | heading only |
| `CANON-3` | 2402-2422 | `eb3765442875` | planned | openspec/specs/language/modules |
| `SECTION-10` | 2423-2425 | `539a6368d8f5` | structural | heading only |
| `CANON-4` | 2426-2481 | `011a17049109` | planned | openspec/specs/language/block-syntax |
| `CANON-7` | 2482-2493 | `954891e7e64b` | planned | openspec/specs/language/block-syntax |
| `SECTION-11` | 2494-2496 | `cc93dd9983b8` | structural | heading only |
| `CANON-5` | 2497-2501 | `107db4bb6efc` | planned | openspec/specs/language/variables |
| `CANON-5/type-section` | 2502-2516 | `ee2b644e002f` | planned | openspec/specs/language/declaration-sections |
| `CANON-5/local-variables-three-valid-forms-only` | 2517-2524 | `7acb630f2600` | planned | openspec/specs/language/variables |
| `CANON-5/grouped-declarations` | 2525-2537 | `211d54982dd6` | planned | openspec/specs/language/variables |
| `CANON-5/compound-assignment` | 2538-2547 | `ac2d6220edcf` | planned | openspec/specs/language/assignment |
| `CANON-5/assignment-is-a-statement-chained-assign` | 2548-2607 | `4a3bbaf10235` | planned | openspec/specs/language/assignment |
| `CANON-5/safe-type-inference` | 2608-2616 | `8cabd03f259a` | planned | openspec/specs/language/variables |
| `CANON-5/const-and-state` | 2617-2626 | `c77d846e37bf` | planned | openspec/specs/language/declaration-sections, openspec/specs/language/module-state |
| `SECTION-12` | 2627-2629 | `21806ebd31bd` | structural | heading only |
| `CANON-8` | 2630-2631 | `44025c9660f2` | structural | heading only |
| `CANON-8/canonical-type-hierarchy` | 2632-2673 | `d404151f7afe` | planned | openspec/specs/language/numeric-types, openspec/specs/language/natural, openspec/specs/language/fixed-width-integers |
| `CANON-8/generics-use-angle-brackets` | 2674-2682 | `599f5b0a40cf` | planned | openspec/specs/language/generics |
| `CANON-8/conversions` | 2683-2695 | `233a06f0e0e2` | planned | openspec/specs/language/conversions |
| `CANON-8/decimal-literal-form` | 2696-2715 | `5d29e066b6b5` | planned | openspec/specs/language/decimal-types |
| `CANON-8/operators-and-numeric-law` | 2716-2743 | `4234270d384a` | planned | openspec/specs/language/numeric-types |
| `SECTION-13` | 2744-2754 | `2897fbbce2ae` | planned | openspec/specs/language/variables, openspec/specs/language/mutability |
| `SECTION-13/variable-declaration-rules` | 2755-2758 | `754e4e1781da` | planned | openspec/specs/language/variables |
| `CANON-10` | 2759-2783 | `9e51efaea2ef` | planned | openspec/specs/language/mutability |
| `SECTION-14` | 2784-2786 | `47360f55ede4` | structural | heading only |
| `scoping-and-no-shadowing` | 2787-2799 | `212122b8a58f` | planned | openspec/specs/language/scoping |
| `SECTION-15` | 2800-2802 | `60939e1901e7` | structural | heading only |
| `CANON-6` | 2803-2823 | `deea50879587` | planned | openspec/specs/language/routines |
| `SECTION-16` | 2824-2826 | `70008477d760` | structural | heading only |
| `control-flow-constructs` | 2827-2828 | `49a7482f73bc` | structural | heading only |
| `CANON-12` | 2829-2832 | `778fe3b1480b` | planned | openspec/specs/language/control-flow |
| `CANON-12/if-elif-else` | 2833-2843 | `3cf7facb9c42` | planned | openspec/specs/language/control-flow |
| `CANON-15` | 2844-2993 | `a52eb819fcce` | planned | openspec/specs/language/exceptions |
| `CANON-15/current-v3-17-lowering-note` | 2994-3004 | `0c522c2d339d` | planned | docs/STATUS.md |
| `CANON-11` | 3005-3036 | `ad57cb2fc650` | planned | openspec/specs/language/with-statement |
| `SECTION-17` | 3037-3039 | `f11baea07e11` | structural | heading only |
| `SECTION-17/while` | 3040-3045 | `ba94f83af4d4` | planned | openspec/specs/language/loops |
| `SECTION-17/repeat-until` | 3046-3055 | `45814b3807ec` | planned | openspec/specs/language/loops |
| `SECTION-17/for-in-range` | 3056-3086 | `363715f8a505` | planned | openspec/specs/language/loops |
| `SECTION-17/conditional-expression` | 3087-3095 | `2dceb1930154` | planned | openspec/specs/language/conditional-expression |
| `SECTION-17/case` | 3096-3121 | `6639c7cf4313` | mirrored | `openspec/specs/language/case-statement/spec.md` (Enum part: change enum-range-set) |
| `SECTION-17/unless` | 3122-3126 | `c75b89637df7` | mirrored | `openspec/specs/language/case-statement/spec.md` |
| `SECTION-18` | 3127-3129 | `e522f90c7379` | structural | heading only |
| `CANON-9` | 3130-3171 | `e7f23cbacfe3` | planned | openspec/specs/language/structs, openspec/specs/language/associated-methods |
| `CANON-9/named-struct-construction` | 3172-3208 | `d0090df0b373` | planned | openspec/specs/language/structs |
| `SECTION-19` | 3209-3211 | `e0d3647383a1` | structural | heading only |
| `CANON-14` | 3212-3237 | `9ed1b7483080` | planned | openspec/specs/language/strings |
| `SECTION-20` | 3238-3242 | `3b560adced11` | planned | openspec/specs/language/numeric-types |
| `numeric-semantics-summary` | 3243-3253 | `bda5b68d9240` | planned | openspec/specs/language/numeric-types |
| `CANON-20` | 3254-3261 | `b20c1473c6e9` | planned | openspec/specs/language/operator-precedence |
| `CANON-20/design-intent` | 3262-3270 | `b9c61f6ebef4` | planned | openspec/specs/language/operator-precedence |
| `CANON-20/precedence-table` | 3271-3307 | `606ca4e1d824` | planned | openspec/specs/language/operator-precedence |
| `CANON-20/mandatory-parentheses-rule` | 3308-3328 | `6d49217d2355` | planned | openspec/specs/language/operator-precedence |
| `CANON-20/verifiability` | 3329-3344 | `40b3313dba9d` | planned | openspec/specs/language/operator-precedence |
| `SECTION-21` | 3345-3347 | `ec02c6ed9865` | structural | heading only |
| `CANON-13` | 3348-3349 | `f59f57e57286` | structural | heading only |
| `CANON-13/array` | 3350-3357 | `f2d44c0dd686` | mirrored | `openspec/specs/language/fixed-arrays/spec.md` |
| `CANON-13/vector` | 3358-3364 | `c99a2607fa26` | planned | openspec/specs/language/vectors |
| `CANON-13/range` | 3365-3370 | `c7b61a41d8eb` | planned | openspec/specs/language/ranges |
| `CANON-13/enum` | 3371-3384 | `2d11b4cc7dc3` | planned | openspec/specs/language/enums |
| `CANON-13/set` | 3385-3396 | `240f92fbca14` | planned | openspec/specs/language/sets |
| `SECTION-22` | 3397-3399 | `f6a146590eb2` | structural | heading only |
| `CANON-18` | 3400-3420 | `c798859f64fa` | planned | openspec/specs/stdlib/standard-library |
| `CANON-17` | 3421-3458 | `05f9d6b3d917` | planned | openspec/specs/stdlib/console-io |
| `SECTION-23` | 3459-3461 | `f29a82eefbab` | structural | heading only |
| `CANON-19` | 3462-3463 | `2682b6b517b1` | structural | heading only |
| `CANON-19/bootstrap-exception-runtime` | 3464-3499 | `3732c1eb5dfc` | planned | openspec/specs/runtime/exception-runtime |
| `CANON-19/checked-integer-arithmetic` | 3500-3576 | `8b1088d2777d` | planned | openspec/specs/runtime/runtime-faults |
| `CANON-19/compiler-implementation-limits` | 3577-3596 | `4564c0b0dffc` | planned | openspec/specs/compiler/limits |
| `temporary-console-i-o-abi` | 3597-3608 | `ee67f38f23b2` | planned | docs/STATUS.md |
| `SECTION-24` | 3609-3611 | `d83447a408b7` | structural | heading only |
| `CANON-20~2` | 3612-3642 | `89bc9490baa9` | planned | openspec/specs/compiler/llvm-backend |
| `current-backend-support-status` | 3643-3644 | `92e4d170aa16` | structural | heading only |
| `B-WORKS` | 3645-3664 | `05b04ae6d0f9` | planned | docs/STATUS.md |
| `B-PARSED` | 3665-3670 | `010d10c5ad64` | planned | docs/STATUS.md |
| `B-GAPS` | 3671-3741 | `376488d68493` | planned | docs/STATUS.md |
| `SECTION-25` | 3742-3744 | `561c278c3296` | structural | heading only |
| `examples-policy` | 3745-3754 | `1ef48ef87dbf` | planned | docs/ENGINEERING.md |
| `B-EXAMPLES` | 3755-3763 | `3d9b7dd4c5e3` | planned | docs/STATUS.md |
| `SECTION-26` | 3764-3766 | `9a88a697ae99` | structural | heading only |
| `CANON-21` | 3767-3800 | `afab1f61829e` | planned | docs/ENGINEERING.md |
| `required-regression-layers` | 3801-3810 | `6098f4656540` | planned | docs/ENGINEERING.md |
| `SECTION-27` | 3811-3813 | `62cc5be85e73` | structural | heading only |
| `release-validation-policy` | 3814-3832 | `c2bd42d65889` | planned | docs/RELEASE.md |
| `DEV-2~2` | 3833-3869 | `6ff9fac762f9` | duplicate | `unit:DEV-2` |
| `GROUP-future~2` | 3870-3878 | `c0c3ec88bb92` | duplicate | `unit:GROUP-future` |
| `SECTION-28` | 3879-3881 | `b97b11b56381` | structural | heading only |
| `documentation-generation-policy` | 3882-3894 | `1d33470c8cd5` | planned | docs/ENGINEERING.md |
| `SECTION-29` | 3895-3897 | `000dd38c7ed9` | structural | heading only |
| `DEV-1` | 3898-3935 | `48712f26b690` | planned | docs/RELEASE.md |
| `portability-targets` | 3936-3982 | `2422ef6e3b0a` | planned | docs/RELEASE.md |
| `SECTION-30` | 3983-3987 | `683f7b9b00c7` | planned | docs/STATUS.md |
| `B-WORKS~2` | 3988-4004 | `65db813fde18` | planned | docs/STATUS.md |
| `B-PARSED~2` | 4005-4010 | `010d10c5ad64` | planned | docs/STATUS.md |
| `B-GAPS~2` | 4011-4079 | `a5ea1b1eb65f` | planned | docs/STATUS.md |
| `B-CONFLICTS` | 4080-4087 | `fdc28764bc4a` | planned | docs/STATUS.md |
| `B-EXAMPLES~2` | 4088-4094 | `f85a9a4e4be5` | planned | docs/STATUS.md |
| `B-PROPOSALS` | 4095-4103 | `b5622600ecc3` | planned | openspec/changes (one change per open item) |
| `SECTION-31` | 4104-4106 | `0c7c92ea754c` | structural | heading only |
| `CANON-22` | 4107-4126 | `0fd628df7854` | planned | docs/ROADMAP.md |
| `GROUP-decisions` | 4127-4134 | `8b90cf0e6ca5` | planned | openspec/README.md (history) |
| `roadmap-to-1-0` | 4135-4149 | `de99ff09125d` | planned | docs/ROADMAP.md |
| `backend-strategy-directive` | 4150-4156 | `3e733152683f` | planned | docs/ENGINEERING.md |
| `backend-strategy-directive/core-principle` | 4157-4170 | `97d3972ccf1e` | planned | docs/ENGINEERING.md |
| `backend-strategy-directive/backend-maturation-sequence` | 4171-4186 | `ff2dbe76d57a` | planned | docs/ENGINEERING.md |
| `backend-strategy-directive/objective-criterion-for-leaving-clang` | 4187-4201 | `1cda38393426` | planned | docs/ENGINEERING.md |
| `backend-strategy-directive/scope-guard-for-backend-adjacent-work` | 4202-4208 | `859bab429005` | planned | docs/ENGINEERING.md |
| `compiler-module-architecture-directive` | 4209-4215 | `7dfc746e78b5` | planned | docs/ENGINEERING.md |
| `compiler-module-architecture-directive/principle-reactive-not-predictive` | 4216-4224 | `6bdbaef0d48c` | planned | docs/ENGINEERING.md |
| `compiler-module-architecture-directive/the-three-tests-for-extracting-a-module` | 4225-4240 | `9bdd4cb899e6` | planned | docs/ENGINEERING.md |
| `compiler-module-architecture-directive/anti-rules` | 4241-4249 | `c8700ead7c93` | planned | docs/ENGINEERING.md |
| `compiler-module-architecture-directive/working-rule-of-thumb` | 4250-4254 | `ec2c782d1c49` | planned | docs/ENGINEERING.md |
| `compiler-module-architecture-directive/timing` | 4255-4261 | `c4c0fef11f84` | planned | docs/ENGINEERING.md |
| `BANNER-end-of-inox-canonical-md-v3-8` | 4262-4267 | `784433b7be07` | planned | openspec/README.md (reading order) |
