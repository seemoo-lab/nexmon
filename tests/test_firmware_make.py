"""Exercise the firmware dependency graph with an isolated fake toolchain.

No firmware is installed, kernel module built, or build statistics transmitted.
"""

import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import textwrap
import unittest


ROOT = Path(__file__).resolve().parents[1]
MAKEFILE = ROOT / "patches/bcm43455c0/7_45_265-28bca26-CY/nexmon/Makefile"


@unittest.skipUnless(shutil.which("make") and shutil.which("gawk") and shutil.which("xxd"),
                     "requires make, gawk and xxd")
class FirmwareMakeTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(prefix="nexmon-make-")
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.build = self.root / "patch/target"
        self.build.mkdir(parents=True)
        shutil.copy2(MAKEFILE, self.build / "Makefile")
        self.write("patch/version.mk", "FW_PATH=$(NEXMON_ROOT)/firmwares\n")
        self.write("firmwares/definitions.mk", """
            RAM_FILE=firmware.bin
            RAMSTART=0x1000
            RAMSIZE=0x1000
            ROMSTART=0
            ROMSIZE=0x1000
            PATCHSTART=0x1800
            PATCHSIZE=0x100
            UCODESTART=0x1100
            FP_CONFIG_BASE=0x1700
            FP_CONFIG_SIZE=0x100
        """)
        for name in ("alpha", "beta", "version"):
            self.write(f"patch/target/src/{name}.c", f"/* {name} */\n")
        self.write("patches/common/common.c", "/* common */\n")
        self.write("firmwares/wrapper.c", "/* firmware wrapper */\n")
        self.write("firmwares/ucode.bin", "ucode\n")
        self.write("firmwares/templateram.bin", "template\n")
        self.write("patch/target/patch.ld", "/* fake linker input */\n")
        self.write("patches/common/header.mk", "all:\n\t@sleep 0.05\n\t@touch init.done\n")
        scripts = self.root / "buildtools/scripts"
        scripts.mkdir(parents=True)
        for script in ROOT.glob("buildtools/scripts/*.awk"):
            shutil.copy2(script, scripts / script.name)
        self.write("buildtools/scripts/getdefine.sh", "#!/bin/sh\necho 1\n", executable=True)
        self.write("buildtools/scripts/getsectionaddr.sh", """
            #!/bin/sh
            if [ ! -f "$2" ]; then
                touch section-before-link
                exit 1
            fi
            echo 0x1800
        """, executable=True)
        self.write("tool-gcc", f"#!{sys.executable}\n" + textwrap.dedent("""
            from pathlib import Path
            import sys
            import time

            assert Path('init.done').is_file(), 'compiler started before init'
            assert Path('BUILD_NUMBER').is_file(), 'missing build number'
            args = sys.argv[1:]
            output = Path(args[args.index('-o') + 1])
            source = Path(args[args.index('-c') + 1])
            pre = Path(next(a.split('=', 1)[1] for a in args
                            if a.startswith('-fplugin-arg-nexmon-prefile=')))
            obj = next(a.split('=', 1)[1] for a in args
                       if a.startswith('-fplugin-arg-nexmon-objfile='))
            with pre.open('a') as f:
                f.write(f'patch TARGETREGION {obj}\\n')
            time.sleep(0.03)
            if 'FAIL' in source.read_text():
                sys.exit(1)
            output.write_text(source.read_text())
            output.with_suffix('.d').write_text(f'{obj}: {source}\\n')
        """), executable=True)
        self.write("tool-ld", f"#!{sys.executable}\n" + textwrap.dedent("""
            from pathlib import Path
            import sys
            import time

            for name in ('memory.ld', 'nexmon.ld', 'flashpatches.ld'):
                assert (Path('gen') / name).is_file(), name
            time.sleep(0.1)
            Path(sys.argv[sys.argv.index('-o') + 1]).write_text('linked')
        """), executable=True)
        self.env = dict(os.environ, NEXMON_ROOT=str(self.root), NEXMON_SETUP_ENV="1")
        # Do not inherit parent make goals, jobserver descriptors, or overrides.
        for name in ("MAKEFLAGS", "MFLAGS", "MAKELEVEL", "MAKEOVERRIDES"):
            self.env.pop(name, None)

    def write(self, relative, content, executable=False):
        path = self.root / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(textwrap.dedent(content).lstrip("\n"))
        if executable:
            path.chmod(0o755)
        return path

    def make(self, *goals, freeze=True, success=True):
        command = ["make", "--no-print-directory", "-j8", "GIT_VERSION=test",
                   f"CC={self.root}/tool-", "CCPLUGIN=unused", "ZLIBFLATE=cat", "Q=@"]
        if freeze:
            command.append("NEXMON_FREEZE_BUILD_NUMBER=1")
        else:
            command.append("NEXMON_FREEZE_BUILD_NUMBER=")
        result = subprocess.run(command + list(goals), cwd=self.build, env=self.env,
                                text=True, capture_output=True, timeout=30)
        if success:
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        else:
            self.assertNotEqual(result.returncode, 0, result.stdout + result.stderr)
        return result

    def test_default_goal_with_existing_dependencies(self):
        self.write("patch/target/obj/alpha.d", "obj/alpha.o: src/alpha.c\n")
        result = self.make("-qp", "-rR", success=False)
        self.assertIn(".DEFAULT_GOAL := all\n", result.stdout)

    def test_parallel_clean_build_and_frozen_incremental_build(self):
        self.make("gen/nexmon.mk", "gen/flashpatches.mk")
        self.assertFalse((self.build / "section-before-link").exists())
        expected = {f"patch TARGETREGION obj/{name}.o" for name in
                    ("alpha", "beta", "version", "common", "wrapper",
                     "ucode_compressed", "templateram")}
        metadata = self.build / "gen/nexmon.pre"
        lines = metadata.read_text().splitlines()
        self.assertEqual(set(lines), expected)
        self.assertEqual(len(lines), len(expected))
        outputs = list((self.build / "obj").glob("*.o")) + [
            metadata, self.build / "gen/patch.elf", self.build / "gen/nexmon.mk"]
        before = {p: p.stat().st_mtime_ns for p in outputs}
        self.make("gen/nexmon.mk", "gen/flashpatches.mk")
        self.assertEqual(before, {p: p.stat().st_mtime_ns for p in outputs})

    def test_missing_metadata_and_incremental_recompile(self):
        self.make("gen/nexmon.mk")
        original = (self.build / "gen/nexmon.pre").read_bytes()
        # Migration from old builds: existing objects have no private metadata.
        for path in (self.build / "obj").glob("*.pre"):
            path.unlink()
        self.make("gen/nexmon.mk")
        self.assertEqual((self.build / "gen/nexmon.pre").read_bytes(), original)
        beta = self.build / "obj/beta.o"
        beta_mtime = beta.stat().st_mtime_ns
        self.write("patch/target/src/alpha.c", "/* changed */\n")
        self.make("gen/nexmon.mk")
        self.assertEqual(beta.stat().st_mtime_ns, beta_mtime)
        self.assertEqual((self.build / "gen/nexmon.pre").read_bytes(), original)
        self.assertIn("changed", (self.build / "obj/alpha.o").read_text())

    def test_compile_failure_does_not_publish_metadata(self):
        self.make("gen/nexmon.mk")
        metadata = self.build / "gen/nexmon.pre"
        before = metadata.read_bytes()
        self.write("patch/target/src/alpha.c", "FAIL\n")
        self.make("gen/nexmon.mk", success=False)
        self.assertEqual(metadata.read_bytes(), before)
        self.write("patch/target/src/alpha.c", "/* recovered */\n")
        self.make("gen/nexmon.mk")
        self.assertEqual(metadata.read_bytes(), before)

    def test_build_number_increments_once_without_touching_source(self):
        source = self.build / "src/version.c"
        before = source.stat().st_mtime_ns
        self.make("gen/nexmon.mk", freeze=False)
        self.assertEqual((self.build / "BUILD_NUMBER").read_text().strip(), "1")
        obj = self.build / "obj/version.o"
        first = obj.stat().st_mtime_ns
        self.make("gen/nexmon.mk", freeze=False)
        self.assertEqual((self.build / "BUILD_NUMBER").read_text().strip(), "2")
        self.assertGreater(obj.stat().st_mtime_ns, first)
        self.assertEqual(source.stat().st_mtime_ns, before)


if __name__ == "__main__":
    unittest.main()
