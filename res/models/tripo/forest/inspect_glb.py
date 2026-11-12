#!/usr/bin/env python3
"""Read-only genuine GLB preflight through Silic2's existing import parser."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path

REPO = Path(__file__).resolve().parents[4]
spec = importlib.util.spec_from_file_location('silic2_static_import', REPO / 'tools/assets/import_static_model.py')
loader = importlib.util.module_from_spec(spec)
spec.loader.exec_module(loader)
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('source', type=Path)
args = parser.parse_args()
glb = loader.Glb(args.source)
parts = glb.parts()
bounds = loader.bounds(parts)
result = {
    'source': str(args.source.resolve()),
    'sha256': hashlib.sha256(glb.data).hexdigest(),
    'bytes': len(glb.data),
    'scene_bounds_before_manual_orientation': bounds,
    'extent_xyz': [bounds['max'][i] - bounds['min'][i] for i in range(3)],
    'source_triangles': sum(len(p['indices'])//3 for p in parts),
    'expected_runtime_triangles': sum(len(p['indices'])//3 * (2 if p['material']['doubleSided'] else 1) for p in parts),
    'parts': [{
        'source_node': p['sourceNode'], 'source_mesh': p['sourceMesh'],
        'triangles': len(p['indices'])//3,
        'has_uv': p['uv'] is not None, 'has_normals': p['normals'] is not None,
        'bounds': loader.bounds([p]), 'material': p['material'],
    } for p in parts],
    'images': [{'index': i, 'extension': ext, 'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}
               for i,(data,ext) in glb.images.items()],
    'warnings': sorted(glb.warnings),
    'limitations': 'Preflight is not visual orientation, image decode, topology closure or collision acceptance. Source bytes are not modified.',
}
print(json.dumps(result, indent=2))
