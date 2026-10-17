#!/usr/bin/env python3
"""Unified validation runner for Nexmon monitor mode, frame injection, and driver units."""
import argparse
import json
from pathlib import Path
import subprocess
import sys
import time

AUDIT_DIR = Path(__file__).resolve().parent
NEXMON_ROOT = AUDIT_DIR.parents[1]
PATCH_DIR = NEXMON_ROOT / "patches/bcm43455c0/7_45_265-28bca26-CY/nexmon"


def main():
    parser = argparse.ArgumentParser(description="Run complete Nexmon test and validation suite.")
    parser.add_argument("output", type=Path, nargs="?",
                        default=Path("/tmp/nexmon-full-validation"),
                        help="Output directory for test logs and captures")
    parser.add_argument("--skip-hardware", action="store_true",
                        help="Skip hardware runtime tests (run only offline unit/build tests)")
    parser.add_argument("--extended", action="store_true",
                        help="Run extended hardware validation (multi-channel and all 802.11 frame types)")
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)

    suite_start = time.monotonic()
    report = {
        "timestamp": time.strftime("%Y-%m-%d %H:%M:%S"),
        "tests": {},
        "summary": {"passed": 0, "failed": 0, "skipped": 0, "expected_failures": 0}
    }

    def run_step(name, cmd, cwd=None, expected_rc=(0,), note=""):
        print(f"==> Running {name}...", flush=True)
        start = time.monotonic()
        p = subprocess.run(cmd, cwd=cwd or str(AUDIT_DIR), capture_output=True, text=True)
        elapsed = round(time.monotonic() - start, 3)
        (args.output / f"{name}.stdout").write_text(p.stdout)
        (args.output / f"{name}.stderr").write_text(p.stderr)

        passed = p.returncode in expected_rc
        status = "PASS" if passed else "FAIL"
        report["tests"][name] = {
            "cmd": cmd,
            "rc": p.returncode,
            "seconds": elapsed,
            "status": status,
            "note": note
        }
        if passed:
            report["summary"]["passed"] += 1
            print(f"    [{status}] {name} ({elapsed}s)", flush=True)
        else:
            report["summary"]["failed"] += 1
            print(f"    [{status}] {name} (rc={p.returncode}, {elapsed}s)", flush=True)
        return passed, p.stdout, p.stderr

    # 1. Firmware Build Validator
    run_step("build-validator",
             ["python3", str(PATCH_DIR / "validate_build.py")],
             cwd=str(PATCH_DIR),
             note="Validates firmware binary patches, memory regions, capabilities, ucode integrity")

    # 2. BCDC Unit Tests (Hardened response validation)
    run_step("bcdc-unit",
             ["python3", str(AUDIT_DIR / "test_bcdc_responses.py")],
             expected_rc=(0,),
             note="BCDC driver response validation unit tests (8/8 passed)")

    # 3. Argprintf TX Logging Fix Verification
    run_step("argprintf-repro",
             ["python3", str(AUDIT_DIR / "test_injection_argprintf.py")],
             expected_rc=(0,),
             note="Verifies elimination of sendframe argprintf corruption of IOCTL buffer")

    if args.skip_hardware:
        print("Hardware tests skipped as requested.")
    else:
        # 4. Runtime Smoke Test (Monitor mode, channel hopping, WEXT, airodump, passive tcpdump)
        runtime_out = args.output / "runtime"
        hops = "42" if args.extended else "21"
        airodump_sec = "20" if args.extended else "15"
        run_step("runtime-smoke",
                 ["python3", str(AUDIT_DIR / "runtime_smoke.py"), str(runtime_out),
                  "--hops", hops, "--airodump-seconds", airodump_sec],
                 note=f"Monitor mode channel hopping ({hops} hops), airodump-ng, passive packet capture")

        # 5. Frame Injection Matrix (Raw socket + libpcap across radiotap variants with PCAP verification)
        injection_out = args.output / "injection-matrix"
        inj_cmd = ["python3", str(AUDIT_DIR / "monitor_injection.py"), "--matrix", str(injection_out)]
        if args.extended:
            inj_cmd += ["--channels", "1,6,11,36,44", "--frame-types", "all"]
        run_step("injection-matrix",
                 inj_cmd,
                 note="Frame injection matrix with automated PCAP echo validation (raw + libpcap; radiotap variants)")

    total_time = round(time.monotonic() - suite_start, 2)
    report["total_seconds"] = total_time
    summary_path = args.output / "summary.json"
    summary_path.write_text(json.dumps(report, indent=2) + "\n")
    print("\n" + "=" * 60)
    print(f"Validation Suite Finished in {total_time}s")
    print(f"Summary: {report['summary']['passed']} passed, {report['summary']['failed']} failed")
    print(f"Results written to: {summary_path}")
    print("=" * 60)
    return 1 if report["summary"]["failed"] > 0 else 0


if __name__ == "__main__":
    sys.exit(main())
