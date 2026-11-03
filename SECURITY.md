# Security Policy

## Supported Versions
| Version | Supported |
|---------|-----------|
| `master` (latest) | ✅ |
| Firmwares `7_45_265` / `7_45_234` / `7_45_206` / `7_45_189` | ✅ (active) |
| Older `firmwares/*` and `patches/*` | ⚠️ Community / best-effort |
| Vendored `utilities/*` (e.g., `b43`, `libnl`, `zlib`) and `buildtools/isl-*` | Upstream-tracked via `buildtools/scripts/check_updates.py`; see below |

## Reporting a Vulnerability
Please **do not** open a public issue. Contact us via **[nexmon@seemoo.de](mailto:nexmon@seemoo.de)** (SEEMOO Lab, TU Darmstadt) with:

- Affected chip/firmware/commit (`git describe --dirty --always --tags`)
- Reproduction steps, `dmesg`/`nexmon_crash_decoder.py` output, and `monitor-mode.sh` / `nexfirmware.py` logs if relevant
- Impact assessment (e.g., firmware crash, host DoS, information leak)

We acknowledge receipt within 3 business days and aim to provide an initial assessment within 14 days. Coordinated disclosure after a fix is available; we request 90 days before public disclosure.

## Disclosure Process
1. Reporter contacts `nexmon@seemoo.de` with `Subject: [nexmon security]`.
2. SEEMOO confirms, reproduces, and assigns a case ID.
3. Fix developed on a private branch, tested on `bcm43455c0` and at least one additional target.
4. Fix merged to `master`, credited in release notes (unless reporter requests anonymity).
5. CVE requested if applicable.

## Hardening Notes
- `brcmfmac` kernel drivers enforce strict Netlink and BCDC bounds checking, rejecting out-of-bounds payloads and validating command ranges.
- `monitor-mode.sh` uses `flock` on `/run/nexmon-monitor.state` (0600), creates state files with `umask 077`, validates `STATE_FILE` to avoid TOCTOU, and protects existing sessions on rerun.
- `nexfirmware.py` validates `put --name` basename, uses atomic `mkstemp`+`fsync`+`rename` across writes/edits, dynamically derives RAM/ROM offsets, and blocks path traversal in `pull`.
- `utilities/rawproxy` / `rawproxyreverse` serialize a standard 16-byte `pcap_pkthdr32` wire format across LP64 and ILP32 platforms, validate `port` with `strtol` range `1..65535`, use `SO_REUSEADDR`, and detect truncation.
- `libnexio` handles Netlink errors with `nl_cb_err`, bounds buffer allocations with `nlmsg_alloc_size`, rejects NULL buffers, binds `nl_pid = 0`, and uses `SOCK_CLOEXEC`/`FD_CLOEXEC`.
- Android pentest suite JNI helpers enforce 64-bit native pointer handles (`jlong`), release `GetStringUTFChars` across all early-exit paths, and eliminate memory corruption in packet processing.
- CI runs `ruff` with `extend-select = ["S","B9"]` (bandit), `shellcheck -S error`, and native AddressSanitizer/UndefinedBehaviorSanitizer test suites; see `pyproject.toml`.

## Checking Vendored Dependencies
`buildtools/scripts/check_updates.py` tracks `utilities/*` and `buildtools/*` against upstream. CI runs it weekly (see `.github/workflows/ci.yml` `check_updates` job). To check locally:

```sh
python3 buildtools/scripts/check_updates.py --category all
python3 buildtools/scripts/check_updates.py --check-only  # exits 1 if updates available
```

## Reporting security issues
Please contact us via **[nexmon@seemoo.de](mailto:nexmon@seemoo.de)** to report suspected security vulnerabilities.
