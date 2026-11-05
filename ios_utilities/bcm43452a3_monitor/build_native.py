# SPDX-License-Identifier: GPL-3.0-only
"""Build/sign project-authored arm64 helpers with a separately obtained iOS SDK.

No firmware, trust-cache, radio or network operations. Output is private and must
not be committed. Execute helper self-tests only after target-specific trusting.
"""
import argparse
import hashlib
import json
from pathlib import Path
import plistlib
import subprocess


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sdk',type=Path,required=True)
    parser.add_argument('--cc',default='clang')
    parser.add_argument('--ldid',default='ldid')
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    if not args.sdk.is_dir():raise SystemExit('SDK directory is missing')
    if args.output.exists():raise SystemExit('Preserve existing native output')
    args.output.mkdir(parents=True)
    source=Path(__file__).resolve().parent/'source'
    records=[]
    for filename,name,wlan in [('RawMirror.c','raw_mirror',True),('monitor_control.c','monitor_control',True),
                               ('AuthTap.c','auth_tap',True),('MonitorRadio.m','monitor_radio',True),
                               ('MemoryLauncher.c','MemoryLauncher',False)]:
        binary=args.output/name
        command=[args.cc,'-target','arm64-apple-ios15.0','-isysroot',str(args.sdk),'-Os','-Wall','-Wextra','-Werror',str(source/filename),'-o',str(binary)]
        if filename.endswith('.m'):command+=['-fobjc-arc','-framework','Foundation']
        subprocess.run(command,check=True)
        permissions={'com.apple.wlan.authentication':True} if wlan else {}
        if name=='monitor_radio':permissions.update({'com.apple.private.corewifi.internal':True,'com.apple.wifi.manager-access':True})
        entitlement=args.output/(name+'.plist');entitlement.write_bytes(plistlib.dumps(permissions))
        subprocess.run([args.ldid,'-Cadhoc','-S'+str(entitlement),str(binary)],check=True)
        readback=subprocess.run([args.ldid,'-e',str(binary)],capture_output=True,check=True)
        if plistlib.loads(readback.stdout)!=permissions:raise RuntimeError('Entitlement readback mismatch')
        binary.chmod(0o755)
        records.append(dict(name=name,sha256=hashlib.sha256(binary.read_bytes()).hexdigest(),bytes=binary.stat().st_size))
    (args.output/'native-manifest.json').write_text(json.dumps(dict(files=records,target='arm64-apple-ios15.0'),indent=2)+'\n')
    print(json.dumps(dict(status='NATIVE_HELPERS_BUILT',helpers=len(records),firmware_operations=0,radio_operations=0,trust_operations=0)))


if __name__=='__main__':main()
