"""Regression tests for monitor-mode daemon state bookkeeping.

The tests evaluate only the two state helper functions with a fake systemctl;
they never import or execute the script's radio setup path.
"""

import os
from pathlib import Path
import subprocess
import tempfile
import textwrap
import unittest


ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "monitor-mode.sh"


def shell_function(name: str) -> str:
    """Extract one top-level shell function from monitor-mode.sh."""
    lines = SCRIPT.read_text().splitlines(keepends=True)
    start = next(i for i, line in enumerate(lines) if line == f"{name}() {{\n")
    end = next(i for i in range(start + 1, len(lines)) if lines[i] == "}\n")
    return "".join(lines[start : end + 1])


class MonitorDaemonStateTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(prefix="nexmon-monitor-state-")
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.bin_dir = self.root / "bin"
        self.bin_dir.mkdir()
        systemctl = self.bin_dir / "systemctl"
        systemctl.write_text(textwrap.dedent("""\
            #!/bin/sh
            case "$1:$2:$3" in
                is-active:--quiet:wpa_supplicant)
                    test -f "$SYSTEMCTL_STATE/active"
                    ;;
                stop:wpa_supplicant:)
                    printf '%s\n' stop >> "$SYSTEMCTL_STATE/calls"
                    rm -f "$SYSTEMCTL_STATE/active"
                    ;;
                start:wpa_supplicant:)
                    printf '%s\n' start >> "$SYSTEMCTL_STATE/calls"
                    test "${SYSTEMCTL_FAIL_START:-0}" != 1 || exit 1
                    touch "$SYSTEMCTL_STATE/active"
                    ;;
                *) exit 1 ;;
            esac
        """))
        systemctl.chmod(0o755)
        self.env = os.environ.copy()
        self.env.update({
            "PATH": f"{self.bin_dir}:{self.env['PATH']}",
            "SYSTEMCTL_STATE": str(self.root),
            "STATE_FILE": str(self.root / "monitor.state"),
        })
        self.functions = shell_function("quiesce_daemons") + shell_function("restore_daemons")

    def run_helpers(self, commands: str, *, success: bool = True, fail_start: bool = False):
        env = self.env.copy()
        if fail_start:
            env["SYSTEMCTL_FAIL_START"] = "1"
        script = textwrap.dedent(f"""\
            set -u
            QUIESCE=1
            STATE_FILE="$STATE_FILE"
            info() {{ :; }}
            warn() {{ :; }}
            {self.functions}
            {commands}
        """)
        result = subprocess.run(["bash", "-c", script], env=env, text=True,
                                capture_output=True, timeout=10)
        if success:
            self.assertEqual(result.returncode, 0, result.stderr)
        else:
            self.assertNotEqual(result.returncode, 0, result.stderr)
        return result

    def test_repeated_quiesce_preserves_original_restart_record(self):
        (self.root / "active").touch()
        self.run_helpers("quiesce_daemons; quiesce_daemons; restore_daemons")

        self.assertTrue((self.root / "active").exists())
        self.assertFalse((self.root / "monitor.state").exists())
        self.assertEqual((self.root / "calls").read_text().splitlines(), ["stop", "start"])

    def test_failed_restore_retains_state_for_retry(self):
        (self.root / "active").touch()
        self.run_helpers("quiesce_daemons; restore_daemons", success=False, fail_start=True)

        self.assertEqual((self.root / "monitor.state").read_text(), "wpa_supplicant\n")
        self.run_helpers("restore_daemons")
        self.assertTrue((self.root / "active").exists())
        self.assertFalse((self.root / "monitor.state").exists())


if __name__ == "__main__":
    unittest.main()
