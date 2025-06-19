import os, sys; sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..")))
import pytest
from scripts.run_all_benchmarks import parse_results


def test_parse_numeric_results():
    output = "TIFFSwabArrayOfShort: 0.181 ms\nscalar_swab_short: 0.158 ms"
    res = parse_results(output)
    assert res == {
        "TIFFSwabArrayOfShort (ms)": 0.181,
        "scalar_swab_short (ms)": 0.158,
    }


def test_parse_status_ok():
    assert parse_results("") == {"status": "ok"}


def test_parse_generic_output():
    text = "Something happened"
    assert parse_results(text) == {"output": text}
