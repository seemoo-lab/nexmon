# SPDX-License-Identifier: GPL-3.0-only
"""Bounded standalone exact-target capture. Root, active Dopamine and exclusive Wi-Fi required.

Use only an owned/authorized AP. Never commits/uploads packets or recovers a
password. A timed-out live native request blocks retry and firmware reload.
"""
import argparse
import contextlib
import ctypes
import fcntl
import hashlib
import json
import os
from pathlib import Path
import platform
import plistlib
import re
import signal
import subprocess
import sys
import time

sys.path.insert(0,str(Path(__file__).resolve().parent))
import firmware
import radio
import raw_transport as transport
from wifi_auth import Collector,pcap,target,parse

STOP=False


class Cancelled(Exception):pass


def stop(signum,frame):
    global STOP
    STOP=True


def atomic(path,value):
    temporary=path.with_suffix('.tmp')
    with temporary.open('w') as file:
        os.fchmod(file.fileno(),0o600);json.dump(value,file);file.flush();os.fsync(file.fileno())
    os.replace(temporary,path)


def verify_tools(directory):
    names={'raw_mirror','monitor_control','auth_tap','monitor_radio','MemoryLauncher'}
    value=directory.stat()
    if value.st_uid!=0 or value.st_mode&0o022:raise ValueError('Untrusted tools directory')
    manifest=json.loads((directory/'native-manifest.json').read_text())
    if {r['name'] for r in manifest['files']}!=names or len(manifest['files'])!=len(names):
        raise ValueError('Native inventory mismatch')
    for row in manifest['files']:
        path=directory/row['name'];value=path.stat()
        if path.is_symlink() or value.st_uid!=0 or value.st_mode&0o022 or firmware.sha(path)!=row['sha256']:
            raise ValueError('Untrusted native helper')
    return [directory/name for name in sorted(names)]


def trust_tools(paths):
    if firmware.sha(firmware.LIBRARY)!=firmware.LIBRARY_SHA:raise RuntimeError('Dopamine library mismatch')
    lib=ctypes.CDLL(firmware.LIBRARY)
    lib.jbclient_trust_file_by_path.argtypes=[ctypes.c_char_p];lib.jbclient_trust_file_by_path.restype=ctypes.c_int
    for path in paths:
        if lib.jbclient_trust_file_by_path(str(path).encode()):raise RuntimeError('Native trust failed')


def address(native):
    path=next((str(p) for p in map(Path,('/sbin/ifconfig','/var/jb/usr/sbin/ifconfig','/var/jb/sbin/ifconfig')) if p.is_file()),None)
    if path is None:raise RuntimeError('ifconfig missing')
    match=re.search(rb'\bether\s+([0-9a-fA-F:]{17})\b',native(path,'en0'))
    if match is None:raise RuntimeError('Own interface address unavailable')
    return match.group(1).decode().lower()


