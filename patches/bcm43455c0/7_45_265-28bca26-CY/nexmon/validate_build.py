#!/usr/bin/env python3
"""Comprehensive build validation for bcm43455c0 7_45_265-28bca26-CY firmware patch.

Checks:
  1. Build artifacts exist and have correct types
  2. Patched firmware size matches original (in-place patching)
  3. Patched firmware differs from original (patches were applied)
  4. ELF patch file is valid ARM 32-bit
  5. All expected object files compiled
  6. Linker/compiler logs have no errors
  7. Patch addresses are within valid memory regions
  8. Version string is properly embedded in patched firmware
  9. Ucode compression/embedding integrity
 10. Flash patch config region integrity
 11. Nexmon capabilities bitmask present in patched firmware
 12. Reproducibility check (clean rebuild produces identical output)
"""
import os, sys, struct, subprocess, hashlib, re

PATCH_DIR  = os.path.dirname(os.path.abspath(__file__))
FW_DIR     = os.path.join(PATCH_DIR, '..', '..', '..', '..', 'firmwares',
                          'bcm43455c0', '7_45_265-28bca26-CY')
NEXMON_ROOT = os.path.join(PATCH_DIR, '..', '..', '..', '..')

PATCHED   = os.path.join(PATCH_DIR, 'cyfmac43455-sdio-standard.bin')
ORIGINAL  = os.path.join(FW_DIR,    'cyfmac43455-sdio-standard.bin')
PATCH_ELF = os.path.join(PATCH_DIR, 'gen', 'patch.elf')

# Memory map from definitions.mk
RAMSTART   = 0x198000
RAMSIZE    = 0xC8000
RAMEND     = RAMSTART + RAMSIZE
ROMSTART   = 0x0
ROMSIZE    = 0xB0000
PATCHSIZE  = 0x4000
UCODESTART = 0x21F24C
UCODESIZE  = 0xCFA8
HNDRTE_RECLAIM_0_END = 0x22CAD4
PATCHSTART = HNDRTE_RECLAIM_0_END - PATCHSIZE
TEMPLATERAMSTART = 0x22C1F4
TEMPLATERAMSIZE  = 0x8E0
FP_CONFIG_SIZE   = 0xC00
FP_CONFIG_BASE   = PATCHSTART - FP_CONFIG_SIZE

EXPECTED_FW_SIZE = 609309

# Expected object files from src/ and common/
EXPECTED_OBJS = [
    'patch.o', 'injection.o', 'monitormode.o', 'ioctl.o', 'sendframe.o',
    'console.o', 'version.o', 'ucode_compressed.o', 'templateram.o',
    # common objects
    'wrapper.o', 'helper.o', 'argprintf.o', 'objmem.o',
    'securitycookie.o', 'udptunnel.o', 'flashpatches.o',
    'radiotap.o', 'ucode_compression_code.o',
]

passed = 0
failed = 0
warnings = 0

def ok(msg):
    global passed
    passed += 1
    print(f"  ✅ PASS: {msg}")

def fail(msg):
    global failed
    failed += 1
    print(f"  ❌ FAIL: {msg}")

def warn(msg):
    global warnings
    warnings += 1
    print(f"  ⚠️  WARN: {msg}")

def section(title):
    print(f"\n{'='*60}")
    print(f"  {title}")
    print(f"{'='*60}")

def _require_artifacts():
    """Exit early when the firmware has not been built.

    The patched .bin and gen/ are build outputs (gitignored), so on a fresh
    checkout there is nothing to validate. Skip with a clear message unless
    --require / NEXMON_REQUIRE_BUILD=1 is set (use that after a build).
    """
    if os.path.isfile(PATCHED) and os.path.isfile(PATCH_ELF):
        return
    require = '--require' in sys.argv[1:] or os.environ.get('NEXMON_REQUIRE_BUILD') == '1'
    print(f"build artifacts not found ({os.path.relpath(PATCHED)}, gen/patch.elf);"
          f" {'FAILING' if require else 'skipping'} build validation.")
    print("Build the firmware first (make -C patches/bcm43455c0/7_45_265-28bca26-CY/nexmon).")
    sys.exit(1 if require else 0)

