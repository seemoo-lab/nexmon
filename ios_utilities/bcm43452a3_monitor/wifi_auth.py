# SPDX-License-Identifier: GPL-3.0-only
"""Bounded passive WPA2 evidence parser for one explicitly selected BSSID.

No RF operations. No password guessing. Shared by the receiver and DB reader.
Definitions: hostap src/common/wpa_common.h; hashcat's hc22000 format.
"""
from collections import OrderedDict
import hashlib
import math
import re
import struct
import zlib


def target(value):
    if not isinstance(value,str) or not re.fullmatch(r'(?:[0-9a-fA-F]{2}:){5}[0-9a-fA-F]{2}',value):
        raise ValueError('Invalid BSSID')
    raw=bytes.fromhex(value.replace(':',''))
    if not any(raw) or raw[0]&1:raise ValueError('Unicast BSSID required')
    return raw


def elements(data):
    result=[];offset=0
    while offset<len(data):
        if offset+2>len(data):raise ValueError('Truncated IE header')
        ident,size=data[offset:offset+2];offset+=2
        if offset+size>len(data):raise ValueError('Truncated IE body')
        result.append((ident,data[offset:offset+size]));offset+=size
    return result


def rsn_akm(data):
    if len(data)<8 or data[:2]!=b'\x01\0':raise ValueError('Invalid RSN')
    count=int.from_bytes(data[6:8],'little');offset=8+4*count
    if not 1<=count<=32 or offset+2>len(data):raise ValueError('Invalid pairwise list')
    count=int.from_bytes(data[offset:offset+2],'little');offset+=2
    if not 1<=count<=32 or offset+4*count>len(data):raise ValueError('Invalid AKM list')
    return [data[i:i+4].hex() for i in range(offset,offset+4*count,4)]


def rsn_pmkids(data):
    rsn_akm(data)
    offset=8+4*int.from_bytes(data[6:8],'little')
    offset+=2+4*int.from_bytes(data[offset:offset+2],'little')
    if len(data)==offset:return []
    if len(data)<offset+2:raise ValueError('Truncated RSN capabilities')
    offset+=2
    if len(data)==offset:return []
    if len(data)<offset+2:raise ValueError('Truncated PMKID count')
    count=int.from_bytes(data[offset:offset+2],'little');offset+=2
    if count>16 or len(data)-offset-16*count not in (0,4):raise ValueError('Invalid PMKID list')
    return [data[i:i+16].hex() for i in range(offset,offset+16*count,16) if any(data[i:i+16])]


def parse(frame,stamp,bssid):
    """Return selected, FCS-valid beacon or complete plaintext EAPOL-Key.

    A selected EAPOL prefix with missing declared bytes is only a diagnostic.
    It never becomes an exportable frame.
    """
    ap=target(bssid)
    if not isinstance(frame,bytes) or not 28<=len(frame)<=4096 or not math.isfinite(stamp) or stamp<=0:return None
    if zlib.crc32(frame[:-4])!=int.from_bytes(frame[-4:],'little'):return None
    fc=int.from_bytes(frame[:2],'little');kind=(fc>>2)&3;subtype=(fc>>4)&15;ds=(fc>>8)&3
    if fc&3 or fc&0x4400 or int.from_bytes(frame[22:24],'little')&15:return None
    payload=frame[:-4]
    if kind==0 and ds==0 and subtype in (0,2) and frame[16:22]==ap and frame[4:10]==ap:
        station=frame[10:16];offset=28 if subtype==0 else 34
        if not any(station) or station[0]&1 or len(payload)<offset:return None
        try:
            ies=elements(payload[offset:]);names=[v for i,v in ies if i==0];rsns=[v for i,v in ies if i==48]
            if len(names)!=1 or not 1<=len(names[0])<=32 or len(rsns)!=1:return None
            akm=rsn_akm(rsns[0]);pmkids=rsn_pmkids(rsns[0])
        except ValueError:return None
        if not pmkids:return None
        return dict(kind='association',timestamp=stamp,ssid_hex=names[0].hex(),station=station.hex(),akm=akm,pmkids=pmkids)
    if kind==0 and ds==0 and subtype in (5,8) and frame[16:22]==ap and frame[10:16]==ap:
        if len(payload)<36:return None
        try:
            ies=elements(payload[36:]);names=[v for i,v in ies if i==0];akms=[]
            for ident,value in ies:
                if ident==48:akms=rsn_akm(value)
            if len(names)!=1 or len(names[0])>32:return None
        except ValueError:return None
        return dict(kind='beacon',timestamp=stamp,ssid_hex=names[0].hex(),akm=akms)
    if kind!=2 or ds not in (1,2):return None
    addr_ap,station=(frame[4:10],frame[10:16]) if ds==1 else (frame[10:16],frame[4:10])
    if addr_ap!=ap or not any(station) or station[0]&1:return None
    header=24
    if subtype&8:
        if len(payload)<26 or payload[24]&0x80:return None  # A-MSDU needs separate validated reassembly.
        header+=2
        if fc&0x8000:header+=4
    llc=b'\xaa\xaa\x03\0\0\0\x88\x8e'
    if payload[header:header+8]!=llc:return None
    eapol=payload[header+8:]
    return parse_eapol(eapol,stamp,station,ds==2)


