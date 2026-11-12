#!/usr/bin/env python3
"""Compare frozen P6 frames from forest-render-probe without image dependencies.

Allow sparse floating-point rasterization changes from local-space instancing;
a missing branch/tree or changed material must fail. Review images as well.
"""
import argparse
import json
from pathlib import Path


def ppm(path):
    magic, dimensions, maximum, pixels = path.read_bytes().split(b'\n', 3)
    if magic != b'P6' or maximum != b'255':
        raise ValueError(f'Unsupported diagnostic PPM: {path}')
    width, height = map(int, dimensions.split())
    if len(pixels) != width * height * 3:
        raise ValueError(f'Invalid pixel count: {path}')
    return (width, height), pixels


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('before', type=Path)
    parser.add_argument('after', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    rows = []
    for name in ('arrival', 'tree_edge', 'eye_spire'):
        for width in (320, 640):
            filename = f'{name}-{width}.ppm'
            size, a = ppm(args.before / filename)
            other_size, b = ppm(args.after / filename)
            if size != other_size:
                raise ValueError(f'Dimension mismatch: {filename}')
            differences = [abs(x-y) for x, y in zip(a, b)]
            pixels = [max(differences[i:i+3]) for i in range(0, len(a), 3)]
            different = sum(x > 0 for x in pixels)
            over_eight = sum(x > 8 for x in pixels)
            rows.append(dict(image=filename, different_pixels=different,
                             pixels=len(pixels), over_8=over_eight,
                             max_channel_difference=max(differences),
                             mean_channel_difference=sum(differences)/len(a),
                             passed=(different / len(pixels) <= .001 and
                                     over_eight / len(pixels) <= .0002)))
    args.output.write_text(json.dumps(rows, indent=2) + '\n')
    print(f'{sum(row["passed"] for row in rows)}/{len(rows)} frozen frames pass')
    return 0 if all(row['passed'] for row in rows) else 1


if __name__ == '__main__':
    raise SystemExit(main())
