# SPDX-License-Identifier: GPL-3.0-only
"""Portable synthetic ring, pending-operation and recovery tests; no phone/packets required."""
import signal
import struct
import subprocess
from pathlib import Path
from types import SimpleNamespace
import unittest
from unittest.mock import patch
import zlib
import raw_transport as w

w.APP=Path('/synthetic-tools')
AP=bytes.fromhex('021122334455')
OWN_A=bytes.fromhex('02aabbccddee')
OWN_B=bytes.fromhex('02aabbccddff')
PEER=bytes.fromhex('026677889900')

def frame(key_data=1,peer=PEER):
    e=bytearray(99+key_data);e[0:2]=bytes((2,3));e[2:4]=(95+key_data).to_bytes(2,'big')
    e[4]=2;e[5:7]=(138).to_bytes(2,'big');e[7:9]=(16).to_bytes(2,'big')
    e[9:17]=(1).to_bytes(8,'big');e[17:49]=bytes([1])*32;e[97:99]=key_data.to_bytes(2,'big')
    raw=struct.pack('<HH',0x0288,0)+peer+AP+AP+bytes(4)+bytes.fromhex('aaaa03000000888e')+bytes(e)
    return raw+struct.pack('<I',zlib.crc32(raw))

f=SimpleNamespace(AP=AP,OWN_A=OWN_A,OWN_B=OWN_B,PEER=PEER,frame=frame)
SCOPE=tuple(':'.join('%02x'%b for b in x) for x in (AP,OWN_A,OWN_B))

def snapshot(total=1,peer=f.PEER):
    data=bytearray(4160);struct.pack_into('<16I',data,0,0x31574152,2,total,0,0,0,1,1,*([0]*8))
    data[32:50]=f.AP+f.OWN_A+f.OWN_B;struct.pack_into('<I',data,52,1)
    for ordinal in range(max(1,total-7),total+1):
        raw=f.frame(key_data=ordinal,peer=peer);base=64+((ordinal-1)&7)*512
        struct.pack_into('<4I',data,base,ordinal,len(raw),int.from_bytes(raw[:2],'little'),0)
        data[base+16:base+16+len(raw)]=raw
    return bytes(data)

class RingTests(unittest.TestCase):
    def test_wrap_gap_and_exact_bytes(self):
        rows,last,lost=w.frames_from_snapshot(snapshot(9),0,SCOPE)
        self.assertEqual([n for n,_ in rows],list(range(2,10)))
        self.assertEqual((last,lost),(9,1))
        for n,raw in rows:self.assertEqual(raw,f.frame(key_data=n))
        self.assertEqual(w.frames_from_snapshot(snapshot(9),9,SCOPE),([],9,0))
    def test_own_scope_and_fcs_rejected(self):
        for own in (f.OWN_A,f.OWN_B):
            with self.assertRaises(AssertionError):w.frames_from_snapshot(snapshot(peer=own),0,SCOPE)
        raw=bytearray(snapshot());raw[80+len(f.frame(key_data=1))-1]^=1
        with self.assertRaises(AssertionError):w.frames_from_snapshot(bytes(raw),0,SCOPE)
    def test_reset_epoch_and_scope_echo_rejected(self):
        for offset,value in ((52,2),(4,3),(28,0),(64,2)):
            raw=bytearray(snapshot());struct.pack_into('<I',raw,offset,value)
            with self.assertRaises(AssertionError):w.frames_from_snapshot(bytes(raw),0,SCOPE)
        with self.assertRaises(AssertionError):w.frames_from_snapshot(snapshot(),2,SCOPE)

class NativeTests(unittest.TestCase):
    def test_completed_busy_is_distinct_from_invalid_snapshot(self):
        n=w.Native('/reader')
        child=SimpleNamespace(returncode=2,communicate=lambda timeout:(b'',b'Raw snapshot failed errno=16\n'))
        with patch.object(w.subprocess,'Popen',return_value=child):
            with self.assertRaises(w.BusySnapshot):n('/reader','--snapshot')
        self.assertFalse(n.pending)
        child.communicate=lambda timeout:(b'',b'Raw snapshot failed errno=92\n')
        with patch.object(w.subprocess,'Popen',return_value=child):
            with self.assertRaises(RuntimeError):n('/reader','--snapshot')
    def test_timeout_is_retained(self):
        n=w.Native('/reader')
        def timeout(**kwargs):raise subprocess.TimeoutExpired('reader',8)
        child=SimpleNamespace(communicate=timeout,poll=lambda:None)
        with patch.object(w.subprocess,'Popen',return_value=child):
            with self.assertRaises(RuntimeError):n('/reader','--snapshot')
        self.assertEqual(n.pending,[child])
    def test_mode_commands_and_wrong_scope_not_allowed(self):
        n=w.Native('/reader');n.scope=('a','b','c');n.chanspec=123
        for path,args in [(w.APP/'monitor_control',('--hold','600')),(w.APP/'monitor_control',('--continuous',)),
                          (w.APP/'monitor_control',('--pin-chanspec','456')),('/reader',('--arm','x','b','c'))]:
            with self.assertRaises(RuntimeError):n(path,*args)

