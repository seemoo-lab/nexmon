#!/usr/bin/env python3
"""Bounded, non-disruptive radio audit; logs every subprocess and result."""
import ctypes
import ctypes.util
import json
from pathlib import Path
import signal
import socket
import struct
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parent
NEX = '/root/nexmon/utilities/nexutil/nexutil'
RESULTS = ROOT / 'commands.jsonl'

def run(name, args, timeout=15, env=None):
    t = time.monotonic()
    try:
        p = subprocess.run(args, capture_output=True, timeout=timeout, env=env)
        rc, out, err = p.returncode, p.stdout, p.stderr
    except subprocess.TimeoutExpired as e:
        rc, out, err = 124, e.stdout or b'', e.stderr or b''
    (ROOT / (name + '.stdout')).write_bytes(out)
    (ROOT / (name + '.stderr')).write_bytes(err)
    result = dict(name=name, args=args, rc=rc, seconds=round(time.monotonic()-t,3))
    with RESULTS.open('a') as f:
        f.write(json.dumps(result)+'\n')
    print(json.dumps(result), flush=True)
    return rc, out, err

def snapshot(tag):
    for suffix, args in [('iw',['iw','dev']), ('phy',['iw','phy']),
                         ('links',['ip','-s','-d','link']), ('dmesg',['dmesg']),
                         ('mode',[NEX,'-I','mon0','-m']),
                         ('caps',[NEX,'-I','mon0','-g400']),
                         ('chanspec',[NEX,'-I','mon0','-k']),
                         ('pm',[NEX,'-I','mon0','-g85']),
                         ('mpc',[NEX,'-I','mon0','-g262','-vmpc','-l8']),
                         ('power',[NEX,'-I','mon0','-g262','-vqtxpower','-l16'])]:
        run(tag+'-'+suffix, args)

def capture(name, seconds=8):
    return run(name, ['timeout','-s','INT',str(seconds),'tcpdump','-i','mon0',
                     '-Q','in','-nn','-s','256','-U','-w',str(ROOT/(name+'.pcap'))], seconds+5)

def channels():
    for channel in [1,6,11,13,36,40,44,48]:
        rc, _, _ = run('channel-'+str(channel), ['iw','dev','mon0','set','channel',str(channel)])
        if rc:
            continue
        run('channel-info-'+str(channel),['iw','dev','mon0','info'])
        capture('capture-ch'+str(channel))

def hopping():
    caplog = (ROOT/'hopping-capture.log').open('wb')
    cap = subprocess.Popen(['tcpdump','-i','mon0','-Q','in','-nn','-s','256','-U','-w',str(ROOT/'hopping.pcap')],stdout=caplog,stderr=caplog)
    try:
        for i in range(60):
            ch=[1,6,11,36,40,44,48][i%7]
            rc,_,_=run('hop-'+str(i),['iw','dev','mon0','set','channel',str(ch)],5)
            if rc:
                break
            time.sleep(.15)
    finally:
        cap.send_signal(signal.SIGINT)
        cap.wait(timeout=5)
        caplog.close()
    run('hopping-mode',[NEX,'-I','mon0','-m'])

def aireplay(channel):
    run('aireplay-channel-'+str(channel),['iw','dev','mon0','set','channel',str(channel)])
    name='aireplay-ch'+str(channel)
    caplog=(ROOT/(name+'-capture.log')).open('wb')
    cap=subprocess.Popen(['tcpdump','-i','mon0','-Q','in','-nn','-s','256','-U','-w',str(ROOT/(name+'.pcap'))],stdout=caplog,stderr=caplog)
    try:
        time.sleep(.5)
        run(name,['timeout','-s','INT','45','stdbuf','-oL','-eL','aireplay-ng','-9','mon0'],50)
    finally:
        cap.send_signal(signal.SIGINT)
        cap.wait(timeout=5)
        caplog.close()

