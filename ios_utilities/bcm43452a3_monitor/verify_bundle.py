# SPDX-License-Identifier: GPL-3.0-only
"""Build/check an allowlisted source and pinned own-code binary local contribution archive; no network I/O."""

import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
from urllib.parse import unquote
import zipfile


ROOT = Path(__file__).resolve().parent
ARCHIVE = "nexmon-public-repro-20261106.zip"
PREVIOUS_ARCHIVE = "nexmon-public-20261105.zip"
MANIFEST = "manifest.json"
FILES = ('DEPENDENCIES.md', 'INTEGRATION.md', 'LICENSE.txt', 'PROVENANCE.md', 'README.md', 'REPORT.md', 'REPRODUCING.md', 'build.py', 'build_native.py', 'capture.py', 'evidence/application-final.json', 'evidence/dopamine-source.json', 'evidence/final-runtime.json', 'evidence/handshake.json', 'evidence/native-build.json', 'evidence/offline-repair.json', 'evidence/service-fifteen-minute.json', 'evidence/service-five-minute.json', 'evidence/source-build.json', 'evidence/standalone-reproduction.json', 'evidence/storage.json', 'firmware.py', 'metadata/provenance.json', 'metadata/target.json', 'radio.py', 'raw_transport.py', 'requirements.txt', 'source/AuthTap.c', 'source/MemoryLauncher.c', 'source/MonitorRadio.m', 'source/RawMirror.c', 'source/eapol_mirror_capture_small.S', 'source/eapol_mirror_control.S', 'source/eapol_mirror_fetch_combined.c', 'source/monitor_control.c', 'test_reproduction.py', 'trust_native.py', 'verify_bundle.py', 'wifi_auth.py')

FILES += ('apply_prebuilt.py', 'test_prebuilt.py', 'prebuilt/README.md', 'prebuilt/firmware.patch.json', 'prebuilt/firmware.patch.bin')
BINARY_PINS = {'prebuilt/firmware.patch.bin': (1556, 'ca478c5cad710d7dc46028e966083d152aae1a4e8df089ffcec4cb2b8f6fc1c6')}

GENERATED = {MANIFEST, ARCHIVE, ARCHIVE + ".sha256",
             PREVIOUS_ARCHIVE, PREVIOUS_ARCHIVE + ".sha256"}


def digest(blob):
    return hashlib.sha256(blob).hexdigest()


def inspect_files(private_context=None):
    """Reject suspicious patterns; report rule/file only, never matched values."""
    actual = {p.relative_to(ROOT).as_posix() for p in ROOT.rglob("*") if p.is_file()}
    extra = actual - set(FILES) - GENERATED
    missing = set(FILES) - actual
    if extra or missing:
        raise ValueError("Allowlist mismatch: unexpected or missing files")
    rules = {
        "network_address": r"(?i)(?:[0-9a-f]{2}:){5}[0-9a-f]{2}",
        "private_device_path": r"/(?:var/mobile|var/jb|private/var|Users)/",
        "windows_absolute_path": r"(?i)[a-z]:[\\/](?:users|my|program)",
        "raw_handshake_record": r"(?i)WPA\*0[12]\*[0-9a-f]{32}\*",
        "private_key": r"-----BEGIN [A-Z ]*PRIVATE KEY-----",
        "session_uuid": r"(?i)\b[0-9a-f]{8}(?:-[0-9a-f]{4}){3}-[0-9a-f]{12}\b",
        "literal_auth_secret": r"(?i)[\"']?(?:password|nonce|mic)[\"']?\s*[:=]\s*[\"'][0-9a-f]{16,}[\"']",
        "non_english_cyrillic": r"[\u0400-\u04ff]",
        "instruction_byte_dump": r"(?im)^\s*(?:0x)?[0-9a-f]{6,16}:\s+(?:[0-9a-f]{2}\s+){4,}",
        "private_ipv4": r"\b(?:10\.|192\.168\.|172\.(?:1[6-9]|2[0-9]|3[01])\.)\d+\.\d+(?:\.\d+)?\b",
    }
    needles = set()
    if private_context:
        data = json.loads(Path(private_context).read_text(encoding="utf-8-sig"))
        for key, value in data.items():
            if key.lower() not in {"ssid", "bssid", "mac", "password", "host"}:
                continue
            if isinstance(value, str) and len(value) >= 4:
                needles.add(value.casefold())
                needles.add(value.encode().hex())
                if key.lower() in {"bssid", "mac"}:
                    needles.add(value.replace(":", "").replace("-", "").casefold())
    issues = []
    blobs = {}
    links_checked = 0
    for name in FILES:
        path = ROOT / name
        if path.is_symlink() or not path.resolve().is_relative_to(ROOT):
            raise ValueError("Unsafe member path")
        blob = path.read_bytes()
        if name in BINARY_PINS:
            size, checksum = BINARY_PINS[name]
            if len(blob) != size or digest(blob) != checksum:
                raise ValueError("Pinned own-code binary mismatch")
            if any(bytes.fromhex(needle) in blob for needle in needles if len(needle) % 2 == 0 and re.fullmatch("[0-9a-f]+", needle)):
                raise ValueError("Private context in binary")
            blobs[name] = blob
            continue
        text = blob.decode("utf-8")
        if b"\x00" in blob:
            raise ValueError("Binary content rejected")
        blobs[name] = blob
        for rule, pattern in rules.items():
            scan_text = text
            if rule == "private_device_path":
                # Fixed generic target paths only; no user/device/run path exemption.
                for generic in ("/var/jb/basebin/libjailbreak.dylib", "/var/jb/usr/sbin/ifconfig",
                                "/var/jb/sbin/ifconfig", "/var/jb/var/monitor-mode/radio.lock",
                                "/var/mobile/WiFiSurvey/data/status.json", "/var/jb/usr/bin/python3"):
                    scan_text = scan_text.replace(generic, "GENERIC_DOCUMENTED_TARGET_PATH")
            if name in ("source/RawMirror.c", "source/AuthTap.c") and rule == "network_address":
                # Explicit synthetic self-test fixtures only, never a broad address exemption.
                for raw_test in ("020000000001", "ffffffffffff"):
                    literal = ":".join(raw_test[i:i+2] for i in range(0,12,2))
                    scan_text = scan_text.replace(literal, "SYNTHETIC_SELF_TEST")
            if re.search(pattern, scan_text):
                issues.append((name, rule))
        if any(needle in text.casefold() for needle in needles):
            issues.append((name, "private_context_value"))
        if name.endswith(".json"):
            json.loads(text)
        if name.endswith(".md"):
            for target in re.findall(r"\[[^\]]*\]\(([^)]+)\)", text):
                if re.match(r"(?:https?|mailto):", target):
                    continue
                target = unquote(target.split("#", 1)[0])
                if not target:
                    continue
                resolved = (path.parent / target).resolve()
                if not resolved.is_relative_to(ROOT) or not resolved.is_file():
                    issues.append((name, "broken_or_external_local_link"))
                links_checked += 1
    if issues:
        for name, rule in issues:
            print("REJECT:", name, rule)
        raise ValueError("Privacy/content checks failed; matched values suppressed")
    return blobs, links_checked, len(needles)


