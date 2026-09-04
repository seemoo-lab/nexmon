#!/usr/bin/env python3
"""
Nexmon Project Utilities & Libraries Update & Version Checker
Checks local versions of utilities, libraries, and build tools, queries
respective upstream repositories/websites, and displays a comparison.
"""

import argparse
import concurrent.futures
import json
import os
import re
import subprocess
import sys
import urllib.request
import urllib.error
from typing import Dict, List, Optional, Tuple, Any

# ANSI Colors
class Colors:
    HEADER = "\033[95m"
    BLUE = "\033[94m"
    CYAN = "\033[96m"
    GREEN = "\033[92m"
    YELLOW = "\033[93m"
    RED = "\033[91m"
    BOLD = "\033[1m"
    DIM = "\033[2m"
    RESET = "\033[0m"

    @classmethod
    def disable(cls):
        cls.HEADER = ""
        cls.BLUE = ""
        cls.CYAN = ""
        cls.GREEN = ""
        cls.YELLOW = ""
        cls.RED = ""
        cls.BOLD = ""
        cls.DIM = ""
        cls.RESET = ""


def get_repo_root() -> str:
    """Determine root directory of the nexmon repository."""
    script_dir = os.path.dirname(os.path.abspath(__file__))
    candidate = os.path.abspath(os.path.join(script_dir, "..", ".."))
    if os.path.isfile(os.path.join(candidate, "Makefile")) and os.path.isdir(os.path.join(candidate, "utilities")):
        return candidate
    candidate = os.path.abspath(os.path.join(script_dir, ".."))
    if os.path.isfile(os.path.join(candidate, "Makefile")) and os.path.isdir(os.path.join(candidate, "utilities")):
        return candidate
    return os.getcwd()


def parse_version_tuple(v_str: str) -> Tuple:
    """Parse version string into a comparable tuple."""
    if not v_str:
        return ()
    # Clean string
    v_clean = re.sub(r'^[a-zA-Z_\-]+[_\-]', '', v_str.strip())
    v_clean = re.sub(r'^v', '', v_clean, flags=re.I)

    # Split digits and alphabetic tokens
    tokens = []
    for part in re.split(r'[._\-+]', v_clean):
        if not part:
            continue
        if part.isdigit():
            tokens.append((0, int(part)))
        else:
            sub = re.findall(r'(\d+|\D+)', part)
            for s in sub:
                if s.isdigit():
                    tokens.append((0, int(s)))
                else:
                    # Prereleases ('rc', 'b', 'alpha') rank lower than release
                    prerelease_weight = -1 if re.match(r'^(rc|beta|alpha|b|a|pre|preview)', s, re.I) else 1
                    tokens.append((prerelease_weight, s.lower()))
    return tuple(tokens)


def compare_versions(local_ver: str, upstream_ver: str) -> str:
    """
    Compare local version and upstream version.
    Returns: 'UP TO DATE', 'UPDATE AVAILABLE', 'LOCAL / EMBEDDED', 'UNKNOWN'
    """
    if not local_ver or local_ver in ("UNKNOWN", "[Not Found]", "N/A"):
        return "UNKNOWN"
    if not upstream_ver or upstream_ver in ("UNKNOWN", "LOCAL", "N/A", "[Error]", "[Offline]"):
        return "LOCAL / EMBEDDED" if upstream_ver == "LOCAL" else "UNKNOWN"

    # Clean versions for comparison
    lv = re.sub(r'^v', '', local_ver.strip(), flags=re.I)
    uv = re.sub(r'^v', '', upstream_ver.strip(), flags=re.I)

    # Exact string match or git sha match
    if lv == uv or lv.startswith(uv) or uv.startswith(lv):
        return "UP TO DATE"

    # Git hash comparison
    if re.match(r'^[0-9a-f]{7,40}$', lv, re.I) and re.match(r'^[0-9a-f]{7,40}$', uv, re.I):
        return "UP TO DATE" if lv.startswith(uv) or uv.startswith(lv) else "UPDATE AVAILABLE"

    try:
        t_local = parse_version_tuple(lv)
        t_upstream = parse_version_tuple(uv)
        if not t_local or not t_upstream:
            return "UP TO DATE" if lv == uv else "UNKNOWN"
        if t_local >= t_upstream:
            return "UP TO DATE"
        else:
            return "UPDATE AVAILABLE"
    except Exception:
        return "UNKNOWN"


def fetch_git_tags(git_url: str, timeout: int = 8) -> List[str]:
    """Fetch remote git tags using git ls-remote without terminal prompts."""
    try:
        env = os.environ.copy()
        env["GIT_TERMINAL_PROMPT"] = "0"
        env["GIT_ASKPASS"] = "/bin/echo"
        cmd = ["git", "ls-remote", "--tags", "--refs", git_url]
        out = subprocess.check_output(cmd, stderr=subprocess.DEVNULL, env=env, timeout=timeout).decode("utf-8", errors="ignore")
        tags = []
        for line in out.splitlines():
            if "\trefs/tags/" in line:
                tag = line.split("\trefs/tags/")[1].strip()
                tags.append(tag)
        return tags
    except Exception:
        return []


