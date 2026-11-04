# MPFR and ISL

[![Check vendored updates](https://github.com/seemoo-lab/nexmon/actions/workflows/check-updates.yml/badge.svg)](https://github.com/seemoo-lab/nexmon/actions/workflows/check-updates.yml)

The current vendored releases are **MPFR 4.2.2** and **ISL 0.28**. `make update`
reads their source versions and checks the official release directories.

## Automated update checks
Weekly CI via [`.github/workflows/check-updates.yml`](../.github/workflows/check-updates.yml) runs `python3 buildtools/scripts/check_updates.py --format json` / `--check-only` and uploads `updates.json` (badge above). Run locally:

```sh
make update
# or
python3 buildtools/scripts/check_updates.py --check-only  # exits 1 if updates available
```

## Source provenance

These directories contain the unmodified upstream release archives, including
their build scripts, tests and license files.

| Source directory | Archive | SHA-256 |
| --- | --- | --- |
| `mpfr-4.2.2/` | [mpfr-4.2.2.tar.xz](https://ftp.gnu.org/gnu/mpfr/mpfr-4.2.2.tar.xz) | `b67ba0383ef7e8a8563734e2e889ef5ec3c3b898a01d00fa0a6869ad81c6ce01` |
| `isl-0.28/` | [isl-0.28.tar.xz](https://libisl.sourceforge.io/isl-0.28.tar.xz) | `3dc31b8e1b18329e42d5dfbf84dd55e15c59b61569a2ab246f61497d9592f727` |

## Building the current libraries

Both releases require a native C compiler, make and GMP development headers;
ISL also uses a C++ compiler for its C++ tests. Release archives already include
`configure`, so regenerating Autotools files is unnecessary.

From the Nexmon repository root, build and test outside the source directories:

```sh
nexmon_source="$PWD"
nexmon_math_build=$(mktemp -d /tmp/nexmon-math-build-XXXXXX)
mkdir -p "$nexmon_math_build/install"
for library in isl-0.28 mpfr-4.2.2; do
    mkdir -p "$nexmon_math_build/$library"
    (
        cd "$nexmon_math_build/$library" &&
        "$nexmon_source/buildtools/$library/configure" \
            --prefix="$nexmon_math_build/install" &&
        make -j2 && make check && make install
    ) || break
done
```

The installation stays in the temporary build directory. Like the older math
sources, these libraries are not built by the default `make` target. Building
them does not change which runtime libraries the bundled firmware compiler uses.

## GCC 5 compatibility sources

The bundled `gcc-arm-none-eabi-5_4-2016q2` compiler and Nexmon GCC plugin still
use the GCC 5.4.1 toolchain. Its ARM32 `cc1` binary requires the following library
ABIs, which differ from the current releases:

| Library | GCC 5 requires | Current release provides | Preserved compatibility source |
| --- | --- | --- | --- |
| ISL | `libisl.so.10` | `libisl.so.23` | `legacy/isl-0.10/` |
| MPFR | `libmpfr.so.4` | `libmpfr.so.6` | `legacy/mpfr-3.1.4/` |

The previous vendored source trees were moved unchanged into `legacy/` so the
older runtime libraries remain reproducible. They are compatibility dependencies,
not the current versions reported by `make update`.

For ARM hosts, `setup_env.sh` uses `buildtools/armhf-compat-libs/` when that local
directory exists. Its libraries must be ARM32/armhf even on an AArch64 host,
because the bundled compiler is an ARM32 executable. Existing compatibility
binaries are not replaced by this source update. For a fresh installation, obtain
the matching armhf library packages or build the preserved sources with an armhf
compiler and GMP development files. Building with an AArch64 compiler produces
libraries that the ARM32 compiler cannot load.

Do not alias current `libisl.so.23` or `libmpfr.so.6` to the older SONAMEs.
Changing a symlink does not restore the old ABI. Updating the firmware compiler's
runtime dependencies requires a separate compiler/plugin compatibility migration.
