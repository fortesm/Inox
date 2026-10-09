# SPDX-License-Identifier: MPL-2.0
# Copyright (c) 2026 Marcelo Fortes and Inox contributors. All rights reserved.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.
"""Compare a test-runner log with a list of known failures.

Usage: python tools/ci_known_failures.py RUNNER_LOG KNOWN_FAILURES_FILE

Exit status 1 when the log has no "Summary:" line (the runner did not finish)
or when a test fails that is not in the known list. A known failure that now
passes is reported as a GitHub Actions notice, not an error, so the list can be
shrunk deliberately.
"""

import re
import sys

FAIL = re.compile(r"^\[FAIL\]\s+(\S+?\.inox)\b")


def normalize(path):
    path = path.replace("\\", "/")
    return path[2:] if path.startswith("./") else path


def main(argv):
    if len(argv) != 3:
        print(__doc__)
        return 2
    with open(argv[1], encoding="utf-8", errors="replace") as stream:
        log = stream.read().splitlines()
    with open(argv[2], encoding="utf-8") as stream:
        known = {normalize(line.strip()) for line in stream
                 if line.strip() and not line.lstrip().startswith("#")}

    if not any(line.startswith("Summary:") for line in log):
        print("::error::the test runner did not print a Summary line")
        return 1

    failed = set()
    for line in log:
        match = FAIL.match(line)
        if match:
            failed.add(normalize(match.group(1)))

    unexpected = sorted(failed - known)
    fixed = sorted(known - failed)
    for path in fixed:
        print("::notice::known failure now passes; remove it from the list: " + path)
    for path in unexpected:
        print("::error::unexpected failure: " + path)
    print("failures: %d known, %d unexpected; %d known entries now pass"
          % (len(failed & known), len(unexpected), len(fixed)))
    return 1 if unexpected else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