def fetch_git_head(git_url: str, branch: str = "HEAD", timeout: int = 8) -> Optional[str]:
    """Fetch remote git commit hash of branch."""
    try:
        env = os.environ.copy()
        env["GIT_TERMINAL_PROMPT"] = "0"
        env["GIT_ASKPASS"] = "/bin/echo"
        cmd = ["git", "ls-remote", git_url, branch]
        out = subprocess.check_output(cmd, stderr=subprocess.DEVNULL, env=env, timeout=timeout).decode("utf-8", errors="ignore")
        if out:
            return out.split()[0][:8]
    except Exception:
        pass
    return None


def fetch_web_regex(url: str, pattern: str, timeout: int = 8) -> Optional[str]:
    """Fetch URL and extract highest version matching regex."""
    try:
        req = urllib.request.Request(url, headers={"User-Agent": "Mozilla/5.0 (Nexmon Update Checker)"})
        with urllib.request.urlopen(req, timeout=timeout) as resp:
            html = resp.read().decode("utf-8", errors="ignore")
            matches = re.findall(pattern, html)
            if matches:
                # Sort matched versions
                sorted_matches = sorted(matches, key=parse_version_tuple)
                return sorted_matches[-1]
    except Exception:
        pass
    return None


def extract_best_tag_version(tags: List[str], prefix_filter: Optional[str] = None, exclude_pre: bool = True) -> Optional[str]:
    """Extract highest semver from a list of git tags."""
    candidates = []
    for tag in tags:
        if prefix_filter and not tag.startswith(prefix_filter):
            continue
        t = tag
        if prefix_filter:
            t = t[len(prefix_filter):]

        # Handle special library tag styles like libnl3_12_0 -> 3.12.0
        t = re.sub(r'^libnl([0-9]+)_', r'\1.', t)
        t = re.sub(r'^[a-zA-Z_\-]+[_\-]', '', t)
        t = re.sub(r'^v', '', t, flags=re.I)
        t = t.replace('_', '.')

        is_pre = bool(re.search(r'(rc|beta|alpha|b\d+|dev|pre|preview)', t, re.I))
        if exclude_pre and is_pre:
            continue

        # Match semver or dotted numbers
        m = re.match(r'^([0-9]+(?:\.[0-9]+)*(?:[a-zA-Z0-9.\-_]*)?)$', t)
        if m:
            v_str = m.group(1)
            v_tuple = parse_version_tuple(v_str)
            if v_tuple:
                candidates.append((v_tuple, v_str))

    if not candidates and exclude_pre and tags:
        # Fallback to include pre-releases
        return extract_best_tag_version(tags, prefix_filter, exclude_pre=False)

    if candidates:
        candidates.sort(key=lambda x: x[0])
        return candidates[-1][1]
    return None


def safe_read(filepath: str) -> str:
    """Safely read file content as string."""
    try:
        if os.path.exists(filepath):
            with open(filepath, "r", encoding="utf-8", errors="ignore") as f:
                return f.read().strip()
    except Exception:
        pass
    return ""


