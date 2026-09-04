#!/usr/bin/env python3
#
# Copyright © 2022-2024 Collabora, Ltd.
#
# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Original author: Xavier Claessens

"""
This script checks Meson configuration logs to verify no installed file is
missing installation tag.
"""

import argparse
import json
from pathlib import Path


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("builddir", type=Path, nargs="?", default=".")
    args = parser.parse_args()

    print("# TAP version 13")

    count = 0
    bad = 0
    path = args.builddir / "meson-info" / "intro-install_plan.json"
    with path.open(encoding="utf-8") as f:
        install_plan = json.load(f)
        for target in install_plan.values():
            for info in target.values():
                count += 1

                if not info["tag"]:
                    bad += 1
                    dest = info["destination"]
                    print(f"not ok {bad} - Missing install_tag for {dest}")

    if bad == 0:
        print(f"ok 1 - All {count} installed files have install_tag")
        print("1..1")
        return 0
    else:
        print(f"# {bad}/{count} installed files do not have install_tag")
        print(f"1..{bad}")
        return 1


if __name__ == "__main__":
    exit(main())
