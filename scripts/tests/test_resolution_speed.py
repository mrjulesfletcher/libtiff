import os, sys
import subprocess
from pathlib import Path

SCRIPT = Path(__file__).resolve().parents[1] / "resolution_speed_test.py"

def test_speed_script_runs(tmp_path):
    result = subprocess.run(
        [sys.executable, str(SCRIPT), "--res", "64x64", "--frames", "2", "--outdir", str(tmp_path)],
        text=True,
        capture_output=True,
        check=True,
    )
    assert "64x64" in result.stdout
    assert "write_fps" in result.stdout
    assert "read_fps" in result.stdout
    assert "write_fps_64x64" in result.stdout
    assert "read_fps_64x64" in result.stdout