def run(args):
    if os.getuid()!=0 or platform.machine()!='iPhone8,4':raise RuntimeError('Exact target and root required')
    system=plistlib.loads(Path('/System/Library/CoreServices/SystemVersion.plist').read_bytes())
    if system['ProductBuildVersion']!='19H12':raise RuntimeError('iOS build mismatch')
    tools=args.tools.resolve();paths=verify_tools(tools)
    if args.output.exists():raise RuntimeError('Preserve previous run; output already exists')
    # Validate pinned input files before changing trust, preferences or firmware.
    for path,digest in ((firmware.STOCK,firmware.STOCK_SHA),(firmware.LOADER,firmware.LOADER_SHA),
                        (firmware.LIBRARY,firmware.LIBRARY_SHA),(args.candidate,firmware.RAW_CANDIDATE_SHA)):
        if firmware.sha(path)!=digest:raise RuntimeError('Target input mismatch')
    args.output.mkdir(parents=True,mode=0o700);args.output.chmod(0o700)
    transport.APP=tools;radio.RESOURCE=tools
    state=dict(phase='preflight',pid=os.getpid(),monitor=0,blocked=False,ready=False,samples=0,
               frames=0,ordinal_gaps=0,busy_snapshots=0,stock_restored=False,digest_restored=False,autojoin_restored=False)
    def checkpoint(phase,**values):
        state.update(values,phase=phase,timestamp=time.time());atomic(args.output/'state.json',state)
    native=transport.Native(str(tools/'raw_mirror'))
    class Lease(radio.RadioLease):
        def command(self,*values):return native(tools/'MemoryLauncher','128','root',str(tools/'monitor_radio'),*values)
    lease=Lease(args.output/'radio-lease.json')
    fw=owner=collector=None;rows=[];last=0;error=None
    try:
        checkpoint('preflight');trust_tools(paths)
        if native(native.reader,'--self-test').get('firmware_commands')!=0:raise RuntimeError('Reader self-test failed')
        # These branches validate static ABI/CLI parsing before opening a socket.
        for name in ('monitor_control','auth_tap','monitor_radio'):
            test=subprocess.run([str(tools/name),'--self-test'],capture_output=True,timeout=5)
            if test.returncode or json.loads(test.stdout).get('firmware_commands')!=0:raise RuntimeError('Native self-test failed')
        if native(tools/'monitor_control','--status')['monitor']!=0:raise RuntimeError('Monitor already owned')
        selected=native(tools/'auth_tap','--network')
        selected['bssid']=':'.join('%02x'%n for n in target(selected['bssid']))
        ssid=bytes.fromhex(selected['ssid_hex'])
        if not 1<=len(ssid)<=32:raise RuntimeError('Connected SSID missing')
        if args.bssid is not None and target(args.bssid)!=target(selected['bssid']):raise RuntimeError('Selected AP is not connected')
        def associated():
            now=native(tools/'auth_tap','--network')
            return now['bssid'].lower()==selected['bssid'] and bytes.fromhex(now['ssid_hex'])==ssid
        initial=address(native);channel=radio.decode_chanspec(native(tools/'monitor_control','--chanspec')['chanspec'])
        native.chanspec=channel['chanspec']
        if lease.get():raise RuntimeError('Autoconnect already disabled; resolve ownership first')
        collector=Collector(selected['bssid'],selected['ssid_hex'])
        fw=firmware.Firmware(args.candidate,args.output,checkpoint,firmware.RAW_CANDIDATE_SHA)
        atomic(args.output/'private-target.json',dict(selected,own_initial=initial,channel=channel))
        if STOP:raise Cancelled()
        lease.pause();checkpoint('autojoin_paused');fw.load();checkpoint('candidate_ready',digest_restored=fw.digest_restored)
        if native(native.reader,'--info')['enabled']:raise RuntimeError('Mirror unexpectedly armed')
        if STOP:raise Cancelled()
        with (args.output/'monitor.stdout').open('xb') as out,(args.output/'monitor.stderr').open('xb') as err:
            owner=subprocess.Popen([str(tools/'monitor_control'),'--hold',str(args.seconds+60)],
                                   stdin=subprocess.DEVNULL,stdout=out,stderr=err)
        checkpoint('enabling',monitor_pid=owner.pid)
        deadline=time.monotonic()+5
        while native(tools/'monitor_control','--status')['monitor']!=1:
            if owner.poll() is not None or time.monotonic()>deadline:raise RuntimeError('Monitor1 not observed')
            time.sleep(.1)
        if native(tools/'monitor_control','--pin-chanspec',str(native.chanspec))['chanspec']!=native.chanspec:
            raise RuntimeError('Channel pin mismatch')
        native.scope=(selected['bssid'],initial,address(native));native(native.reader,'--arm',*native.scope)
        atomic(args.output/'private-target.json',dict(selected,scope=native.scope,channel=channel))
        checkpoint('arming',monitor=1)
        start=time.monotonic()
        with (args.output/'frames.ndjson').open('x') as file:
            os.fchmod(file.fileno(),0o600)
            while not STOP and time.monotonic()-start<args.seconds and len(rows)<1024:
                if owner.poll() is not None:raise RuntimeError('Monitor owner exited early')
                try:snapshot=native(native.reader,'--snapshot')
                except transport.BusySnapshot:state['busy_snapshots']+=1;time.sleep(.1);continue
                stamp=time.time();items,last,lost=transport.frames_from_snapshot(snapshot,last,native.scope)
                state['ordinal_gaps']+=lost;state['samples']+=1
                for ordinal,frame in items[:1024-len(rows)]:
                    value=parse(frame,stamp,selected['bssid'])
                    if value is None or value['kind']!='eapol':raise RuntimeError('Unexpected mirrored payload')
                    collector.feed(frame,stamp);rows.append((stamp,frame))
                    file.write(json.dumps(dict(ordinal=ordinal,timestamp=stamp,raw_hex=frame.hex()))+'\n')
                file.flush();os.fsync(file.fileno())
                checkpoint('capturing',ready=True,monitor=1,frames=len(rows),summary=collector.summary())
                time.sleep(.25)
    except Cancelled:pass
    except Exception as exc:error=str(exc)
    finally:
        try:
            checkpoint('stopping',ready=False)
            if fw is not None:transport.cleanup(native,owner,fw,lease,checkpoint)
            else:
                if any(p.poll() is None for p in native.pending):raise RuntimeError('Live native request; recovery blocked')
                lease.restore();lease.close()
            state.update(radio_cleanup_verified=True,monitor=0)
            if fw is not None and fw.attempted:transport.wait_association(associated,native)
        except Exception as exc:state['blocked']=True;error=(error+'; ' if error else '')+'cleanup: '+str(exc)
        if rows:
            (args.output/'selected.pcap').write_bytes(pcap(rows));(args.output/'selected.pcap').chmod(0o600)
        if collector is not None and collector.lines:
            (args.output/'selected.hc22000').write_text('\n'.join(collector.lines)+'\n');(args.output/'selected.hc22000').chmod(0o600)
        state.update(error=error,ready=False,pending_native_pids=[p.pid for p in native.pending if p.poll() is None])
        checkpoint('blocked' if state['blocked'] else 'failed' if error else 'stopped')
        print(json.dumps(dict(passed=error is None,phase=state['phase'],frames=len(rows),samples=state['samples'],
                              monitor=state['monitor'],blocked=state['blocked'],error_present=bool(error))),flush=True)
    if error:raise SystemExit(1)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--tools',type=Path,required=True)
    parser.add_argument('--candidate',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True,help='New private run directory')
    parser.add_argument('--seconds',type=int,default=540)
    parser.add_argument('--bssid',help='Optional explicit authorized target; must equal the current association')
    args=parser.parse_args()
    if not 5<=args.seconds<=7140:raise SystemExit('Duration must be5..7140seconds')
    os.umask(0o077)
    for sig in (signal.SIGINT,signal.SIGTERM):signal.signal(sig,stop)
    signal.signal(signal.SIGHUP,signal.SIG_IGN)
    # Coordinate both this CLI and an installed Wi-Fi Map, if present.
    with contextlib.ExitStack() as stack:
        for path in (Path('/var/run/nexmon-monitor.lock'),Path('/var/jb/var/monitor-mode/radio.lock')):
            if path.parent.is_dir():
                file=stack.enter_context(path.open('a'));fcntl.flock(file,fcntl.LOCK_EX|fcntl.LOCK_NB)
        survey=Path('/var/mobile/WiFiSurvey/data/status.json')
        if survey.is_file() and json.loads(survey.read_text()).get('recording'):
            raise SystemExit('Stop Wi-Fi Map recording before this standalone experiment')
        run(args)


if __name__=='__main__':main()