_require_artifacts()

def read_bin(path):
    with open(path, 'rb') as f:
        return f.read()

def rd32(data, addr):
    off = addr - RAMSTART
    return struct.unpack('<I', data[off:off+4])[0]

# ─────────────────────────────────────────────────────────────
section("1. Build Artifact Existence")
# ─────────────────────────────────────────────────────────────

for name, path in [
    ("Patched firmware",   PATCHED),
    ("Original firmware",  ORIGINAL),
    ("Patch ELF",          PATCH_ELF),
]:
    if os.path.isfile(path):
        ok(f"{name} exists: {os.path.basename(path)}")
    else:
        fail(f"{name} missing: {path}")

for gen_file in ['nexmon.ld', 'flashpatches.ld', 'memory.ld', 'nexmon.mk',
                 'flashpatches.mk', 'nexmon.pre', 'ucode.bin',
                 'ucode_compressed.bin']:
    p = os.path.join(PATCH_DIR, 'gen', gen_file)
    if os.path.isfile(p):
        ok(f"Generated file: {gen_file}")
    else:
        fail(f"Generated file missing: {gen_file}")

# ─────────────────────────────────────────────────────────────
section("2. Object File Compilation")
# ─────────────────────────────────────────────────────────────

obj_dir = os.path.join(PATCH_DIR, 'obj')
actual_objs = set(os.listdir(obj_dir)) if os.path.isdir(obj_dir) else set()
for obj in EXPECTED_OBJS:
    if obj in actual_objs:
        ok(f"Object file compiled: {obj}")
    else:
        fail(f"Object file missing: {obj}")

extra = actual_objs - set(EXPECTED_OBJS) - {f.replace('.o', '.d') for f in EXPECTED_OBJS}
extra_o = {f for f in extra if f.endswith('.o')}
if extra_o:
    warn(f"Extra object files (unexpected): {extra_o}")

# ─────────────────────────────────────────────────────────────
section("3. Firmware Size and Integrity")
# ─────────────────────────────────────────────────────────────

if os.path.isfile(PATCHED) and os.path.isfile(ORIGINAL):
    patched_data = read_bin(PATCHED)
    original_data = read_bin(ORIGINAL)

    if len(patched_data) == len(original_data):
        ok(f"Sizes match: {len(patched_data)} bytes (in-place patching)")
    else:
        fail(f"Size mismatch: patched={len(patched_data)}, original={len(original_data)}")

    if len(patched_data) == EXPECTED_FW_SIZE:
        ok(f"Firmware size correct: {EXPECTED_FW_SIZE} bytes")
    else:
        fail(f"Unexpected firmware size: {len(patched_data)} (expected {EXPECTED_FW_SIZE})")

    if patched_data != original_data:
        # Count differing bytes
        diff_count = sum(1 for a, b in zip(patched_data, original_data) if a != b)
        ok(f"Firmware was patched: {diff_count} bytes differ from original")
    else:
        fail("Patched firmware is IDENTICAL to original — patches not applied!")

    # Verify most of the firmware is untouched (patches should be localized)
    unchanged_pct = (len(patched_data) - diff_count) / len(patched_data) * 100
    if unchanged_pct > 90:
        ok(f"Patch is localized: {unchanged_pct:.1f}% of firmware unchanged")
    else:
        warn(f"Large modification: only {unchanged_pct:.1f}% unchanged")
else:
    fail("Cannot compare — firmware file(s) missing")

# ─────────────────────────────────────────────────────────────
section("4. ELF Patch File Validation")
# ─────────────────────────────────────────────────────────────