def parse_ethernet(frame,stamp,bssid):
    """Validate actual endpoint Ethernet bytes; no invented radio header/FCS."""
    if not isinstance(frame,bytes) or not 113<=len(frame)<=2062 or frame[12:14]!=b'\x88\x8e':return None
    ap=target(bssid)
    if frame[6:12]==ap:station=frame[:6];from_ap=True
    elif frame[:6]==ap:station=frame[6:12];from_ap=False
    else:return None
    return parse_eapol(frame[14:],stamp,station,from_ap)


def parse_eapol(eapol,stamp,station,from_ap):
    """Protocol validation shared by verified 802.11 and observed own Ethernet.

    The caller establishes frame provenance and AP/client direction. This does
    not invent a radio header, FCS, timestamp, or proof of MIC correctness.
    """
    if not isinstance(eapol,bytes) or not math.isfinite(stamp) or stamp<=0:return None
    if not isinstance(station,bytes) or len(station)!=6 or not any(station) or station[0]&1:return None
    if type(from_ap) is not bool:return None
    if len(eapol)<4:return dict(kind='incomplete',timestamp=stamp)
    declared=4+int.from_bytes(eapol[2:4],'big')
    if eapol[1]!=3 or eapol[0] not in (1,2,3):return None
    if declared>len(eapol):return dict(kind='incomplete',timestamp=stamp,expected=declared,received=len(eapol))
    if not 99<=declared<=2048 or declared!=len(eapol) or eapol[4]!=2:return None
    info=int.from_bytes(eapol[5:7],'big');version=info&7
    if not info&8 or info&0x0c00 or version not in (1,2,3):return None
    if 99+int.from_bytes(eapol[97:99],'big')!=declared:return dict(kind='invalid',timestamp=stamp)
    ack,mic,secure,install=bool(info&128),bool(info&256),bool(info&512),bool(info&64)
    message=1 if ack and not mic and not secure and not install else 3 if ack and mic and install else 4 if not ack and mic and secure else 2 if not ack and mic and not install else 0
    if not message or (message in (1,3))!=from_ap:return None
    nonce=eapol[17:49];keymic=eapol[81:97]
    if message in (1,2,3) and not any(nonce):return None
    if mic and not any(keymic):return None
    pmkids=[]
    if message==1 and not info&4096:
        try:
            for ident,value in elements(eapol[99:]):
                if ident==221 and len(value)==20 and value[:4]==b'\x00\x0f\xac\x04' and any(value[4:]):
                    if value[4:].hex() not in pmkids:pmkids.append(value[4:].hex())
        except ValueError:return dict(kind='invalid',timestamp=stamp)
    akms=[]
    if message==2:
        try:
            for ident,value in elements(eapol[99:]):
                if ident==48:akms=rsn_akm(value)
        except ValueError:return dict(kind='invalid',timestamp=stamp)
    return dict(kind='eapol',timestamp=stamp,message=message,station=station.hex(),replay=int.from_bytes(eapol[9:17],'big'),
                nonce=nonce.hex(),mic=keymic.hex(),version=version,eapol=eapol.hex(),pmkids=pmkids,akm=akms)


