import sys
import subprocess
import shutil
from pathlib import Path

SCRIPT = Path(__file__).resolve().parents[1] / "raw_to_dng_benchmark.py"


def test_raw_to_dng_script_runs(tmp_path):
    tool = shutil.which("true") or "true"
    result = subprocess.run(
        [sys.executable, str(SCRIPT), "--raw2tiff", tool, "--frames", "1", "--outdir", str(tmp_path)],
        text=True,
        capture_output=True,
        check=True,
    )
    assert "convert_fps" in result.stdout
