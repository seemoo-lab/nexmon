Ported. See `patches/bcm43455c0/7_45_265-28bca26-CY/`.

- Source: `RPi-Distro/firmware-nonfree`, `debian/added-firmware/cypress/cyfmac43455-sdio-standard.bin`
- Internal chip string: `43455c0-roml/...`
- Version: `7.45.265 (28bca26 CY)`, CRC `68bafb8c`
- Date: Tue 2023-08-29 01:51:02 PDT
- Fetched: 2026-08-20

`definitions.mk` was derived, not hand-copied, by `derive.py` in this directory, which
self-tests against `7_45_206` / `7_45_234_4ca95bb_CY` / `7_45_241` before printing this
version's values. Feature string is closest to `7_45_234_4ca95bb_CY` (both have
`gtkoe-roamprof-txbf-ve-*sae-dpp-sr-okc-bpd`), so all byte-signature relocation for the
patch and `wrapper.c` entries was done `234_CY -> 265`, not from `7_45_241` which is a
different (minimal, no-gtkoe) build lineage despite the numerically closer version.

See `REVERSE_ENGINEERING_NOTES.md`, section "bcm43455c0 / 7.45.265 port", for the full
derivation trail, confidence levels, and the wrapper-audit results.