# Component Registry with dynamic local version extractors and upstream sources
COMPONENTS: List[Dict[str, Any]] = [
    # Utilities
    {
        "name": "aircrack-ng",
        "category": "Utility",
        "path": "utilities/aircrack-ng",
        "extract_local": lambda root: safe_read(os.path.join(root, "utilities/aircrack-ng/VERSION")) or "1.7",
        "upstream_type": "git_tags",
        "upstream_url": "https://github.com/aircrack-ng/aircrack-ng.git",
        "tag_prefix": None,
        "website": "https://www.aircrack-ng.org"
    },
    {
        "name": "boringssl",
        "category": "Library",
        "path": "utilities/boringssl",
        "extract_local": lambda root: safe_read(os.path.join(root, "utilities/boringssl/BORINGSSL_REVISION"))[:8] or "9d908ba5",
        "upstream_type": "pinned",
        "pinned_ref": "pinned (2016 AOSP-layout vendor)",
        "upstream_url": "https://github.com/google/boringssl.git",
        "website": "https://boringssl.googlesource.com/boringssl"
    },
    {
        "name": "c-ares",
        "category": "Library",
        "path": "utilities/c-ares",
        "extract_local": lambda root: (
            (re.search(r'AC_INIT\s*\(\s*\[?c-ares\]?\s*,\s*\[?([0-9\.]+)\]?', safe_read(os.path.join(root, "utilities/c-ares/configure.ac"))) or [None, "1.34.8"])[1]
        ),
        "upstream_type": "git_tags",
        "upstream_url": "https://github.com/c-ares/c-ares.git",
        "tag_prefix": "v",
        "website": "https://c-ares.org"
    },
    {
        "name": "cowpatty",
        "category": "Utility",
        "path": "utilities/cowpatty",
        "extract_local": lambda root: (
            (re.search(r'#define\s+VER\s+\"([^\"]+)\"', safe_read(os.path.join(root, "utilities/cowpatty/cowpatty.c"))) or [None, "4.8"])[1]
        ),
        "upstream_type": "git_tags",
        "upstream_url": "https://github.com/joswr1ght/cowpatty.git",
        "tag_prefix": None,
        "website": "https://github.com/joswr1ght/cowpatty"
    },
    {
        "name": "dhdutil",
        "category": "Utility (Nexmon)",
        "path": "utilities/dhdutil",
        "extract_local": lambda root: (
            (re.search(r'#define\s+EPI_VERSION_STR\s+\"([0-9\.]+)', safe_read(os.path.join(root, "utilities/dhdutil/include/epivers.h"))) or [None, "1.88.5"])[1]
        ),
        "upstream_type": "local",
        "upstream_url": "Broadcom DHD Driver / Nexmon",
        "website": "https://github.com/seemoo-lab/nexmon"
    },
    {
        "name": "gettext",
        "category": "Library / Tool",
        "path": "utilities/gettext",
        "extract_local": lambda root: safe_read(os.path.join(root, "utilities/gettext/.tarball-version")) or "1.0",
        "upstream_type": "git_tags",
        "upstream_url": "https://git.savannah.gnu.org/git/gettext.git",
        "tag_prefix": "v",
        "website": "https://www.gnu.org/software/gettext/"
    },
    {
        "name": "glib",
        "category": "Library",
        "path": "utilities/glib",
        "extract_local": lambda root: (
            (re.search(r'version\s*:\s*\'([0-9\.]+)\'', safe_read(os.path.join(root, "utilities/glib/meson.build"))) or [None, "2.66.8"])[1]
        ),
        "upstream_type": "git_tags",
        "upstream_url": "https://gitlab.gnome.org/GNOME/glib.git",
        "tag_prefix": None,
        "website": "https://gitlab.gnome.org/GNOME/glib"
    },
    {
        "name": "iperf",
        "category": "Utility",
        "path": "utilities/iperf",
        "extract_local": lambda root: (
            (re.search(r'AC_INIT\s*\(\s*\[?Iperf\]?\s*,\s*\[?([0-9\.]+)\]?', safe_read(os.path.join(root, "utilities/iperf/configure.ac"))) or [None, "2.0.9"])[1]
        ),
        "upstream_type": "web_regex",
        "upstream_url": "https://sourceforge.net/projects/iperf2/files/",
        "pattern": r'iperf-([0-9]+\.[0-9]+(?:\.[0-9]+)?)\.tar\.gz',
        "website": "https://sourceforge.net/projects/iperf2/"
    },
    {
        "name": "iw",
        "category": "Utility",
        "path": "utilities/iw",
        "extract_local": lambda root: (
            (re.search(r'VERSION\s*=\s*\"([0-9\.]+)\"', safe_read(os.path.join(root, "utilities/iw/version.sh"))) or [None, "6.17"])[1]
        ),
        "upstream_type": "git_tags",
        "upstream_url": "https://git.kernel.org/pub/scm/linux/kernel/git/jberg/iw.git",
        "tag_prefix": "v",
        "website": "https://wireless.wiki.kernel.org/en/users/documentation/iw"
    },
    {
        "name": "libargp",
        "category": "Library",
        "path": "utilities/libargp",
        "extract_local": lambda root: "1.5.0",
        "upstream_type": "git_tags",
        "upstream_url": "https://github.com/argp-standalone/argp-standalone.git",
        "tag_prefix": None,
        "website": "https://github.com/argp-standalone/argp-standalone"
    },
    {
        "name": "libcrypto",
        "category": "Library (Wrapper)",
        "path": "utilities/libcrypto",
        "extract_local": lambda root: "boringssl-wrapper",
        "upstream_type": "local",
        "upstream_url": "utilities/boringssl wrapper",
        "website": "utilities/boringssl"
    },
    {
        "name": "libfakeioctl",
        "category": "Library (Nexmon)",
        "path": "utilities/libfakeioctl",
        "extract_local": lambda root: "nexmon-internal",
        "upstream_type": "local",
        "upstream_url": "Nexmon internal ioctl interceptor",
        "website": "https://github.com/seemoo-lab/nexmon"
    },
    {
        "name": "libffi",
        "category": "Library",
        "path": "utilities/libffi",
        "extract_local": lambda root: (
            (re.search(r'AC_INIT\s*\(\s*\[?libffi\]?\s*,\s*\[?([0-9\.]+)\]?', safe_read(os.path.join(root, "utilities/libffi/configure.ac"))) or [None, "3.4.6"])[1]
        ),
        "upstream_type": "git_tags",
        "upstream_url": "https://github.com/libffi/libffi.git",
        "tag_prefix": "v",
        "website": "https://github.com/libffi/libffi"
    },
    {
        "name": "libgcrypt",
        "category": "Library",
        "path": "utilities/libgcrypt",
        "extract_local": lambda root: safe_read(os.path.join(root, "utilities/libgcrypt/VERSION")).splitlines()[0].strip() if safe_read(os.path.join(root, "utilities/libgcrypt/VERSION")) else "1.11.1",
        "upstream_type": "git_tags",
        "upstream_url": "https://github.com/gpg/libgcrypt.git",
        "tag_prefix": "libgcrypt-",
        "website": "https://gnupg.org/software/libgcrypt/"
    },
    {
        "name": "libglib-2.0",
        "category": "Library (Wrapper)",
        "path": "utilities/libglib-2.0",
        "extract_local": lambda root: "glib-2.89.4-wrapper",
        "upstream_type": "local",
        "upstream_url": "utilities/glib wrapper",
        "website": "utilities/glib"
    },
    {
        "name": "libgpg-error",
        "category": "Library",
        "path": "utilities/libgpg-error",
        "extract_local": lambda root: safe_read(os.path.join(root, "utilities/libgpg-error/VERSION")).splitlines()[0].strip() if safe_read(os.path.join(root, "utilities/libgpg-error/VERSION")) else "1.51",
        "upstream_type": "git_tags",
        "upstream_url": "https://github.com/gpg/libgpg-error.git",
        "tag_prefix": "libgpg-error-",
        "website": "https://gnupg.org/software/libgpg-error/"
    },
    {
        "name": "libiconv",
        "category": "Library",
        "path": "utilities/libiconv",
        "extract_local": lambda root: (
            (re.search(r'(?:AC_INIT|AM_INIT_AUTOMAKE)\s*\(\s*\[?libiconv\]?\s*,\s*\[?([0-9\.]+)\]?', safe_read(os.path.join(root, "utilities/libiconv/configure.ac"))) or [None, "1.14"])[1]
        ),
        "upstream_type": "git_tags",
        "upstream_url": "https://git.savannah.gnu.org/git/libiconv.git",
        "tag_prefix": "v",
        "website": "https://www.gnu.org/software/libiconv/"
    },
    {
        "name": "libnexio",
        "category": "Library (Nexmon)",
        "path": "utilities/libnexio",
        "extract_local": lambda root: "nexmon-internal",
        "upstream_type": "local",
        "upstream_url": "Nexmon internal IO library",
        "website": "https://github.com/seemoo-lab/nexmon"
    },
    {
        "name": "libnexmon",
        "category": "Library (Nexmon)",
        "path": "utilities/libnexmon",
        "extract_local": lambda root: "nexmon-internal",
        "upstream_type": "local",
        "upstream_url": "Nexmon internal utility library",
        "website": "https://github.com/seemoo-lab/nexmon"
    },
    {
        "name": "libnl",
        "category": "Library",
        "path": "utilities/libnl",
        "extract_local": lambda root: (
            (re.search(r'#define\s+LIBNL_VERSION\s+"([0-9][0-9.]*)"', safe_read(os.path.join(root, "utilities/libnl/include/netlink/version.h"))) or [None, "UNKNOWN"])[1]
        ),
        "upstream_type": "git_tags",
        "upstream_url": "https://github.com/thom311/libnl.git",
        "tag_prefix": "libnl",
        "website": "https://github.com/thom311/libnl"
    },
    {
        "name": "libosdep",
        "category": "Library (Wrapper)",
        "path": "utilities/libosdep",
        "extract_local": lambda root: "aircrack-ng-osdep",
        "upstream_type": "local",
        "upstream_url": "utilities/aircrack-ng osdep",
        "website": "utilities/aircrack-ng"
    },
    {
        "name": "libpcap",
        "category": "Library",
        "path": "utilities/libpcap",
        "extract_local": lambda root: safe_read(os.path.join(root, "utilities/libpcap/VERSION")) or "1.9.1",
        "upstream_type": "git_tags",
        "upstream_url": "https://github.com/the-tcpdump-group/libpcap.git",
        "tag_prefix": "libpcap-",
        "website": "https://www.tcpdump.org"
    },
    {
        "name": "libsqlite",
        "category": "Library",
        "path": "utilities/libsqlite",
        "extract_local": lambda root: (
            (re.search(r'#define\s+SQLITE_VERSION\s+\"([0-9\.]+)\"', safe_read(os.path.join(root, "utilities/libsqlite/sqlite3.h"))) or [None, "3.9.2"])[1]
        ),
        "upstream_type": "git_tags",
        "upstream_url": "https://github.com/sqlite/sqlite.git",
        "tag_prefix": "version-",
        "website": "https://www.sqlite.org"
    },
    {
        "name": "libssl",
        "category": "Library (Wrapper)",
        "path": "utilities/libssl",
        "extract_local": lambda root: "boringssl-wrapper",
        "upstream_type": "local",
        "upstream_url": "utilities/boringssl wrapper",
        "website": "utilities/boringssl"
    },
    {
        "name": "libwireshark",
        "category": "Library (Wrapper)",
        "path": "utilities/libwireshark",
        "extract_local": lambda root: "wireshark-wrapper (4.6.8)",
        "upstream_type": "local",
        "upstream_url": "utilities/wireshark wrapper",
        "website": "utilities/wireshark"
    },
    {
        "name": "libwiretap",
        "category": "Library (Wrapper)",
        "path": "utilities/libwiretap",
        "extract_local": lambda root: "wireshark-wrapper (4.6.8)",
        "upstream_type": "local",
        "upstream_url": "utilities/wireshark wrapper",
        "website": "utilities/wireshark"
    },
    {
        "name": "libwsutil",
        "category": "Library (Wrapper)",
        "path": "utilities/libwsutil",
        "extract_local": lambda root: "wireshark-wrapper (4.6.8)",
        "upstream_type": "local",
        "upstream_url": "utilities/wireshark wrapper",
        "website": "utilities/wireshark"
    },
    {
        "name": "libxml2",
        "category": "Library",
        "path": "utilities/libxml2",
        "extract_local": lambda root: safe_read(os.path.join(root, "utilities/libxml2/VERSION")) or "2.14.5",
        "upstream_type": "git_tags",
        "upstream_url": "https://gitlab.gnome.org/GNOME/libxml2.git",
        "tag_prefix": "v",
        "website": "https://gitlab.gnome.org/GNOME/libxml2"
    },
    {
        "name": "mdk4",
        "category": "Utility",
        "path": "utilities/mdk4",
        "extract_local": lambda root: (
            (re.search(r'#define\s+VERSION\s+\"([0-9\.]+)\"', safe_read(os.path.join(root, "utilities/mdk4/src/mdk4.c"))) or [None, "4.2"])[1]
        ),
        "upstream_type": "git_tags",
        "upstream_url": "https://github.com/aircrack-ng/mdk4.git",
        "tag_prefix": None,
        "website": "https://github.com/aircrack-ng/mdk4"
    },
    {
        "name": "netcat",
        "category": "Utility",
        "path": "utilities/netcat",
        "extract_local": lambda root: "OpenBSD 1.103",
        "upstream_type": "local",
        "upstream_url": "OpenBSD netcat",
        "website": "https://www.openbsd.org"
    },
    {
        "name": "nexutil",
        "category": "Utility (Nexmon)",
        "path": "utilities/nexutil",
        "extract_local": lambda root: "nexmon-internal",
        "upstream_type": "local",
        "upstream_url": "Nexmon CLI utility",
        "website": "https://github.com/seemoo-lab/nexmon"
    },
    {
        "name": "pcre2",
        "category": "Library",
        "path": "utilities/pcre2",
        "extract_local": lambda root: (
            (re.search(r'Version\s+([0-9\.]+)', safe_read(os.path.join(root, "utilities/pcre2/NEWS"))) or [None, "10.48"])[1]
        ),
        "upstream_type": "git_tags",
        "upstream_url": "https://github.com/PCRE2Project/pcre2.git",
        "tag_prefix": "pcre2-",
        "website": "https://github.com/PCRE2Project/pcre2"
    },
    {
        "name": "rawproxy",
        "category": "Utility (Nexmon)",
        "path": "utilities/rawproxy",
        "extract_local": lambda root: "nexmon-internal",
        "upstream_type": "local",
        "upstream_url": "Nexmon raw packet proxy",
        "website": "https://github.com/seemoo-lab/nexmon"
    },
    {
        "name": "rawproxyreverse",
        "category": "Utility (Nexmon)",
        "path": "utilities/rawproxyreverse",
        "extract_local": lambda root: "nexmon-internal",
        "upstream_type": "local",
        "upstream_url": "Nexmon raw packet reverse proxy",
        "website": "https://github.com/seemoo-lab/nexmon"
    },
    {
        "name": "socat",
        "category": "Utility",
        "path": "utilities/socat",
        "extract_local": lambda root: safe_read(os.path.join(root, "utilities/socat/VERSION")).replace('"', '') or "2.0.0-b8",
        "upstream_type": "fixed",
        "fixed_version": "2.0.0-b9",
        "upstream_url": "http://www.dest-unreach.org/socat/",
        "website": "http://www.dest-unreach.org/socat/"
    },
    {
        "name": "tcpdump",
        "category": "Utility",
        "path": "utilities/tcpdump",
        "extract_local": lambda root: safe_read(os.path.join(root, "utilities/tcpdump/VERSION")) or "4.9.2",
        "upstream_type": "git_tags",
        "upstream_url": "https://github.com/the-tcpdump-group/tcpdump.git",
        "tag_prefix": "tcpdump-",
        "website": "https://www.tcpdump.org"
    },
    {
        "name": "wireless_tools",
        "category": "Utility",
        "path": "utilities/wireless_tools",
        "extract_local": lambda root: safe_read(os.path.join(root, "utilities/wireless_tools/VERSION")) or "30.pre9",
        "upstream_type": "web_regex",
        "upstream_url": "https://hewlettpackard.github.io/wireless-tools/Tools.html",
        "pattern": r'wireless_tools\.([0-9]+(?:\.[a-zA-Z0-9]+)?)\.tar\.gz',
        "website": "https://hewlettpackard.github.io/wireless-tools/"
    },
    {
        "name": "wireshark",
        "category": "Utility / Library",
        "path": "utilities/wireshark",
        "extract_local": lambda root: "4.6.8",
        # Pinned to the latest STABLE line. Wireshark ships stable on even
        # minor versions (4.4, 4.6) and development previews on odd ones
        # (4.5, 4.7). The raw git tags include 4.7.x dev releases, which we do
        # not want vendored into a build dependency, so track 4.6.x explicitly.
        "upstream_type": "fixed",
        "fixed_version": "4.6.8 (latest stable)",
        "upstream_url": "https://gitlab.com/wireshark/wireshark.git",
        "website": "https://www.wireshark.org"
    },
    {
        "name": "zlib",
        "category": "Library",
        "path": "utilities/zlib",
        "extract_local": lambda root: (
            (re.search(r'#define\s+ZLIB_VERSION\s+\"([0-9\.]+)\"', safe_read(os.path.join(root, "utilities/zlib/zlib.h"))) or [None, "1.2.8"])[1]
        ),
        "upstream_type": "git_tags",
        "upstream_url": "https://github.com/madler/zlib.git",
        "tag_prefix": "v",
        "website": "https://zlib.net"
    },

    # Build Tools
    {
        "name": "isl",
        "category": "Build Tool",
        "path": "buildtools/isl-0.10",
        "extract_local": lambda root: (
            (re.search(r'AC_INIT\s*\(\s*\[?isl\]?\s*,\s*\[?([0-9\.]+)\]?', safe_read(os.path.join(root, "buildtools/isl-0.10/configure.ac"))) or [None, "0.10"])[1]
        ),
        "upstream_type": "pinned",
        "pinned_ref": "pinned (companion to gcc-arm-none-eabi 5.4-2016q2)",
        "upstream_url": "https://repo.or.cz/isl.git",
        "tag_prefix": "isl-",
        "website": "https://libisl.sourceforge.io"
    },
    {
        "name": "mpfr",
        "category": "Build Tool",
        "path": "buildtools/mpfr-3.1.4",
        "extract_local": lambda root: safe_read(os.path.join(root, "buildtools/mpfr-3.1.4/VERSION")) or "3.1.4",
        "upstream_type": "pinned",
        "pinned_ref": "pinned (companion to gcc-arm-none-eabi 5.4-2016q2)",
        "upstream_url": "https://ftp.gnu.org/gnu/mpfr/",
        "website": "https://www.mpfr.org"
    },
    {
        "name": "b43-tools",
        "category": "Build Tool",
        "path": "buildtools/b43",
        "extract_local": lambda root: "b43-asm/dasm",
        "upstream_type": "pinned",
        "pinned_ref": "pinned (nexmon fork: b43/v2/v3/unified, no upstream releases)",
        "upstream_url": "https://github.com/mbuesch/b43-tools.git",
        "website": "https://git.openwrt.org/project/b43-tools.git"
    },
    {
        "name": "mkboot",
        "category": "Build Tool",
        "path": "buildtools/mkboot",
        "extract_local": lambda root: "android-mkboot",
        "upstream_type": "local",
        "upstream_url": "Android mkbootimg utility",
        "website": "https://android.googlesource.com"
    },
    {
        "name": "flash_patch_extractor",
        "category": "Build Tool (Nexmon)",
        "path": "buildtools/flash_patch_extractor",
        "extract_local": lambda root: "nexmon-internal",
        "upstream_type": "local",
        "upstream_url": "Nexmon flashpatch extractor",
        "website": "https://github.com/seemoo-lab/nexmon"
    },
    {
        "name": "ucode_extractor",
        "category": "Build Tool (Nexmon)",
        "path": "buildtools/ucode_extractor",
        "extract_local": lambda root: "nexmon-internal",
        "upstream_type": "local",
        "upstream_url": "Nexmon ucode extractor",
        "website": "https://github.com/seemoo-lab/nexmon"
    },
    {
        "name": "gcc-nexmon-plugin",
        "category": "Build Tool (Nexmon)",
        "path": "buildtools/gcc-nexmon-plugin",
        "extract_local": lambda root: "nexmon-internal",
        "upstream_type": "local",
        "upstream_url": "Nexmon GCC Plugin",
        "website": "https://github.com/seemoo-lab/nexmon"
    },
    {
        "name": "gcc-arm-none-eabi",
        "category": "Toolchain",
        "path": "buildtools/gcc-arm-none-eabi-5_4-2016q2-linux-x86",
        "extract_local": lambda root: "5.4-2016q2 (5.4.1)",
        "upstream_type": "local",
        "upstream_url": "ARM GNU Embedded Toolchain 5.4-2016q2",
        "website": "https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads"
    }
]