class CleanupTests(unittest.TestCase):
    def case(self,pending=False,stuck=False,monitor_value=0,fetch=0):
        events=[]
        class N:
            reader='/reader';armed_maybe=True
            def __call__(self,path,*args):
                if args==('--status',):events.append('status');return dict(monitor=monitor_value)
                if args==('--disarm',):events.append('disarm');return dict(enabled=False)
                events.append('info');return dict(enabled=False,pending=fetch)
        n=N();n.pending=[SimpleNamespace(poll=lambda:None)] if pending else []
        class Owner:
            def poll(self):return None
            def send_signal(self,value):self.signal=value;events.append('signal')
            def wait(self,timeout):
                events.append('wait')
                if stuck:raise subprocess.TimeoutExpired('owner',timeout)
        fw=SimpleNamespace(pending=False,attempted=True,loaded=True,digest_restored=True,restore=lambda:events.append('stock'))
        lease=SimpleNamespace(restore=lambda:events.append('lease'),close=lambda:events.append('close'))
        return events,n,Owner(),fw,lease,lambda *a,**k:events.append('checkpoint')
    def test_normal_order(self):
        a=self.case();w.cleanup(*a[1:]);self.assertEqual(a[0],['signal','wait','status','disarm','info','stock','lease','close','checkpoint'])
        self.assertEqual(a[2].signal,signal.SIGINT)
    def test_pending_native_blocks_even_owner_stop(self):
        a=self.case(pending=True)
        with self.assertRaises(RuntimeError):w.cleanup(*a[1:])
        self.assertEqual(a[0],[])


    def test_stuck_owner_blocks_reload(self):
        a=self.case(stuck=True)
        with self.assertRaises(RuntimeError):w.cleanup(*a[1:])
        self.assertEqual(a[0],['signal','wait'])
    def test_monitor_nonzero_blocks_disarm_reload(self):
        a=self.case(monitor_value=1)
        with self.assertRaises(AssertionError):w.cleanup(*a[1:])
        self.assertEqual(a[0],['signal','wait','status'])
    def test_fetch_pending_blocks_reload(self):
        a=self.case(fetch=1);times=iter((0,6))
        with self.assertRaises(RuntimeError):w.cleanup(*a[1:],clock=lambda:next(times),sleep=lambda _:None)
        self.assertNotIn('stock',a[0])
    def test_loader_pending_blocks_everything(self):
        a=self.case();a[3].pending=True
        with self.assertRaises(RuntimeError):w.cleanup(*a[1:])
        self.assertEqual(a[0],[])

class AssociationTests(unittest.TestCase):
    def test_completed_not_connected_then_connected(self):
        calls=[];native=SimpleNamespace(pending=[])
        def association():
            calls.append(True)
            if len(calls)==1:raise RuntimeError('Native auth_tap failed')
            return len(calls)>=3
        w.wait_association(association,native,clock=lambda:0,sleep=lambda _:None)
        self.assertEqual(len(calls),3)
    def test_pending_native_is_not_retried(self):
        calls=[];native=SimpleNamespace(pending=[SimpleNamespace(poll=lambda:None)])
        def association():calls.append(True);raise RuntimeError('Pending')
        with self.assertRaises(RuntimeError):w.wait_association(association,native,clock=lambda:0,sleep=lambda _:None)
        self.assertEqual(len(calls),1)
    def test_completed_negative_has_deadline(self):
        times=iter((0,21))
        with self.assertRaisesRegex(RuntimeError,'recovery window'):
            w.wait_association(lambda:False,SimpleNamespace(pending=[]),clock=lambda:next(times),sleep=lambda _:None)

class PayloadTests(unittest.TestCase):
    def test_original_frame_and_bad_fcs(self):
        from wifi_auth import parse
        raw=frame(key_data=0)
        value=parse(raw,1.0,SCOPE[0]);self.assertEqual(value['kind'],'eapol');self.assertEqual(value['message'],1)
        self.assertIsNone(parse(raw[:-1]+bytes([raw[-1]^1]),1.0,SCOPE[0]))

    def test_malformed_key_data_is_not_valid_eapol(self):
        from wifi_auth import parse
        self.assertEqual(parse(frame(key_data=1),1.0,SCOPE[0])['kind'],'invalid')

    def test_input_counter_and_scope_bounds(self):
        raw=snapshot(total=9)
        rows,last,gaps=w.frames_from_snapshot(raw,0,SCOPE)
        self.assertEqual((len(rows),last,gaps),(8,9,1))
        with self.assertRaises(AssertionError):w.frames_from_snapshot(raw,0,(SCOPE[1],SCOPE[0],SCOPE[2]))


if __name__=='__main__':unittest.main()