def build(blobs):
    manifest = {"schema": 1, "scope": "Allowlisted sources and hash-pinned own-code binary patch",
                "archive": ARCHIVE, "manifest_self_hash_excluded": True,
                "files": [{"path": name, "bytes": len(blob), "sha256": digest(blob)}
                          for name, blob in sorted(blobs.items())]}
    manifest_blob = (json.dumps(manifest, indent=2) + "\n").encode()
    (ROOT / MANIFEST).write_bytes(manifest_blob)
    members = {**blobs, MANIFEST: manifest_blob}
    with zipfile.ZipFile(ROOT / ARCHIVE, "w", compression=zipfile.ZIP_DEFLATED) as archive:
        for name, blob in sorted(members.items()):
            info = zipfile.ZipInfo(name, date_time=(2026, 11, 6, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o100644 << 16
            archive.writestr(info, blob)
    checksum = digest((ROOT / ARCHIVE).read_bytes())
    (ROOT / (ARCHIVE + ".sha256")).write_text(checksum + "  " + ARCHIVE + "\n", encoding="ascii")


def verify(blobs):
    manifest_blob = (ROOT / MANIFEST).read_bytes()
    manifest = json.loads(manifest_blob)
    rows = manifest["files"]
    if len(rows) != len(FILES) or {row["path"] for row in rows} != set(FILES):
        raise ValueError("Manifest allowlist mismatch")
    for row in rows:
        blob = blobs[row["path"]]
        if row["bytes"] != len(blob) or row["sha256"] != digest(blob):
            raise ValueError("Manifest member integrity mismatch")
    archive_path = ROOT / ARCHIVE
    checksum_path = ROOT / (ARCHIVE + ".sha256")
    if archive_path.exists() != checksum_path.exists():
        raise ValueError("Archive/checksum pair incomplete")
    if archive_path.exists():
        expected = {**blobs, MANIFEST: manifest_blob}
        with zipfile.ZipFile(archive_path) as archive:
            if len(archive.namelist()) != len(expected) or set(archive.namelist()) != set(expected):
                raise ValueError("ZIP member allowlist mismatch")
            for name, blob in expected.items():
                if archive.read(name) != blob:
                    raise ValueError("ZIP member readback mismatch")
            if archive.testzip() is not None:
                raise ValueError("ZIP CRC failure")
        recorded = checksum_path.read_text(encoding="ascii").strip()
        if recorded != digest(archive_path.read_bytes()) + "  " + ARCHIVE:
            raise ValueError("ZIP checksum mismatch")
    return archive_path.exists()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", action="store_true", help="Regenerate this directory's manifest/ZIP")
    parser.add_argument("--private-context", help="Optional local JSON for silent exact-value exclusion checks")
    args = parser.parse_args()
    blobs, links, private_needles = inspect_files(args.private_context)
    pass  # No hardware or packet example executes during packaging.
    if args.build:
        build(blobs)
    archive_verified = verify(blobs)
    print("PASS:", len(FILES), "allowlisted members;", links,
          "local links; privacy-pattern checks;", private_needles,
          "private-context forms checked; manifest verified")
    if archive_verified:
        print("PASS: ZIP member readback, CRC and checksum verified")
        print("ZIP SHA256:", digest((ROOT / ARCHIVE).read_bytes()))
    else:
        print("Archive absent: checked local members and manifest only")


if __name__ == "__main__":
    try:
        main()
    except (ValueError, UnicodeError, OSError, json.JSONDecodeError, subprocess.CalledProcessError) as error:
        print("FAIL:", type(error).__name__, "(sensitive values suppressed)")
        sys.exit(1)
