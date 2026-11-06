# Prebuilt own-code firmware patch

`firmware.patch.bin` is a ready-to-apply, GPL-3.0-only binary payload generated
from this contribution's C/assembly source build. It contains 1,556 bytes across
11 replacement ranges: custom code and branch/pointer edits. The accompanying
JSON records file offsets, lengths and hashes. It contains no unchanged stock
firmware, ROM dump, SDK, packet capture or private network data.

Apply it offline to your own exact stock firmware using Python 3.9+ only:

```sh
python3 -B apply_prebuilt.py --stock "$NEXMON_STOCK" --output "$NEXMON_WORK/candidate.trx"
```

Run that command from the parent contribution directory. The output directory
must already exist; the output file must not exist. The utility never overwrites
the stock image, loads firmware, contacts a phone or changes Wi-Fi settings.
It checks the exact stock hash, payload hash, range bounds/non-overlap and final
image hash before exclusively creating the output.

Stock SHA256:
`280bc2d7ad25f2c789ce332c37da9d687693fbda156830803fe1516e8d938244`

Result SHA256 (527,400 bytes):
`0245b815b9ff0a5d7e7a3b4f42b491338d2dbeefd8f5bf66cf8a86a83fb00f1f`

This route needs no Zig, Capstone or Keystone. The independent source-build
route remains in [REPRODUCING.md](../REPRODUCING.md), including native-helper
build/sign/trust requirements and exact-target operating limits. These helpers
are still built from source; this payload is not a ready-to-install application.
No ROM dump is required for either fixed-profile firmware build route.

Validation: applying this payload to the exact owner's stock image reproduced
the already live-tested candidate byte for byte. No new RF experiment was
performed for this packaging change. Run `python3 -B test_prebuilt.py` in the
parent directory for synthetic corruption, overlap and boundary tests.
