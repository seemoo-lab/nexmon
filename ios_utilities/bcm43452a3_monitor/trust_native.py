# SPDX-License-Identifier: GPL-3.0-only
"""Trust only five root-owned, manifest-verified project helpers using pinned Dopamine.

Run as root on the matching jailbreak after building. No firmware or radio
commands. This bootstraps MemoryLauncher before its first execution.
"""
import argparse
import ctypes
import hashlib
import json
import os
from pathlib import Path

LIBRARY=Path('/var/jb/basebin/libjailbreak.dylib')
LIBRARY_SHA='ea76a9d9b6af0a50c0aa2ace1f770ad5cfcfff89c4555b2c60dc4b7d1b4427be'


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--tools',type=Path,required=True)
    args=parser.parse_args()
    if os.getuid()!=0:raise SystemExit('Root is required')
    directory=args.tools.resolve()
    if directory.stat().st_uid!=0 or directory.stat().st_mode&0o022:raise SystemExit('Untrusted helper directory')
    manifest=json.loads((directory/'native-manifest.json').read_text())
    names={'raw_mirror','monitor_control','auth_tap','monitor_radio','MemoryLauncher'}
    if len(manifest['files'])!=5 or {i['name'] for i in manifest['files']}!=names:raise SystemExit('Unexpected helper inventory')
    paths=[]
    for item in manifest['files']:
        path=directory/item['name'];state=path.stat()
        if path.is_symlink() or state.st_uid!=0 or state.st_mode&0o022:raise SystemExit('Untrusted helper permissions')
        if hashlib.sha256(path.read_bytes()).hexdigest()!=item['sha256']:raise SystemExit('Helper hash mismatch')
        paths.append(path)
    if hashlib.sha256(LIBRARY.read_bytes()).hexdigest()!=LIBRARY_SHA:raise SystemExit('Dopamine library mismatch')
    lib=ctypes.CDLL(str(LIBRARY));lib.jbclient_trust_file_by_path.argtypes=[ctypes.c_char_p]
    lib.jbclient_trust_file_by_path.restype=ctypes.c_int
    for path in paths:
        if lib.jbclient_trust_file_by_path(str(path).encode()):raise SystemExit('Trust operation failed')
    print(json.dumps(dict(trusted_helpers=5,firmware_operations=0,radio_operations=0)))


if __name__=='__main__':main()
