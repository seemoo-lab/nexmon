#!/usr/bin/env python3
"""Derive definitions.mk values for a bcm43455c0 firmware image.
Self-tests against 7_45_206 / 7_45_234_4ca95bb_CY / 7_45_241, then prints 7_45_265."""
import struct, sys, re
RS = 0x198000
UCODE_MAGIC = bytes.fromhex('4e10000360bc0100')

def slots(d, val):
    b = struct.pack('<I', val); out = []; i = d.find(b)
    while i != -1:
        if i % 4 == 0: out.append(RS + i)
        i = d.find(b, i + 1)
    return out

def rd(d, a): return struct.unpack('<I', d[a-RS:a-RS+4])[0]

def derive(path):
    d = open(path, 'rb').read(); r = {}
    # ucode + the BL hook whose literal pool holds UCODESTART at hook+0x10
    hits = [RS + i for i in range(len(d)) if d.startswith(UCODE_MAGIC, i)]
    assert len(hits) == 1, hits
    r['UCODESTART'] = us = hits[0]
    s = [a for a in slots(d, us) if a > 0x200000]
    assert len(s) == 1, s
    r['WLC_UCODE_WRITE_BL_HOOK_ADDR'] = s[0] - 0x10
    # reclaim_0_end: the <end>,0x1980D4,0x198100,0x198000,0x198000 tail
    for a in range(0x19A000, 0x19A800, 4):
        if [rd(d, a+4*k) for k in (1,2,3,4)] == [0x1980D4, 0x198100, 0x198000, 0x198000]:
            r['HNDRTE_RECLAIM_0_END_PTR'] = a; r['HNDRTE_RECLAIM_0_END'] = rd(d, a); break
    # templateram: slot at UCODESTART-0x16C, if it points just below reclaim_0_end
    t = rd(d, us - 0x16C)
    if us < t < r['HNDRTE_RECLAIM_0_END']:
        r['TEMPLATERAMSTART_PTR'] = us - 0x16C; r['TEMPLATERAMSTART'] = t
        r['TEMPLATERAMSIZE'] = r['HNDRTE_RECLAIM_0_END'] - t
        r['UCODESIZE'] = t - us
    else:
        r['TEMPLATERAMSTART_PTR'] = r['TEMPLATERAMSTART'] = r['TEMPLATERAMSIZE'] = 0
        r['UCODESIZE'] = r['HNDRTE_RECLAIM_0_END'] - us
    # flash-patch bookkeeping: six aligned slots equal to FP_CONFIG_ORIGBASE
    fp = slots(d, 0x199000)
    assert len(fp) == 6, [hex(x) for x in fp]
    r['FP_DATA_END_PTR'] = rd(d, fp[0] - 4)
    r['FP_CONFIG_BASE_PTR_1'], r['FP_CONFIG_END_PTR_1'] = fp[1], fp[1] - 4
    r['FP_CONFIG_BASE_PTR_2'], r['FP_CONFIG_END_PTR_2'] = fp[5], fp[5] - 4
    a = 0x199000
    while True:
        tg, sz, dp = struct.unpack('<III', d[a-RS:a-RS+12])
        if not (0 <= tg < 0xB0000 and 0 < sz <= 0x40 and RS <= dp < RS + len(d)): break
        a += 12
    r['FP_CONFIG_ORIGEND'] = a
    assert rd(d, r['FP_CONFIG_END_PTR_1']) == a, 'ORIGEND disagrees with END_PTR_1'
    # version / date / time string slots
    m = re.search(rb'7\.45\.\d+ \([0-9a-fr]+ CY\)\x00', d)
    r['_VERSION'] = m.group()[:-1].decode(); vs = slots(d, RS + m.start())
    for n, v in enumerate(vs, 1): r[f'VERSION_PTR_{n}'] = v
    dm = re.search(rb'[A-Z][a-z]{2} [ 0-9][0-9] 20[0-9]{2}\x00', d)
    r['DATE_PTR'] = slots(d, RS + dm.start())[0]
    tm = re.search(rb'[0-9]{2}:[0-9]{2}:[0-9]{2}\x00', d[dm.start():])
    r['TIME_PTR'] = slots(d, RS + dm.start() + tm.start())[0]
    # invariants
    assert r['UCODESTART'] + r['UCODESIZE'] + r['TEMPLATERAMSIZE'] == r['HNDRTE_RECLAIM_0_END']
    return r

EXPECT = {
 '7_45_206/brcmfmac43455-sdio.bin': dict(
    WLC_UCODE_WRITE_BL_HOOK_ADDR=0x211A4C, HNDRTE_RECLAIM_0_END=0x2307F0,
    UCODESTART=0x222ED8, UCODESIZE=0xD918, FP_DATA_END_PTR=0x2036B0,
    FP_CONFIG_BASE_PTR_1=0x20575C, FP_CONFIG_BASE_PTR_2=0x2059E0, FP_CONFIG_ORIGEND=0x199BF4),
 '7_45_234_4ca95bb_CY/cyfmac43455-sdio-standard.bin': dict(
    WLC_UCODE_WRITE_BL_HOOK_ADDR=0x215E58, HNDRTE_RECLAIM_0_END=0x2350F0,
    UCODESTART=0x2264C8, UCODESIZE=0xE348, TEMPLATERAMSTART=0x234810, TEMPLATERAMSIZE=0x8E0,
    FP_DATA_END_PTR=0x207B10, FP_CONFIG_BASE_PTR_1=0x209B84, FP_CONFIG_BASE_PTR_2=0x209E08,
    FP_CONFIG_ORIGEND=0x199BE8, VERSION_PTR_1=0x1A7EF8, DATE_PTR=0x1A7F04, TIME_PTR=0x1A7EF4),
 '7_45_241/cyfmac43455-sdio-standard.bin': dict(
    WLC_UCODE_WRITE_BL_HOOK_ADDR=0x213E84, HNDRTE_RECLAIM_0_END=0x2338A0,
    UCODESTART=0x2254C0, FP_DATA_END_PTR=0x205AC0,
    FP_CONFIG_BASE_PTR_1=0x207B70, FP_CONFIG_BASE_PTR_2=0x207DF4, FP_CONFIG_ORIGEND=0x199BF4),
}

def main():
    import os
    base = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
    for rel, exp in EXPECT.items():
        got = derive(os.path.join(base, rel))
        bad = {k: (hex(v), hex(got.get(k, -1))) for k, v in exp.items() if got.get(k) != v}
        print(('OK   ' if not bad else 'FAIL '), rel, bad or '')
        if bad: sys.exit(1)

    r = derive(os.path.join(base, '7_45_265-28bca26-CY/cyfmac43455-sdio-standard.bin'))
    print('\n# 7_45_265', r.pop('_VERSION'))
    for k, v in r.items(): print(f'{k}=0x{v:X}')

if __name__ == '__main__':
    main()