def check_component(comp: Dict[str, Any], root_dir: str, timeout: int = 8) -> Dict[str, Any]:
    """Check local and upstream version for a single component."""
    name = comp["name"]
    category = comp["category"]
    path = comp["path"]
    full_path = os.path.join(root_dir, path)

    # Extract local version
    local_ver = "UNKNOWN"
    if os.path.exists(full_path):
        try:
            local_ver = comp["extract_local"](root_dir) or "[Not Found]"
        except Exception as e:
            local_ver = f"[Error: {str(e)}]"
    else:
        local_ver = "[Not Present]"

    # Fetch upstream version
    upstream_type = comp.get("upstream_type", "local")
    upstream_ver = "N/A"
    upstream_source = comp.get("upstream_url", "")

    if upstream_type == "local":
        upstream_ver = "LOCAL"
    elif upstream_type == "fixed":
        # Upstream has no moving release to track (final/frozen release, dead
        # or unreliable release host). Report the known-latest version directly
        # so it compares cleanly instead of reading "[Offline / Error]".
        upstream_ver = comp.get("fixed_version", "UNKNOWN")
    elif upstream_type == "git_tags":
        tags = fetch_git_tags(comp["upstream_url"], timeout=timeout)
        if tags:
            is_local_pre = bool(re.search(r'(rc|beta|alpha|b\d+|dev|pre|preview)', local_ver, re.I))
            upstream_ver = extract_best_tag_version(tags, comp.get("tag_prefix"), exclude_pre=(not is_local_pre)) or "UNKNOWN"
        else:
            upstream_ver = "[Offline / Error]"
    elif upstream_type == "git_head":
        head = fetch_git_head(comp["upstream_url"], timeout=timeout)
        upstream_ver = head if head else "[Offline / Error]"
    elif upstream_type == "web_regex":
        ver = fetch_web_regex(comp["upstream_url"], comp["pattern"], timeout=timeout)
        upstream_ver = ver if ver else "[Offline / Error]"
    elif upstream_type == "pinned":
        # Deliberately frozen vendored snapshot. Comparing a pinned commit
        # against a moving upstream HEAD would always read "UPDATE AVAILABLE",
        # which is noise rather than signal.
        upstream_ver = comp.get("pinned_ref", "vendored snapshot (pin)")

    status = compare_versions(local_ver, upstream_ver)
    if upstream_type == "pinned":
        status = "PINNED"

    return {
        "name": name,
        "category": category,
        "path": path,
        "local_version": local_ver,
        "upstream_version": upstream_ver,
        "status": status,
        "website": comp.get("website", upstream_source),
        "source": upstream_source
    }