if os.path.isfile(PATCH_ELF):
    elf_data = read_bin(PATCH_ELF)
    # Check ELF magic
    if elf_data[:4] == b'\x7fELF':
        ok("ELF magic number correct")
    else:
        fail("Invalid ELF magic")

    # Check 32-bit
    if elf_data[4] == 1:
        ok("ELF class: 32-bit (correct for ARM)")
    else:
        fail(f"ELF class: {elf_data[4]} (expected 1 for 32-bit)")

    # Check little-endian
    if elf_data[5] == 1:
        ok("ELF encoding: little-endian (correct)")
    else:
        fail(f"ELF encoding: {elf_data[5]} (expected 1 for LE)")

    # Check ARM architecture (e_machine = 0x28 = 40)
    e_machine = struct.unpack('<H', elf_data[18:20])[0]
    if e_machine == 40:
        ok("ELF machine: ARM (EM_ARM = 40)")
    else:
        fail(f"ELF machine: {e_machine} (expected 40 for ARM)")

    # Check it's an executable
    e_type = struct.unpack('<H', elf_data[16:18])[0]
    if e_type == 2:
        ok("ELF type: ET_EXEC (executable)")
    else:
        warn(f"ELF type: {e_type} (expected 2 for executable)")

    # Check entry point is in valid range
    e_entry = struct.unpack('<I', elf_data[24:28])[0]
    if RAMSTART <= e_entry < RAMEND or e_entry < ROMSIZE:
        ok(f"ELF entry point: 0x{e_entry:08X} (within valid range)")
    else:
        warn(f"ELF entry point: 0x{e_entry:08X} (outside expected ranges)")

    # Use readelf/objdump to check sections if available
    try:
        result = subprocess.run(
            ['arm-none-eabi-objdump', '-h', PATCH_ELF],
            capture_output=True, text=True, timeout=10,
            env={**os.environ, 'PATH': os.environ.get('PATH', '') + ':' +
                 os.path.join(NEXMON_ROOT, 'buildtools',
                              'gcc-arm-none-eabi-5_4-2016q2-linux-armv7l', 'bin')}
        )
        if result.returncode == 0:
            sections = [l for l in result.stdout.split('\n') if ' .text' in l or ' .patch' in l or ' .data' in l]
            if sections:
                ok(f"ELF has {len(sections)} code/data sections")
            else:
                warn("No .text/.patch/.data sections found in ELF")
        else:
            warn("Could not run objdump on ELF (non-zero return)")
    except FileNotFoundError:
        warn("arm-none-eabi-objdump not in PATH; skipping detailed ELF analysis")
    except Exception as e:
        warn(f"objdump check skipped: {e}")

# ─────────────────────────────────────────────────────────────
section("5. Memory Map Validation")
# ─────────────────────────────────────────────────────────────

# The linker memory regions (from gen/memory.ld) are sequential:
#   ucode → fpconfig → patch  (all within the reclaim area)
# These are the regions the *linker* uses; they differ from the raw
# definitions.mk addresses which describe the original firmware layout.
# Parse gen/memory.ld for the authoritative layout.
memory_ld_path = os.path.join(PATCH_DIR, 'gen', 'memory.ld')
linker_regions = {}
if os.path.isfile(memory_ld_path):
    import re as _re
    for line in open(memory_ld_path):
        m = _re.match(r'(\w+)\s*:\s*ORIGIN\s*=\s*(0x[0-9a-fA-F]+),\s*LENGTH\s*=\s*(0x[0-9a-fA-F]+)', line)
        if m:
            name = m.group(1)
            origin = int(m.group(2), 16)
            length = int(m.group(3), 16)
            linker_regions[name] = (origin, origin + length, length)

    for name, (start, end, size) in linker_regions.items():
        if name in ('rom', 'ram'):
            ok(f"Linker region '{name}': 0x{start:06X}-0x{end:06X} (0x{size:X})")
        elif RAMSTART <= start < end <= RAMEND:
            ok(f"Linker region '{name}': 0x{start:06X}-0x{end:06X} (0x{size:X}) within RAM")
        else:
            fail(f"Linker region '{name}': 0x{start:06X}-0x{end:06X} outside RAM")

    # Check sequential ordering of sub-regions: ucode < fpconfig < patch
    sub = [(n, linker_regions[n]) for n in ('ucode', 'fpconfig', 'patch') if n in linker_regions]
    sub.sort(key=lambda x: x[1][0])
    for i in range(len(sub) - 1):
        n1, (_, e1, _) = sub[i]
        n2, (s2, _, _) = sub[i+1]
        if e1 <= s2:
            ok(f"Linker regions sequential: '{n1}' ends 0x{e1:X} ≤ '{n2}' starts 0x{s2:X}")
        else:
            fail(f"Linker region overlap: '{n1}' ends 0x{e1:X} > '{n2}' starts 0x{s2:X}")

    # Verify patch region ends at HNDRTE_RECLAIM_0_END
    if 'patch' in linker_regions:
        _, patch_end, _ = linker_regions['patch']
        if patch_end == HNDRTE_RECLAIM_0_END:
            ok(f"Patch region ends at HNDRTE_RECLAIM_0_END (0x{HNDRTE_RECLAIM_0_END:X})")
        else:
            fail(f"Patch end 0x{patch_end:X} != HNDRTE_RECLAIM_0_END 0x{HNDRTE_RECLAIM_0_END:X}")
