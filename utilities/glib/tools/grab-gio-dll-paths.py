#!/usr/bin/env python3
#
# Copyright © 2024 Chun-wei Fan.
#
# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Original author: Chun-wei Fan <fanc999@yahoo.com.tw>

"""
This script runs pkg-config against the uninstalled pkg-config files to obtain the
paths where the newly-built GLib and subproject DLLs reside
"""

import argparse
import os
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-path", required=True, help="Root build directory")
    parser.add_argument(
        "--pkg-config", help="pkg-config or compatible executable", default="pkg-config"
    )
    parser.add_argument(
        "--pkg-config-path",
        help="Additional paths to look for pkg-config .pc files",
    )
    args = parser.parse_args()
    build_path = args.build_path
    uninstalled_pc_path = os.path.join(build_path, "meson-uninstalled")
    if not os.path.isdir(uninstalled_pc_path):
        raise ValueError("%s is not a valid build path" % build_path)

    additional_pkg_config_paths = [uninstalled_pc_path]
    if args.pkg_config_path is not None:
        additional_pkg_config_paths += args.pkg_config_path.split(os.pathsep)

    if "PKG_CONFIG_PATH" in os.environ:
        additional_pkg_config_paths += os.environ["PKG_CONFIG_PATH"].split(os.pathsep)

    os.environ["PKG_CONFIG_PATH"] = os.pathsep.join(additional_pkg_config_paths)

    # The _giscanner Python module depends on GIO, so use gio-2.0-uninstalled.pc
    pkg_config_result = subprocess.run(
        [args.pkg_config, "--libs-only-L", "gio-2.0"],
        capture_output=True,
        text=True,
        check=True,
    )
    lib_args = pkg_config_result.stdout.split()
    libpaths = []
    for arg in lib_args:
        # Get rid of the "-L" prefix in the library paths
        if arg[2:] not in libpaths:
            libpaths.append(arg[2:])

    # Now append the paths in %PATH%, if the paths exist
    paths = os.environ["PATH"].split(os.pathsep)
    for path in paths:
        if os.path.isdir(path):
            libpaths.append(path)

    print(os.pathsep.join(libpaths).rstrip())


if __name__ == "__main__":
    main()