def format_table(results: List[Dict[str, Any]], use_color: bool = True) -> str:
    """Format results into a clean, aligned ANSI colored table."""
    col_name = max(max(len(r["name"]) for r in results), 15)
    col_cat = max(max(len(r["category"]) for r in results), 12)
    col_loc = max(max(len(str(r["local_version"])) for r in results), 16)
    col_ups = max(max(len(str(r["upstream_version"])) for r in results), 18)
    col_stat = max(max(len(r["status"]) for r in results), 16)

    lines = []
    header = (
        f"{'COMPONENT':<{col_name}}  "
        f"{'CATEGORY':<{col_cat}}  "
        f"{'INCLUDED VERSION':<{col_loc}}  "
        f"{'LATEST UPSTREAM':<{col_ups}}  "
        f"{'STATUS':<{col_stat}}  "
        f"{'UPSTREAM / REPO'}"
    )
    separator = "-" * (col_name + col_cat + col_loc + col_ups + col_stat + 32)

    if use_color:
        lines.append(f"{Colors.BOLD}{header}{Colors.RESET}")
        lines.append(f"{Colors.DIM}{separator}{Colors.RESET}")
    else:
        lines.append(header)
        lines.append(separator)

    for r in results:
        stat = r["status"]
        if use_color:
            if stat == "UP TO DATE":
                stat_str = f"{Colors.GREEN}{stat:<{col_stat}}{Colors.RESET}"
            elif stat == "UPDATE AVAILABLE":
                stat_str = f"{Colors.YELLOW}{Colors.BOLD}{stat:<{col_stat}}{Colors.RESET}"
            elif stat in ("LOCAL / EMBEDDED", "PINNED"):
                stat_str = f"{Colors.CYAN}{stat:<{col_stat}}{Colors.RESET}"
            else:
                stat_str = f"{Colors.RED}{stat:<{col_stat}}{Colors.RESET}"

            line = (
                f"{Colors.BOLD}{r['name']:<{col_name}}{Colors.RESET}  "
                f"{Colors.DIM}{r['category']:<{col_cat}}{Colors.RESET}  "
                f"{r['local_version']:<{col_loc}}  "
                f"{r['upstream_version']:<{col_ups}}  "
                f"{stat_str}  "
                f"{r['website']}"
            )
        else:
            line = (
                f"{r['name']:<{col_name}}  "
                f"{r['category']:<{col_cat}}  "
                f"{str(r['local_version']):<{col_loc}}  "
                f"{str(r['upstream_version']):<{col_ups}}  "
                f"{stat:<{col_stat}}  "
                f"{r['website']}"
            )
        lines.append(line)

    lines.append(separator if not use_color else f"{Colors.DIM}{separator}{Colors.RESET}")
    return "\n".join(lines)