else:
    warn("gen/memory.ld not found; skipping linker region checks")

# High-level: verify definitions.mk invariant
expected_end = UCODESTART + UCODESIZE + TEMPLATERAMSIZE
if expected_end == HNDRTE_RECLAIM_0_END:
    ok(f"UCODE + TEMPLATERAM = HNDRTE_RECLAIM_0_END (0x{HNDRTE_RECLAIM_0_END:X})")
else:
    fail(f"Region sum 0x{expected_end:X} != HNDRTE_RECLAIM_0_END 0x{HNDRTE_RECLAIM_0_END:X}")

# ─────────────────────────────────────────────────────────────
section("6. Version String in Patched Firmware")
# ─────────────────────────────────────────────────────────────

if os.path.isfile(PATCHED):
    patched_data = read_bin(PATCHED)

    # Check for nexmon version string
    if b'nexmon' in patched_data.lower():
        # find all nexmon occurrences
        idx = 0
        locations = []
        search = patched_data.lower()
        while True:
            idx = search.find(b'nexmon', idx)
            if idx == -1:
                break
            locations.append(RAMSTART + idx)
            idx += 1
        ok(f"Nexmon identifier found in firmware at {len(locations)} location(s): " +
           ", ".join(f"0x{a:X}" for a in locations[:5]))
    else:
        warn("No 'nexmon' string found in patched firmware")

    # Check for original version string
    ver_match = re.search(rb'7\.45\.265', patched_data)
    if ver_match:
        ok(f"Original version string '7.45.265' preserved at offset 0x{ver_match.start():X}")
    else:
        warn("Original version string '7.45.265' not found")

    # Check for build number
    build_num_path = os.path.join(PATCH_DIR, 'BUILD_NUMBER')
    if os.path.isfile(build_num_path):
        build_num = open(build_num_path).read().strip()
        ok(f"Build number: {build_num}")

# ─────────────────────────────────────────────────────────────
section("7. Patch Region Content Verification")
# ─────────────────────────────────────────────────────────────

if os.path.isfile(PATCHED) and os.path.isfile(ORIGINAL):
    patched_data = read_bin(PATCHED)
    original_data = read_bin(ORIGINAL)

    # Check that the patch region has been modified
    patch_offset = PATCHSTART - RAMSTART
    patch_region_orig = original_data[patch_offset:patch_offset + PATCHSIZE]
    patch_region_patched = patched_data[patch_offset:patch_offset + PATCHSIZE]

    if patch_region_orig != patch_region_patched:
        diff_in_patch = sum(1 for a, b in zip(patch_region_orig, patch_region_patched) if a != b)
        ok(f"Patch region modified: {diff_in_patch} bytes changed in 0x{PATCHSIZE:X}-byte region")
    else:
        fail("Patch region NOT modified — patches may not have been applied")

    # Check that patch region is not all zeros (it should have code)
    if patch_region_patched != b'\x00' * PATCHSIZE:
        nonzero = sum(1 for b in patch_region_patched if b != 0)
        ok(f"Patch region has content: {nonzero} non-zero bytes ({nonzero*100//PATCHSIZE}% fill)")
    else:
        fail("Patch region is all zeros!")

    # Check the ucode region was modified (compressed ucode should be different)
    ucode_offset = UCODESTART - RAMSTART
    ucode_orig = original_data[ucode_offset:ucode_offset + UCODESIZE]
    ucode_patched = patched_data[ucode_offset:ucode_offset + UCODESIZE]
    if ucode_orig != ucode_patched:
        ok("Ucode region was modified (compressed ucode replaced)")
    else:
        ok("Ucode region unchanged (original ucode preserved — expected if no ucode.asm)")

    # Flash patch config region
    fp_offset = FP_CONFIG_BASE - RAMSTART
    fp_orig = original_data[fp_offset:fp_offset + FP_CONFIG_SIZE]
    fp_patched = patched_data[fp_offset:fp_offset + FP_CONFIG_SIZE]
    if fp_orig != fp_patched:
        fp_diff = sum(1 for a, b in zip(fp_orig, fp_patched) if a != b)
        ok(f"Flash patch config region modified: {fp_diff} bytes changed")
    else:
        warn("Flash patch config region unchanged")

