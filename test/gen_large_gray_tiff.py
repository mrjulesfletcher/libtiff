#!/usr/bin/env python3
import os
import subprocess
import sys

WIDTH = 2000
HEIGHT = 2000

def main(dst, raw2tiff):
    tmp = dst + '.raw'
    with open(tmp, 'wb') as f:
        f.write(b"\x00" * (WIDTH * HEIGHT))
    subprocess.run([raw2tiff, '-w', str(WIDTH), '-l', str(HEIGHT), '-d', 'byte',
                    '-p', 'minisblack', tmp, dst], check=True)
    os.remove(tmp)

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print('Usage: gen_large_gray_tiff.py OUTPUT.tiff RAW2TIFF', file=sys.stderr)
        sys.exit(1)
    main(sys.argv[1], sys.argv[2])
