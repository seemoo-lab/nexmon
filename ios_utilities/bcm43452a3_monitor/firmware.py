# SPDX-License-Identifier: GPL-3.0-only
"""Pinned host digest transaction and original Apple loader. No code patching."""
import ctypes as c
import hashlib
import json
from pathlib import Path
import subprocess

STOCK = '/usr/share/firmware/wifi/C-43452__s-A3/hans.trx'
STOCK_SHA = '280bc2d7ad25f2c789ce332c37da9d687693fbda156830803fe1516e8d938244'
LOADER = '/usr/libexec/wifiFirmwareLoader'
LOADER_SHA = 'a391c0e1c13e161434d4063ade9b9f95669d81cc81382a435adc17519f0a3227'
LIBRARY = '/var/jb/basebin/libjailbreak.dylib'
LIBRARY_SHA = 'ea76a9d9b6af0a50c0aa2ace1f770ad5cfcfff89c4555b2c60dc4b7d1b4427be'
CANDIDATE_SHA = '2c29107967332430da1980a8d7cc3dfd272a2d0cbdc3b0c25af4f19dc25931c5'
AUTH_CANDIDATE_SHA = '2ef5823d2b844e94da96031a7bad8e91370cda15adf2de5feb58aa4efc27a5cc'
RAW_CANDIDATE_SHA = '0245b815b9ff0a5d7e7a3b4f42b491338d2dbeefd8f5bf66cf8a86a83fb00f1f'


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def fixed_json(path, *args):
    run = subprocess.run([str(path), *args], capture_output=True, timeout=4)
    try:
        value = json.loads(run.stdout)
    except ValueError:
        raise RuntimeError('Native helper '+Path(path).name+' exit='+str(run.returncode)+' no JSON') from None
    if run.returncode or value.get('status', 0):
        raise RuntimeError('Native helper '+Path(path).name+' exit='+str(run.returncode)+' status='+str(value.get('status'))+' errno='+str(value.get('errno')))
    return value


class Firmware:
    def __init__(self, candidate, directory, checkpoint, expected_sha=CANDIDATE_SHA):
        if expected_sha not in (CANDIDATE_SHA,AUTH_CANDIDATE_SHA,RAW_CANDIDATE_SHA):raise RuntimeError('Unknown firmware profile')
        self.expected_sha=expected_sha
        self.candidate, self.directory, self.checkpoint = str(candidate), Path(directory), checkpoint
        self.pending = False
        self.digest_restored = False
        self.attempted = False
        self.loaded = False
        for path, expected in [(STOCK, STOCK_SHA), (LOADER, LOADER_SHA), (LIBRARY, LIBRARY_SHA), (candidate, self.expected_sha)]:
            if sha(path) != expected:
                raise RuntimeError('Target hash mismatch: ' + Path(path).name)

    def force_load(self, path, label):
        with (self.directory/(label+'.stdout')).open('xb') as out, (self.directory/(label+'.stderr')).open('xb') as err:
            child = subprocess.Popen([LOADER, '-r', '-F', path], stdin=subprocess.DEVNULL, stdout=out, stderr=err)
            self.checkpoint('loading_'+label, loader_pid=child.pid)
            try:
                status = child.wait(timeout=30)
            except subprocess.TimeoutExpired:
                self.pending = True
                self.checkpoint('blocked_loader_pending', loader_pid=child.pid)
                raise RuntimeError('Loader pending; stock reload prohibited')
        if status:
            raise RuntimeError('Loader exit ' + str(status))

    def load(self):
        lib = c.CDLL(LIBRARY)
        xpc = c.CDLL('/usr/lib/system/libxpc.dylib')
        lib.jbclient_root_get_sysinfo.argtypes = [c.POINTER(c.c_void_p)]
        lib.jbclient_root_get_sysinfo.restype = c.c_int
        xpc.xpc_dictionary_get_uint64.argtypes = [c.c_void_p, c.c_char_p]
        xpc.xpc_dictionary_get_uint64.restype = c.c_uint64
        xpc.xpc_release.argtypes = [c.c_void_p]
        info = c.c_void_p()
        if lib.jbclient_root_get_sysinfo(c.byref(info)) or not info.value:
            raise RuntimeError('Dopamine sysinfo unavailable')
        try:
            values = [xpc.xpc_dictionary_get_uint64(info, ('kernelConstant.'+key).encode()) for key in ('base', 'staticBase', 'slide')]
        finally:
            xpc.xpc_release(info)
        base, static, slide = values
        if static != 0xfffffff007004000 or base != static+slide or not 0 <= slide <= 0x100000000:
            raise RuntimeError('Kernel mapping mismatch')
        lib.jbclient_initialize_primitives.argtypes = []
        lib.jbclient_initialize_primitives.restype = c.c_int
        if lib.jbclient_initialize_primitives():
            raise RuntimeError('Dopamine primitives unavailable')
        for name in ('kreadbuf', 'kwritebuf'):
            function = getattr(lib, name)
            function.argtypes = [c.c_uint64, c.c_void_p, c.c_size_t]
            function.restype = c.c_int

        def read():
            data = c.create_string_buffer(64)
            if lib.kreadbuf(0xfffffff0054f042c+slide, data, 64):
                raise RuntimeError('Digest read failed')
            return data.raw

        def write(value):
            if lib.kwritebuf(0xfffffff0054f043c+slide, c.create_string_buffer(value), 32):
                raise RuntimeError('Digest write failed')

        original = read()
        if original[16:48].hex() != STOCK_SHA:
            raise RuntimeError('Original digest mismatch; no write')
        try:
            self.attempted = True
            write(bytes.fromhex(self.expected_sha))
            if read() != original[:16]+bytes.fromhex(self.expected_sha)+original[48:]:
                raise RuntimeError('Candidate digest readback mismatch')
            self.force_load(self.candidate, 'candidate')
            self.loaded = True
        finally:
            write(original[16:48])
            self.digest_restored = read() == original
            self.checkpoint('digest_restored' if self.digest_restored else 'blocked_digest', digest_restored=self.digest_restored)
        if not self.digest_restored:
            raise RuntimeError('Digest restore failed')

    def restore(self):
        if self.attempted and self.digest_restored and not self.pending:
            self.force_load(STOCK, 'stock')
            self.checkpoint('stock_restored', stock_restored=True)
        else:
            raise RuntimeError('Unsafe stock restore state')


def restore_stock(directory,checkpoint):
    """Restore the exact stock image; no digest write or arbitrary image input."""
    for path,expected in ((STOCK,STOCK_SHA),(LOADER,LOADER_SHA)):
        if sha(path)!=expected:raise RuntimeError('Stock recovery integrity mismatch')
    directory=Path(directory)
    with (directory/'stock.stdout').open('xb') as out,(directory/'stock.stderr').open('xb') as err:
        child=subprocess.Popen([LOADER,'-r','-F',STOCK],stdin=subprocess.DEVNULL,stdout=out,stderr=err)
        checkpoint('loading_stock',loader_pid=child.pid)
        try:status=child.wait(timeout=30)
        except subprocess.TimeoutExpired:
            checkpoint('blocked_loader_pending',loader_pid=child.pid,loader_pending=True)
            raise RuntimeError('Stock loader pending; do not retry') from None
    if status:raise RuntimeError('Stock loader exit '+str(status))
    checkpoint('stock_restored',stock_restored=True,firmware_resident=False)