def inject(channel):
    rc,_,_=run('raw-channel-'+str(channel),['nexutil','-Iwlan0','-s30','-i','-v'+str(channel)])
    if rc:
        return
    rx=socket.socket(socket.AF_PACKET,socket.SOCK_RAW,socket.htons(3))
    rx.bind(('wlan0',0)); rx.settimeout(.05)
    tx=socket.socket(socket.AF_PACKET,socket.SOCK_RAW,socket.htons(3))
    tx.bind(('wlan0',0))
    lib=ctypes.CDLL(ctypes.util.find_library('pcap'))
    lib.pcap_open_live.argtypes=[ctypes.c_char_p,ctypes.c_int,ctypes.c_int,ctypes.c_int,ctypes.c_char_p]
    lib.pcap_open_live.restype=ctypes.c_void_p
    lib.pcap_inject.argtypes=[ctypes.c_void_p,ctypes.c_void_p,ctypes.c_size_t]
    lib.pcap_inject.restype=ctypes.c_int
    lib.pcap_close.argtypes=[ctypes.c_void_p]
    err=ctypes.create_string_buffer(256)
    handle=lib.pcap_open_live(b'wlan0',4096,0,10,err)
    if not handle:
        raise RuntimeError(err.value)
    tests=[]
    ap_mac=b"\x5c\xe2\x8c\x23\xdc\xb0"
    # Management probe requests only; unique locally administered addresses.
    variants=[('bare',8,0,b''),('rate',9,4,bytes([2 if channel<15 else 12])),
              ('ofdm6',9,4,b'\x0c'),('ofdm54',9,4,b'\x6c'),
              ('padded128',128,0,b''),('padded256',256,0,b'')]
    try:
        for backend in ['raw','pcap']:
            for n,(name,rlen,present,fields) in enumerate(variants):
                src=bytes([2,0x4e,0x58,channel,0 if backend=='raw' else 1,n+1])
                hdr=struct.pack("<H",0x04)+struct.pack("<H",0)+ap_mac+src+ap_mac+struct.pack("<H",0)
                rt=(struct.pack("<BBHI",0,0,rlen,present)+fields).ljust(rlen,b"\0")
                frame_body=b"\x00\x0a"+b"CYBERSPACE"+b"\x01\x04\x02\x04\x0b\x16"
                pkt=rt+hdr+frame_body
                rec=dict(backend=backend,variant=name,channel=channel,source=src.hex(":"),length=len(pkt),sent=[],outgoing_echoes=0,incoming_probe_responses=0)
                for repeat in range(3):
                    rec['sent'].append(tx.send(pkt) if backend=='raw' else lib.pcap_inject(handle,pkt,len(pkt)))
                    end=time.monotonic()+.25
                    while time.monotonic()<end:
                        try:
                            data,addr=rx.recvfrom(8192)
                        except socket.timeout:
                            continue
                        if len(data)<8:
                            continue
                        off=struct.unpack_from('<H',data,2)[0]
                        body=data[off:]
                        if len(body)<24:
                            continue
                        if addr[2]==socket.PACKET_OUTGOING and body[10:16]==src:
                            rec['outgoing_echoes']+=1
                        if addr[2]!=socket.PACKET_OUTGOING and body[0]&0xfc==0x50 and body[4:10]==src:
                            rec['incoming_probe_responses']+=1
                tests.append(rec)
                print(json.dumps(rec),flush=True)
    finally:
        lib.pcap_close(handle);tx.close();rx.close()
        (ROOT/('raw-injection-ch'+str(channel)+'.json')).write_text(json.dumps(tests,indent=2)+'\n')

if __name__=='__main__':
    action=sys.argv[1]
    if action=='snapshot': snapshot(sys.argv[2])
    elif action=='channels': channels()
    elif action=='hopping': hopping()
    elif action=='aireplay': aireplay(int(sys.argv[2]))
    elif action=='inject': inject(int(sys.argv[2]))