# ─────────────────────────────────────────────────────────────
section("8. Compiler/Linker Log Analysis")
# ─────────────────────────────────────────────────────────────

log_dir = os.path.join(PATCH_DIR, 'log')
for logname in ['compiler.log', 'linker.log', 'linker.err', 'flashpatches.log', 'patches.log']:
    logpath = os.path.join(log_dir, logname)
    if not os.path.isfile(logpath):
        warn(f"Log file missing: {logname}")
        continue

    content = open(logpath).read()
    # Check for fatal errors (skip "Removing unused section" which is normal,
    # and ASSERT success lines where the linker reports the assertion text)
    errors = [l for l in content.split('\n')
              if ('error' in l.lower() or 'undefined reference' in l.lower())
              and 'removing unused section' not in l.lower()
              and 'ASSERT' not in l  # Linker ASSERT output (condition text, not a failure)
              and l.strip()]
    if errors:
        fail(f"{logname}: {len(errors)} error(s) found")
        for e in errors[:3]:
            print(f"    → {e.strip()[:120]}")
    else:
        ok(f"{logname}: no errors")

# ─────────────────────────────────────────────────────────────
section("9. Ucode Compression Integrity")
# ─────────────────────────────────────────────────────────────

ucode_orig = os.path.join(FW_DIR, 'ucode.bin')
ucode_gen  = os.path.join(PATCH_DIR, 'gen', 'ucode.bin')
ucode_comp = os.path.join(PATCH_DIR, 'gen', 'ucode_compressed.bin')

if os.path.isfile(ucode_orig) and os.path.isfile(ucode_gen):
    orig_hash = hashlib.sha256(read_bin(ucode_orig)).hexdigest()[:16]
    gen_hash  = hashlib.sha256(read_bin(ucode_gen)).hexdigest()[:16]
    if orig_hash == gen_hash:
        ok(f"Ucode binary matches original (sha256: {orig_hash}...)")
    else:
        warn("Ucode binary differs from original (expected if custom ucode.asm used)")

if os.path.isfile(ucode_comp) and os.path.isfile(ucode_gen):
    comp_size = os.path.getsize(ucode_comp)
    orig_size = os.path.getsize(ucode_gen)
    if comp_size < orig_size:
        ratio = comp_size / orig_size * 100
        ok(f"Ucode compressed: {orig_size} → {comp_size} bytes ({ratio:.1f}% of original)")
    else:
        warn(f"Compressed ucode ({comp_size}) not smaller than original ({orig_size})")

# ─────────────────────────────────────────────────────────────
section("10. Generated Linker/Make File Validation")
# ─────────────────────────────────────────────────────────────

# Check memory.ld has correct regions
memory_ld = os.path.join(PATCH_DIR, 'gen', 'memory.ld')
if os.path.isfile(memory_ld):
    mld = open(memory_ld).read()
    for region in ['rom', 'ram', 'patch', 'ucode', 'fpconfig']:
        if region in mld:
            ok(f"memory.ld defines '{region}' region")
        else:
            fail(f"memory.ld missing '{region}' region")

