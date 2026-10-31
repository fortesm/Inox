#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
# Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.
"""The version and the canon revision say the same thing everywhere.

`src/compiler/support/Version.h` defines the version (MAJOR.MINOR.RELEASE
and its stage) and the canon revision (constitution VERSIONING). This check
fails when a document that states them disagrees:

  docs/openspec/specs/constitution/revisions.md  "Language version: Inox X.Y.Z <stage>"
                                                  "Canon revision: rN"
  README.md                                       "version X.Y.Z, <stage>"
  CHANGELOG.md                                    a section "## X.Y.Z ..."
  docs/STATUS.md                                  "Inox X.Y.Z" in its verification line

Usage: python3 tools/version_check.py   (from the repository root)
Change `ci-gates` (foundation step F1, canon r3.111).
"""

import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent


def read(relative):
    return (ROOT / relative).read_text(encoding="utf-8")


def header_value(text, name):
    match = re.search(r"\b" + name + r"\s*=\s*\"?([^\";]+)\"?\s*;", text)
    if match is None:
        sys.exit("version_check: %s not found in Version.h" % name)
    return match.group(1).strip()


def main():
    header = read("src/compiler/support/Version.h")
    version = ".".join(header_value(header, "kVersion" + part) for part in ("Major", "Minor", "Release"))
    stage = header_value(header, "kVersionStage")
    canon = header_value(header, "kCanonRevision")

    problems = []

    def expect(document, pattern, what):
        if re.search(pattern, read(document), re.MULTILINE) is None:
            problems.append("%s: %s" % (document, what))

    escaped = re.escape(version)
    expect("docs/openspec/specs/constitution/revisions.md",
           r"^Language version: Inox " + escaped + " " + re.escape(stage) + r"$",
           "expected 'Language version: Inox %s %s'" % (version, stage))
    expect("docs/openspec/specs/constitution/revisions.md",
           r"^Canon revision: " + re.escape(canon) + r"$",
           "expected 'Canon revision: %s' (kCanonRevision)" % canon)
    expect("README.md", r"\bversion " + escaped + ", " + re.escape(stage) + r"\b",
           "expected 'version %s, %s'" % (version, stage))
    expect("CHANGELOG.md", r"^## " + escaped + r"\b", "expected a section '## %s'" % version)
    expect("docs/STATUS.md", r"\bInox " + escaped + r"\b", "expected 'Inox %s' in its verification line" % version)

    for problem in problems:
        print("version_check: " + problem)
    print("version_check: Inox %s %s, canon %s: %d problem(s)" % (version, stage, canon, len(problems)))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
