# SPDX-License-Identifier: GPL-3.0-only
"""Scoped raw-frame transport and conservative cleanup, verified on the target SE."""
import json
from pathlib import Path
import re
import signal
import struct
import subprocess
import time
import zlib

APP=None  # Set to the root-owned tools directory by the standalone CLI.


class BusySnapshot(Exception):pass


class Native:
    def __init__(self,reader):
        self.reader=reader;self.pending=[];self.scope=None;self.arm_attempts=0
        self.armed_maybe=False;self.chanspec=None
    def __call__(self,path,*args):
        allowed=(str(path)==self.reader and (args in (('--self-test',),('--info',),('--disarm',),('--snapshot',)) or
                 (self.scope is not None and args==('--arm',*self.scope))))
        allowed=allowed or (path==APP/'monitor_control' and (args in (('--status',),('--chanspec',)) or
                   (self.chanspec is not None and args==('--pin-chanspec',str(self.chanspec)))))
        allowed=allowed or (path==APP/'auth_tap' and args==('--network',))
        allowed=allowed or (path==APP/'MemoryLauncher' and args in
            (('128','root',str(APP/'monitor_radio'),'--get'),('128','root',str(APP/'monitor_radio'),'--set','0'),
             ('128','root',str(APP/'monitor_radio'),'--set','1')))
        allowed=allowed or (str(path) in ('/sbin/ifconfig','/var/jb/usr/sbin/ifconfig','/var/jb/sbin/ifconfig') and args==('en0',))
        if not allowed:raise RuntimeError('Native command outside scoped capture')
        if args and args[0]=='--arm':self.arm_attempts+=1;self.armed_maybe=True
        p=subprocess.Popen([str(path),*args],stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
        try:out,err=p.communicate(timeout=8)
        except subprocess.TimeoutExpired:
            self.pending.append(p);raise RuntimeError('Native request pending; no retry/reload') from None
        if str(path)==self.reader and args==('--snapshot',):
            if p.returncode==2 and not out and re.fullmatch(rb'Raw snapshot failed errno=16\s*',err):raise BusySnapshot()
            if p.returncode or len(out)!=4160:raise RuntimeError('Raw snapshot failed structural/FCS validation')
            return out
        if Path(path).name=='ifconfig':
            if p.returncode:raise RuntimeError('Interface read failed')
            return out
        value=json.loads(out)
        if p.returncode or value.get('status',0) or value.get('ok',True) is not True:raise RuntimeError('Native '+Path(path).name+' failed')
        if str(path)==self.reader and args==('--disarm',) and value.get('enabled') is False:self.armed_maybe=False
        return value


def cleanup(native,monitor,fw,lease,checkpoint,clock=time.monotonic,sleep=time.sleep):
    if fw.pending or any(p.poll() is None for p in native.pending):raise RuntimeError('Pending operation; cleanup blocked')
    if fw.attempted and not fw.digest_restored:raise RuntimeError('Digest restoration unverified')
    if monitor is not None and monitor.poll() is None:
        monitor.send_signal(signal.SIGINT)
        try:monitor.wait(timeout=6)
        except subprocess.TimeoutExpired:raise RuntimeError('Monitor owner still running; no reload') from None
    assert native(APP/'monitor_control','--status')['monitor']==0,'Monitor0 not verified'
    if fw.loaded:
        if native.armed_maybe:native(native.reader,'--disarm')
        deadline=clock()+5
        while True:
            value=native(native.reader,'--info')
            assert value['enabled'] is False
            if value['pending']==0:break
            if clock()>deadline:raise RuntimeError('Fetch pending; no reload')
            sleep(.1)
    if fw.attempted:fw.restore()
    lease.restore();lease.close()
    checkpoint('cleanup_complete',stock_restored=bool(fw.attempted),autojoin_restored=True,monitor=0)


def frames_from_snapshot(raw,last,scope):
    assert len(raw)==4160
    state=struct.unpack_from('<16I',raw)
    expected=b''.join(bytes.fromhex(v.replace(':','')) for v in scope)
    assert state[0]==0x31574152 and state[6]==1 and state[7]==1 and not state[1]&1
    assert state[4]==0 and state[13]==1 and raw[32:50]==expected
    total=state[2];assert total>=last,'Counter reset/wrap not accepted in bounded trial'
    lost=max(0,total-last-8);result=[]
    own={expected[6:12],expected[12:18]}
    for ordinal in range(max(last+1,total-7),total+1):
        pos=64+((ordinal-1)&7)*512
        observed,length,fc,reserved=struct.unpack_from('<4I',raw,pos)
        assert observed==ordinal and not reserved and 135<=length<=496
        frame=raw[pos+16:pos+16+length];assert int.from_bytes(frame[:2],'little')==fc
        assert zlib.crc32(frame[:-4])==int.from_bytes(frame[-4:],'little'),'Invalid original FCS'
        ds=(fc>>8)&3;assert ds in (1,2)
        ap,peer=(frame[4:10],frame[10:16]) if ds==1 else (frame[10:16],frame[4:10])
        assert ap==expected[:6] and peer not in own,'Scope violation'
        result.append((ordinal,frame))
    return result,total,lost


def wait_association(associated,native,clock=time.monotonic,sleep=time.sleep):
    deadline=clock()+20
    while True:
        try:
            if associated():return
        except (RuntimeError,ValueError):
            if any(p.poll() is None for p in native.pending):raise
        if clock()>=deadline:raise RuntimeError('Association not restored within recovery window')
        sleep(.25)