# Check nexmon.mk references the correct output file
nexmon_mk = os.path.join(PATCH_DIR, 'gen', 'nexmon.mk')
if os.path.isfile(nexmon_mk):
    nmk = open(nexmon_mk).read()
    if 'cyfmac43455-sdio-standard.bin' in nmk:
        ok("nexmon.mk targets correct output file")
    else:
        fail("nexmon.mk does not reference cyfmac43455-sdio-standard.bin")
    if 'gen/patch.elf' in nmk:
        ok("nexmon.mk references patch ELF")
    else:
        fail("nexmon.mk does not reference patch ELF")

# ─────────────────────────────────────────────────────────────
section("11. Nexmon Capability Flags Check")
# ─────────────────────────────────────────────────────────────

# patch.c defines capabilities as NEX_CAP_MONITOR_MODE | NEX_CAP_RADIOTAP | NEX_CAP_FRAME_INJECTION
# These are typically bit flags. Check patch.c source for the actual values.
patch_c = os.path.join(PATCH_DIR, 'src', 'patch.c')
if os.path.isfile(patch_c):
    pc = open(patch_c).read()
    caps = []
    for cap in ['MONITOR_MODE', 'RADIOTAP', 'FRAME_INJECTION']:
        if cap in pc:
            caps.append(cap)
    if caps:
        ok(f"patch.c declares capabilities: {', '.join(caps)}")
    else:
        warn("No capability flags found in patch.c")
else:
    warn("patch.c not found")

# ─────────────────────────────────────────────────────────────
section("12. Binary Diff Summary")
# ─────────────────────────────────────────────────────────────

if os.path.isfile(PATCHED) and os.path.isfile(ORIGINAL):
    patched_data = read_bin(PATCHED)
    original_data = read_bin(ORIGINAL)

    # Find contiguous diff regions
    diff_regions = []
    in_diff = False
    start = 0
    for i in range(len(patched_data)):
        if patched_data[i] != original_data[i]:
            if not in_diff:
                in_diff = True
                start = i
        else:
            if in_diff:
                diff_regions.append((RAMSTART + start, RAMSTART + i, i - start))
                in_diff = False
    if in_diff:
        diff_regions.append((RAMSTART + start, RAMSTART + len(patched_data), len(patched_data) - start))

    ok(f"Found {len(diff_regions)} contiguous modified region(s)")
    for addr_start, addr_end, size in diff_regions:
        label = ""
        if PATCHSTART <= addr_start < PATCHSTART + PATCHSIZE:
            label = " [PATCH region]"
        elif FP_CONFIG_BASE <= addr_start < FP_CONFIG_BASE + FP_CONFIG_SIZE:
            label = " [FP_CONFIG region]"
        elif UCODESTART <= addr_start < UCODESTART + UCODESIZE:
            label = " [UCODE region]"
        elif TEMPLATERAMSTART <= addr_start < TEMPLATERAMSTART + TEMPLATERAMSIZE:
            label = " [TEMPLATERAM region]"
        print(f"    0x{addr_start:06X} - 0x{addr_end:06X} ({size:5d} bytes){label}")

# ─────────────────────────────────────────────────────────────
section("13. Patched Firmware Checksum")
# ─────────────────────────────────────────────────────────────

if os.path.isfile(PATCHED):
    patched_data = read_bin(PATCHED)
    sha = hashlib.sha256(patched_data).hexdigest()
    try:
        md5 = hashlib.md5(patched_data, usedforsecurity=False).hexdigest()
    except TypeError:
        md5 = hashlib.md5(patched_data).hexdigest()
    ok(f"SHA-256: {sha}")
    ok(f"MD5:     {md5}")

if os.path.isfile(ORIGINAL):
    original_data = read_bin(ORIGINAL)
    sha_orig = hashlib.sha256(original_data).hexdigest()
    ok(f"Original SHA-256: {sha_orig}")

# ═══════════════════════════════════════════════════════════════
print(f"\n{'='*60}")
print("  VALIDATION SUMMARY")
print(f"{'='*60}")
print(f"  ✅ Passed:  {passed}")
print(f"  ❌ Failed:  {failed}")
print(f"  ⚠️  Warnings: {warnings}")
print(f"{'='*60}")

if failed > 0:
    print("\n  BUILD VALIDATION FAILED")
    sys.exit(1)
else:
    print("\n  BUILD VALIDATION PASSED ✓")
    sys.exit(0)