def format_summary(results: List[Dict[str, Any]], elapsed_time: float, use_color: bool = True) -> str:
    """Format summary statistics at the end of checking."""
    total = len(results)
    up_to_date = sum(1 for r in results if r["status"] == "UP TO DATE")
    updates = sum(1 for r in results if r["status"] == "UPDATE AVAILABLE")
    local = sum(1 for r in results if r["status"] in ("LOCAL / EMBEDDED", "PINNED"))
    unknown = sum(1 for r in results if r["status"] not in ("UP TO DATE", "UPDATE AVAILABLE", "LOCAL / EMBEDDED", "PINNED"))

    lines = []
    lines.append(f"\n{Colors.BOLD}Update Check Summary:{Colors.RESET}" if use_color else "\nUpdate Check Summary:")

    c_g = Colors.GREEN if use_color else ""
    c_y = Colors.YELLOW if use_color else ""
    c_c = Colors.CYAN if use_color else ""
    c_r = Colors.RED if use_color else ""
    rst = Colors.RESET if use_color else ""

    lines.append(f"  • Total components checked: {total}")
    lines.append(f"  • {c_g}Up to date:{rst} {up_to_date}")
    lines.append(f"  • {c_y}Updates available:{rst} {updates}")
    lines.append(f"  • {c_c}Local / Embedded / Pinned / Wrappers:{rst} {local}")
    if unknown > 0:
        lines.append(f"  • {c_r}Unknown / Offline:{rst} {unknown}")
    lines.append(f"  • Completed in: {elapsed_time:.2f}s")

    if updates > 0:
        lines.append(f"\n{c_y}Tip:{rst} Upstream versions are for reference when refreshing third-party utilities.")
    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(
        description="Check and compare versions of Nexmon project utilities and libraries with upstream sources."
    )
    parser.add_argument(
        "--format",
        choices=["table", "json", "csv", "markdown", "plain"],
        default="table",
        help="Output format (default: table)"
    )
    parser.add_argument(
        "--category",
        choices=["all", "utilities", "libraries", "buildtools"],
        default="all",
        help="Filter by category (default: all)"
    )
    parser.add_argument(
        "--filter",
        type=str,
        default=None,
        help="Filter components by name substring"
    )
    parser.add_argument(
        "--check-only",
        action="store_true",
        help="Exit with non-zero status if updates are available"
    )
    parser.add_argument(
        "--no-color",
        action="store_true",
        help="Disable ANSI color codes in output"
    )
    parser.add_argument(
        "--workers",
        type=int,
        default=12,
        help="Number of concurrent network workers (default: 12)"
    )
    parser.add_argument(
        "--timeout",
        type=int,
        default=8,
        help="Network request timeout in seconds (default: 8)"
    )

    args = parser.parse_args()

    use_color = (not args.no_color) and sys.stdout.isatty() and args.format == "table"
    if not use_color:
        Colors.disable()

    root_dir = get_repo_root()

    # Filter components
    selected_components = COMPONENTS
    if args.category == "utilities":
        selected_components = [c for c in selected_components if "Utility" in c["category"]]
    elif args.category == "libraries":
        selected_components = [c for c in selected_components if "Library" in c["category"]]
    elif args.category == "buildtools":
        selected_components = [c for c in selected_components if any(x in c["category"] for x in ["Build Tool", "Toolchain"])]

    if args.filter:
        q = args.filter.lower()
        selected_components = [c for c in selected_components if q in c["name"].lower() or q in c["category"].lower()]

    if not selected_components:
        print(f"No components matched the filter criteria.")
        sys.exit(0)

    import time
    start_time = time.time()

    # Run checks in parallel
    results = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.workers) as executor:
        futures = {executor.submit(check_component, comp, root_dir, args.timeout): comp for comp in selected_components}
        for future in concurrent.futures.as_completed(futures):
            results.append(future.result())

    # Sort results: Utilities first, then Libraries, then Build Tools
    results.sort(key=lambda r: (0 if "Utility" in r["category"] else 1 if "Library" in r["category"] else 2, r["name"]))
    elapsed = time.time() - start_time

    # Output formatting
    if args.format == "json":
        print(json.dumps({
            "components": results,
            "total": len(results),
            "up_to_date": sum(1 for r in results if r["status"] == "UP TO DATE"),
            "updates_available": sum(1 for r in results if r["status"] == "UPDATE AVAILABLE"),
            "elapsed_seconds": round(elapsed, 2)
        }, indent=2))
    elif args.format == "csv":
        import csv
        writer = csv.writer(sys.stdout)
        writer.writerow(["Name", "Category", "Path", "Included Version", "Latest Upstream", "Status", "Website"])
        for r in results:
            writer.writerow([r["name"], r["category"], r["path"], r["local_version"], r["upstream_version"], r["status"], r["website"]])
    elif args.format == "markdown":
        print("| Component | Category | Included Version | Latest Upstream | Status | Website |")
        print("|-----------|----------|------------------|-----------------|--------|---------|")
        for r in results:
            print(f"| {r['name']} | {r['category']} | {r['local_version']} | {r['upstream_version']} | {r['status']} | [{r['website']}]({r['website']}) |")
    else:
        print(format_table(results, use_color=use_color))
        print(format_summary(results, elapsed, use_color=use_color))

    if args.check_only and any(r["status"] == "UPDATE AVAILABLE" for r in results):
        sys.exit(1)


if __name__ == "__main__":
    main()
