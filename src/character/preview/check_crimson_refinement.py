#!/usr/bin/env python3
"""Fast real-render continuity check for the two user-marked interrupted seams.
Usage: python3 src/character/preview/check_crimson_refinement.py RENDERER OUT_DIR
The renderer must produce fresh hip.ppm and ads.ppm at exactly 480x300. This
checks a bright crystal-blue 8-connected path through each marked region, excluding dim
bloom. No world scene, enemies, UI, input, API or timing is needed to reproduce.
"""
import collections
import json
from pathlib import Path
import subprocess
import sys

renderer, folder = sys.argv[1], Path(sys.argv[2])
folder.mkdir(parents=True, exist_ok=True)
result = subprocess.run([renderer, str(folder)], capture_output=True, text=True)
(folder / 'renderer.log').write_text(result.stdout + result.stderr)
if result.returncode:
    print(result.stdout + result.stderr)
    raise SystemExit(result.returncode)

def load(name):
    content = (folder / (name + '.ppm')).read_bytes().split(b'\n', 3)
    w, h = map(int, content[1].split())
    assert (w, h) == (480, 300), 'exact game internal resolution required'
    return content[3]

def path_check(name, roi, start, end):
    data = load(name)
    def within(p, bounds):
        x, y = p
        return bounds[0] <= x <= bounds[2] and bounds[1] <= y <= bounds[3]
    energy = set()
    for y in range(roi[1], roi[3] + 1):
        for x in range(roi[0], roi[2] + 1):
            r, g, b = data[(y*480+x)*3:(y*480+x)*3+3]
            if b > 190 and g > 140 and b > 1.15*r:
                energy.add((x, y))
    begin = [p for p in energy if within(p, start)]
    target = {p for p in energy if within(p, end)}
    seen, queue = set(begin), collections.deque(begin)
    while queue:
        x, y = queue.popleft()
        for dx in (-1, 0, 1):
            for dy in (-1, 0, 1):
                p = x+dx, y+dy
                if p in energy and p not in seen:
                    seen.add(p)
                    queue.append(p)
    passed = bool(target & seen)
    min_gap = min((max(abs(a[0]-b[0]), abs(a[1]-b[1])) for a in seen for b in target), default=None)
    return {'pose': name, 'passed': passed, 'startEmissionPixels': len(begin), 'targetEmissionPixels': len(target),
            'roi': roi, 'targetDistanceFromStartComponentPixels': min_gap,
            'threshold': 'B>190 and G>140 and B>1.15*R, 8-connectivity'}

checks = [path_check('hip', (295, 215, 339, 248), (296, 218, 307, 226), (324, 239, 332, 247)),
          path_check('ads', (220, 180, 260, 299), (225, 184, 255, 212), (225, 278, 255, 299))]
(folder / 'continuity.json').write_text(json.dumps(checks, indent=2) + '\n')
for check in checks:
    print(('GREEN' if check['passed'] else 'RED') + ' ' + json.dumps(check))
raise SystemExit(0 if all(c['passed'] for c in checks) else 1)