class Collector:
    def __init__(self,bssid,ssid_hex=''):
        self.ap=target(bssid).hex();self.bssid=':'.join(self.ap[i:i+2] for i in range(0,12,2))
        self.ssid=bytes.fromhex(ssid_hex)
        if len(self.ssid)>32:raise ValueError('SSID exceeds 32 bytes')
        self.akm=[];self.messages=OrderedDict();self.client_pmkids=OrderedDict();self.lines=OrderedDict();self.seen=OrderedDict()
        self.counts={str(n):0 for n in range(1,5)};self.incomplete=0;self.invalid=0;self.complete=0
        self.transport=dict(from_ap=0,to_ap=0,protected_from_ap=0,protected_to_ap=0,eapol_from_ap=0,eapol_to_ap=0,amsdu_from_ap=0,
                            unverified_eapol_from_ap=0,unverified_eapol_to_ap=0,unverified_short_from_ap=0,unverified_ethernet_eapol_to_ap=0)

    def observe_unverified(self,frame):
        # Header/LLC pattern only: these counters never establish a valid frame
        # and never retain or export an unverified EAPOL body.
        if not isinstance(frame,bytes) or not 36<=len(frame)<=4096:return
        if frame[:6].hex()==self.ap and frame[12:14]==b'\x88\x8e':
            self.transport['unverified_ethernet_eapol_to_ap']+=1
        fc=int.from_bytes(frame[:2],'little');ds=(fc>>8)&3;subtype=(fc>>4)&15
        if fc&3 or (fc>>2)&3!=2 or ds not in (1,2) or fc&0x4000:return
        ap=frame[4:10] if ds==1 else frame[10:16]
        if ap.hex()!=self.ap:return
        header=24+(2 if subtype&8 else 0)+(4 if subtype&8 and fc&0x8000 else 0)
        if frame[header:header+8]!=b'\xaa\xaa\x03\0\0\0\x88\x8e':return
        self.transport['unverified_eapol_to_ap' if ds==1 else 'unverified_eapol_from_ap']+=1
        body=frame[header+8:]
        if ds==2 and len(body)>=4 and 4+int.from_bytes(body[2:4],'big')>len(body):self.transport['unverified_short_from_ap']+=1

    def feed(self,frame,stamp):
        if not isinstance(frame,bytes) or len(frame)<28 or zlib.crc32(frame[:-4])!=int.from_bytes(frame[-4:],'little'):
            self.observe_unverified(frame)
            return None
        if isinstance(frame,bytes) and 28<=len(frame)<=4096 and zlib.crc32(frame[:-4])==int.from_bytes(frame[-4:],'little'):
            fc=int.from_bytes(frame[:2],'little');ds=(fc>>8)&3;subtype=(fc>>4)&15
            if (fc>>2)&3==2 and ds in (1,2):
                ap=frame[4:10] if ds==1 else frame[10:16]
                if ap.hex()==self.ap:
                    direction='to_ap' if ds==1 else 'from_ap';self.transport[direction]+=1
                    if fc&0x4000:self.transport['protected_'+direction]+=1
                    header=24+(2 if subtype&8 else 0)+(4 if subtype&8 and fc&0x8000 else 0)
                    if subtype&8 and frame[24]&0x80 and ds==2:self.transport['amsdu_from_ap']+=1
                    if not fc&0x4000 and frame[header:header+8]==b'\xaa\xaa\x03\0\0\0\x88\x8e':self.transport['eapol_'+direction]+=1
        return self._accept(parse(frame,stamp,self.bssid))

    def feed_ethernet(self,frame,stamp):
        """Observed own-supplicant Ethernet; never treat it as FCS-verified RF."""
        return self._accept(parse_ethernet(frame,stamp,self.bssid))

    def _accept(self,value):
        if not value:return None
        stamp=value['timestamp']
        kind=value['kind']
        if kind in ('incomplete','invalid'):
            if kind=='incomplete':self.incomplete+=1
            else:self.invalid+=1
            return None
        if kind in ('beacon','association'):
            observed=bytes.fromhex(value['ssid_hex'])
            if observed and self.ssid and observed!=self.ssid:return None
            if observed:self.ssid=observed
            self.akm=value['akm']
            fingerprint=kind+':'+value['ssid_hex']+','.join(value['akm'])
            if kind=='association':fingerprint+=value['station']+','.join(value['pmkids'])
        else:fingerprint=hashlib.sha256(bytes.fromhex(value['eapol'])+bytes.fromhex(value['station'])).hexdigest()
        if fingerprint in self.seen:return None
        self.seen[fingerprint]=stamp
        while len(self.seen)>512:self.seen.popitem(last=False)
        if kind=='eapol':
            self.complete+=1;self.counts[str(value['message'])]+=1
            key=(value['station'],value['message'],value['replay'])
            # Keep distinct nonce attempts at equal replay counters separate.
            self.messages[(key,value['nonce'])]=value
            while len(self.messages)>128:self.messages.popitem(last=False)
        elif kind=='association' and value['akm']==['000fac02']:
            for pmkid in value['pmkids']:self.client_pmkids[(value['station'],pmkid)]=True
            while len(self.client_pmkids)>64:self.client_pmkids.popitem(last=False)
        self._assemble()
        return value

    def _assemble(self):
        self.lines.clear()
        if not self.ssid:return
        if self.akm==['000fac02']:
            for station,pmkid in self.client_pmkids:
                self.lines['WPA*01*'+pmkid+'*'+self.ap+'*'+station+'*'+self.ssid.hex()+'***04']='PMKID'
        for message in list(self.messages.values()):
            if message['message']==1 and self.akm==['000fac02']:
                for pmkid in message['pmkids']:
                    self.lines['WPA*01*'+pmkid+'*'+self.ap+'*'+message['station']+'*'+self.ssid.hex()+'***02']='PMKID'
            if message['message']!=2 or message['version']!=2 or message['akm']!=['000fac02']:continue
            matches=[]
            for ap in self.messages.values():
                if ap['station']!=message['station'] or ap['version']!=message['version'] or abs(ap['timestamp']-message['timestamp'])>10:continue
                pair=0 if ap['message']==1 and ap['replay']==message['replay'] else 2 if ap['message']==3 and ap['replay']==message['replay']+1 else None
                if pair is None:continue
                matches.append((ap,pair))
            # Reused replay counters across different nonces are ambiguous.
            if len({ap['nonce'] for ap,pair in matches})!=1 or len(bytes.fromhex(message['eapol']))>256:continue
            for ap,pair in matches:
                eapol=bytearray.fromhex(message['eapol']);eapol[81:97]=bytes(16)
                line='*'.join(('WPA','02',message['mic'],self.ap,message['station'],self.ssid.hex(),ap['nonce'],eapol.hex(),'%02x'%pair))
                self.lines[line]='M1+M2' if pair==0 else 'M2+M3'
        while len(self.lines)>64:self.lines.popitem(last=False)

    def summary(self):
        return dict(complete_eapol=self.complete,messages=dict(self.counts),incomplete=self.incomplete,invalid=self.invalid,
                    pairs=sum(v!='PMKID' for v in self.lines.values()),pmkids=sum(v=='PMKID' for v in self.lines.values()),
                    exportable=bool(self.lines),akm=list(self.akm),mic_verified=False,transport=dict(self.transport))


def pcap(rows,linktype=105):
    """Observed Ethernet(DLT1) or FCS-verified IEEE802.11(DLT105)."""
    if linktype not in (1,105):raise ValueError('Unsupported capture format')
    out=bytearray(struct.pack('<IHHIIII',0xa1b2c3d4,2,4,0,0,4096,linktype))
    for stamp,frame in rows:
        sec=int(stamp);usec=min(999999,max(0,int((stamp-sec)*1e6)));data=frame if linktype==1 else frame[:-4]
        out.extend(struct.pack('<IIII',sec,usec,len(data),len(data)));out.extend(data)
    return bytes(out)
