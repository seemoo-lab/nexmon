# SPDX-License-Identifier: GPL-3.0-only
"""Crash-safe autojoin lease via a bounded, WLAN-entitled native helper."""
import json
import os
from pathlib import Path
import subprocess

RESOURCE=None  # Set to the root-owned tools directory by the standalone CLI.


def decode_chanspec(value):
    if type(value) is not int or not 0<value<=65535:raise ValueError('Invalid chanspec')
    band=value&0xc000;center=value&255;side=(value>>8)&7
    modes={0x1000:(20,0),0x1800:(40,2),0x2000:(80,6)}
    if band not in (0,0xc000) or value&0x3800 not in modes:raise ValueError('Unsupported chanspec')
    width,offset=modes[value&0x3800];primary=center-offset+4*side
    if side>=width//20 or not 1<=primary<=196 or not 1<=center<=196 or (band==0 and (primary>14 or width==80)) or (band==0xc000 and primary<=14):raise ValueError('Invalid control channel')
    return dict(chanspec=value,primary=primary,center=center,width=width)


class RadioLease:
    def __init__(self,path):self.path=path

    def command(self,*args):
        run=subprocess.run([str(RESOURCE/'MemoryLauncher'),'128','root',str(RESOURCE/'monitor_radio'),*args],
                           capture_output=True,timeout=8)
        try:value=json.loads(run.stdout)
        except (ValueError,UnicodeError):raise RuntimeError('No reply from Wi-Fi helper') from None
        if run.returncode or value.get('ok') is not True:raise RuntimeError('CoreWiFi autojoin operation unavailable')
        return value

    def get(self):return self.command('--get')['disabled']

    def set(self,value):
        result=self.command('--set','1' if value else '0')
        if result.get('disabled') is not bool(value):raise RuntimeError('CoreWiFi autojoin change not verified')

    def save(self,value):
        temporary=self.path.with_suffix('.tmp')
        with temporary.open('w') as out:
            os.chmod(temporary,0o600);json.dump(value,out);out.flush();os.fsync(out.fileno())
        os.replace(temporary,self.path)

    def pause(self):
        if self.path.exists():raise RuntimeError('Radio lease already exists')
        value=dict(original_disabled=self.get(),attempted=True,restored=False)
        self.save(value)
        self.set(True)

    def restore(self):
        if not self.path.exists():return
        value=json.loads(self.path.read_text())
        if type(value.get('original_disabled')) is not bool:raise RuntimeError('Invalid autojoin recovery journal')
        if not value.get('restored'):
            self.set(value['original_disabled']);value['restored']=True;self.save(value)

    def close(self):pass
