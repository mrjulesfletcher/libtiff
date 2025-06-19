import os, sys; sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..")))
import asyncio
from scripts.run_all_benchmarks import parse_results

async def parse_async(text):
    return parse_results(text)

def test_async_parse_concurrent():
    async def gather():
        outs = [
            "TIFFSwabArrayOfShort: 1.0 ms",
            "scalar_swab_short: 0.8 ms",
        ]
        results = await asyncio.gather(*(parse_async(o) for o in outs))
        assert results[0]["TIFFSwabArrayOfShort (ms)"] == 1.0
        assert results[1]["scalar_swab_short (ms)"] == 0.8

    asyncio.run(gather())

