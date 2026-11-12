#!/usr/bin/env python3
"""Offline static GLB/OBJ -> Silic2 OBJ bundle. Python 3.9+, standard library only."""
import argparse
import base64
import hashlib
import json
import math
import os
from pathlib import Path
import shutil
import struct
import sys
import tempfile

MAX_BYTES = 512 * 1024 * 1024
MAX_VERTICES = 200000
MAX_TRIANGLES = 100000
IDENTITY = [[float(i == j) for j in range(4)] for i in range(4)]


class ImportFailure(ValueError):
    pass


def check(ok, message):
    if not ok:
        raise ImportFailure(message)


def finite(values, size, label):
    check(isinstance(values, (list, tuple)) and len(values) == size, label + ': invalid length')
    check(all(isinstance(v, (int, float)) and not isinstance(v, bool) and math.isfinite(v) for v in values), label + ': non-finite value')
    return list(values)


def integer(value, label, minimum=0):
    check(isinstance(value, int) and not isinstance(value, bool) and value >= minimum, label + ': invalid integer')
    return value


def entry(seq, index, label):
    check(isinstance(seq, list), label + ': expected array')
    integer(index, label)
    check(index < len(seq), label + ': index out of range')
    check(isinstance(seq[index], dict), label + ': expected object')
    return seq[index]


def read_file(path):
    check(path.stat().st_size <= MAX_BYTES, 'input exceeds 512 MiB: ' + str(path))
    return path.read_bytes()


def sha(data):
    return hashlib.sha256(data).hexdigest()


def mul(a, b):
    return [[sum(a[i][k] * b[k][j] for k in range(4)) for j in range(4)] for i in range(4)]


def point(m, v):
    return [sum(m[i][j] * v[j] for j in range(3)) + m[i][3] for i in range(3)]


def cross(a, b):
    return [a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0]]


def sub(a, b):
    return [x-y for x, y in zip(a, b)]


def dot(a, b):
    return sum(x*y for x, y in zip(a, b))


def unit(v):
    length = math.sqrt(dot(v, v))
    check(length > 1e-12 and math.isfinite(length), 'zero or invalid normal')
    return [x/length for x in v]


def determinant(m):
    return dot(m[0][:3], cross(m[1][:3], m[2][:3]))


def normal(m, v):
    # Rows of inverse-transpose = cofactors / determinant, including mirror sign.
    cof = [cross(m[1][:3], m[2][:3]), cross(m[2][:3], m[0][:3]), cross(m[0][:3], m[1][:3])]
    d = determinant(m)
    check(abs(d) > 1e-12, 'singular node transform')
    return unit([dot(row, v)/d for row in cof])


def node_transform(node):
    if 'matrix' in node:
        check(not any(k in node for k in ('translation', 'rotation', 'scale')), 'node mixes matrix and TRS')
        a = finite(node['matrix'], 16, 'node.matrix')
        m = [[a[j*4+i] for j in range(4)] for i in range(4)]
        check(all(abs(m[3][i] - IDENTITY[3][i]) < 1e-7 for i in range(4)), 'non-affine node matrix')
    else:
        t = finite(node.get('translation', [0, 0, 0]), 3, 'translation')
        s = finite(node.get('scale', [1, 1, 1]), 3, 'scale')
        x, y, z, w = finite(node.get('rotation', [0, 0, 0, 1]), 4, 'rotation')
        check(abs(x*x+y*y+z*z+w*w-1) < 1e-4, 'node quaternion must be normalized')
        r = [[1-2*y*y-2*z*z, 2*x*y-2*z*w, 2*x*z+2*y*w],
             [2*x*y+2*z*w, 1-2*x*x-2*z*z, 2*y*z-2*x*w],
             [2*x*z-2*y*w, 2*y*z+2*x*w, 1-2*x*x-2*y*y]]
        m = [[r[i][j]*s[j] for j in range(3)] + [t[i]] for i in range(3)] + [[0, 0, 0, 1]]
    check(abs(determinant(m)) > 1e-12, 'singular node transform')
    return m


def rotation_xyz(degrees):
    m = [r[:] for r in IDENTITY]
    for axis, angle in enumerate(degrees):
        r = [row[:] for row in IDENTITY]
        a, b = (axis+1) % 3, (axis+2) % 3
        c, s = math.cos(math.radians(angle)), math.sin(math.radians(angle))
        r[a][a] = r[b][b] = c
        r[a][b], r[b][a] = -s, s
        m = mul(r, m)
    return m


def bounds(parts):
    vertices = [p['positions'][i] for p in parts for i in p['indices']]
    check(vertices, 'no static geometry in selected scene')
    return {'min': [min(v[i] for v in vertices) for i in range(3)],
            'max': [max(v[i] for v in vertices) for i in range(3)]}


class Glb:
    def __init__(self, path):
        self.path = path
        self.data = read_file(path)
        check(len(self.data) >= 20, 'truncated GLB header')
        magic, version, length = struct.unpack_from('<III', self.data)
        check(magic == 0x46546c67 and version == 2 and length == len(self.data), 'invalid GLB 2 header/length')
        chunks, offset = [], 12
        while offset < length:
            check(offset+8 <= length, 'truncated GLB chunk')
            size, kind = struct.unpack_from('<II', self.data, offset)
            offset += 8
            check(size % 4 == 0 and offset+size <= length, 'invalid GLB chunk length/alignment')
            chunks.append((kind, self.data[offset:offset+size]))
            offset += size
        check(chunks and chunks[0][0] == 0x4e4f534a, 'GLB JSON must be first chunk')
        check(len(chunks) == 2 and chunks[1][0] == 0x004e4942, 'expected JSON plus one embedded BIN chunk')
        self.doc = json.loads(chunks[0][1].decode('utf-8'))
        self.binary = chunks[1][1]
        check(isinstance(self.doc, dict), 'GLB JSON root must be an object')
        check(self.doc.get('asset', {}).get('version') == '2.0', 'glTF version must be 2.0')
        check(self.doc.get('asset', {}).get('minVersion', '2.0') == '2.0', 'unsupported glTF minVersion')
        supported = {'KHR_materials_unlit', 'KHR_texture_transform'}
        check(not set(self.doc.get('extensionsRequired', [])) - supported, 'unsupported required glTF extension (export uncompressed core glTF)')
        for ext in ('KHR_draco_mesh_compression', 'EXT_meshopt_compression', 'KHR_texture_basisu', 'EXT_mesh_gpu_instancing'):
            check(ext not in self.doc.get('extensionsUsed', []), 'unsupported extension: ' + ext)
        check(not self.doc.get('animations') and not self.doc.get('skins'), 'animated/skinned models need static export first')
        buffers = self.doc.get('buffers', [])
        check(len(buffers) == 1 and 'uri' not in buffers[0], 'only one embedded GLB buffer supported')
        size = integer(buffers[0].get('byteLength'), 'buffer.byteLength', 1)
        check(size <= len(self.binary) <= size+3, 'BIN buffer length mismatch')
        self.binary = self.binary[:size]
        self.warnings = set()
        self.images = {}

    def view(self, index):
        v = entry(self.doc.get('bufferViews', []), index, 'bufferView')
        check(v.get('buffer') == 0, 'invalid bufferView.buffer')
        check(not v.get('extensions'), 'bufferView extensions unsupported')
        offset = integer(v.get('byteOffset', 0), 'bufferView.byteOffset')
        size = integer(v.get('byteLength'), 'bufferView.byteLength', 1)
        check(offset+size <= len(self.binary), 'bufferView exceeds BIN buffer')
        return v, self.binary[offset:offset+size]

    def accessor(self, index, kind, allowed):
        a = entry(self.doc.get('accessors', []), index, 'accessor')
        check('sparse' not in a and 'bufferView' in a, 'sparse/zero-initialized accessors unsupported; export dense buffers')
        check(a.get('type') == kind and a.get('componentType') in allowed, 'unsupported accessor type/component')
        formats = {5120: 'b', 5121: 'B', 5122: 'h', 5123: 'H', 5125: 'I', 5126: 'f'}
        components = {'SCALAR': 1, 'VEC2': 2, 'VEC3': 3, 'VEC4': 4}[kind]
        code = formats[a['componentType']]
        width = struct.calcsize('<'+code)
        fmt = '<' + code*components
        size = width*components
        v, data = self.view(a['bufferView'])
        stride = integer(v.get('byteStride', size), 'byteStride', 1)
        count = integer(a.get('count'), 'accessor.count', 1)
        offset = integer(a.get('byteOffset', 0), 'accessor.byteOffset')
        check(count <= 3*MAX_TRIANGLES and stride >= size and stride % width == 0 and offset % width == 0, 'invalid accessor count/stride/alignment')
        check(offset+(count-1)*stride+size <= len(data), 'accessor exceeds bufferView')
        values = [list(struct.unpack_from(fmt, data, offset+i*stride)) for i in range(count)]
        check(all(math.isfinite(x) for v in values for x in v), 'non-finite accessor value')
        if a.get('normalized', False):
            check(code in ('b', 'B', 'h', 'H'), 'invalid normalized component type')
            divisor = {'b': 127, 'B': 255, 'h': 32767, 'H': 65535}[code]
            values = [[max(-1, x/divisor) for x in v] for v in values]
        elif code != 'f' and kind != 'SCALAR':
            raise ImportFailure('integer vertex attributes must be normalized')
        return values

    def material(self, index):
        m = {} if index is None else entry(self.doc.get('materials', []), index, 'material')
        check(m.get('alphaMode', 'OPAQUE') == 'OPAQUE', 'transparent/cutout material unsupported; export opaque geometry')
        pbr = m.get('pbrMetallicRoughness', {})
        factor = finite(pbr.get('baseColorFactor', [1, 1, 1, 1]), 4, 'baseColorFactor')
        check(all(0 <= x <= 1 for x in factor), 'baseColorFactor outside [0,1]')
        # Alpha is irrelevant for OPAQUE materials.
        factor[3] = 1
        result = {'baseColorFactor': factor, 'baseColorTexture': '', 'doubleSided': bool(m.get('doubleSided', False)), 'materialName': str(m.get('name', ''))}
        if m.get('normalTexture') or m.get('occlusionTexture') or m.get('emissiveTexture') or pbr.get('metallicRoughnessTexture') or m.get('emissiveFactor', [0, 0, 0]) != [0, 0, 0]:
            self.warnings.add('Only base color is rendered; normal/metallic/roughness/occlusion/emission maps are not transferred.')
        self.warnings.add('Silic2 uses its own lighting/nearest texture filtering, not glTF PBR; base-color factors are retained.')
        texture = pbr.get('baseColorTexture')
        if texture:
            t = entry(self.doc.get('textures', []), texture.get('index'), 'texture')
            check(not t.get('extensions'), 'texture extensions unsupported')
            sampler = entry(self.doc.get('samplers', []), t['sampler'], 'sampler') if 'sampler' in t else {}
            check(sampler.get('wrapS', 10497) == 10497 and sampler.get('wrapT', 10497) == 10497, 'non-repeat texture wrapping unsupported by runtime')
            result['imageIndex'] = t.get('source')
            transform = texture.get('extensions', {}).get('KHR_texture_transform', {})
            result['texCoord'] = transform.get('texCoord', texture.get('texCoord', 0))
            check(result['texCoord'] in (0, 1), 'only TEXCOORD_0/1 supported')
            result['uvScale'] = finite(transform.get('scale', [1, 1]), 2, 'texture scale')
            result['uvOffset'] = finite(transform.get('offset', [0, 0]), 2, 'texture offset')
            result['uvRotation'] = transform.get('rotation', 0)
            check(isinstance(result['uvRotation'], (float, int)) and math.isfinite(result['uvRotation']), 'invalid texture rotation')
        return result

    def image(self, index):
        if index not in self.images:
            image = entry(self.doc.get('images', []), index, 'image')
            mime = image.get('mimeType')
            if 'bufferView' in image:
                _, data = self.view(image['bufferView'])
            elif image.get('uri', '').startswith('data:'):
                head, encoded = image['uri'].split(',', 1)
                check(head in ('data:image/png;base64', 'data:image/jpeg;base64'), 'only base64 PNG/JPEG image data URI supported')
                mime = head[5:].split(';')[0]
                data = base64.b64decode(encoded, validate=True)
            else:
                raise ImportFailure('GLB images must be embedded PNG/JPEG (no external/network image URLs)')
            extension = image_extension(data)
            check(mime == {'png': 'image/png', 'jpg': 'image/jpeg'}[extension], 'image MIME/header mismatch')
            self.images[index] = (data, extension)
        return self.images[index]

    def parts(self, scene_index=None):
        scenes = self.doc.get('scenes', [])
        check(scenes, 'GLB has no scene')
        if scene_index is None:
            check('scene' in self.doc or len(scenes) == 1, 'multiple scenes: choose --scene explicitly')
            scene_index = self.doc.get('scene', 0)
        scene = entry(scenes, scene_index, 'scene')
        nodes, output, visited = self.doc.get('nodes', []), [], set()
        triangle_budget = 0

        def visit(index, parent, depth):
            nonlocal triangle_budget
            check(depth < 128 and index not in visited, 'cyclic/repeated/deep scene node')
            node = entry(nodes, index, 'node')
            visited.add(index)
            check('skin' not in node and not node.get('weights'), 'skinned/morphed node unsupported')
            check(not node.get('extensions'), 'node extensions unsupported')
            world = mul(parent, node_transform(node))
            if 'mesh' in node:
                mesh = entry(self.doc.get('meshes', []), node['mesh'], 'mesh')
                check(not mesh.get('weights'), 'morph weights unsupported')
                for primitive in mesh.get('primitives', []):
                    check(primitive.get('mode', 4) == 4, 'only TRIANGLES primitives supported')
                    check(not primitive.get('targets') and not primitive.get('extensions'), 'morph targets/primitive extensions unsupported')
                    a = primitive.get('attributes', {})
                    check(not any(k.startswith(('COLOR_', 'JOINTS_', 'WEIGHTS_')) for k in a), 'vertex colors/skin attributes unsupported; bake color into base-color texture')
                    positions = self.accessor(a.get('POSITION'), 'VEC3', (5126,))
                    check(len(positions) <= MAX_VERTICES, 'primitive exceeds 200000 source vertices')
                    normals = self.accessor(a['NORMAL'], 'VEC3', (5126,)) if 'NORMAL' in a else None
                    check(normals is None or len(normals) == len(positions), 'normal count mismatch')
                    indices = [v[0] for v in self.accessor(primitive['indices'], 'SCALAR', (5121, 5123, 5125))] if 'indices' in primitive else list(range(len(positions)))
                    if 'indices' in primitive:
                        check(not self.doc['accessors'][primitive['indices']].get('normalized', False), 'indices cannot be normalized')
                    check(len(indices) % 3 == 0 and len(indices) <= 3*MAX_TRIANGLES, 'invalid/excessive triangle count')
                    check(all(isinstance(i, int) and 0 <= i < len(positions) for i in indices), 'primitive index out of range')
                    mat = self.material(primitive.get('material'))
                    triangle_budget += len(indices)//3 * (2 if mat['doubleSided'] else 1)
                    check(triangle_budget <= MAX_TRIANGLES, 'bundle exceeds 100000 total triangles; decimate source offline')
                    uv_name = 'TEXCOORD_' + str(mat.get('texCoord', 0))
                    uv = self.accessor(a[uv_name], 'VEC2', (5126, 5121, 5123)) if uv_name in a else None
                    check(uv is None or len(uv) == len(positions), 'UV count mismatch')
                    check('imageIndex' not in mat or uv is not None, 'textured primitive missing selected UV set')
                    if uv:
                        angle = mat.get('uvRotation', 0)
                        scale, off = mat.get('uvScale', [1, 1]), mat.get('uvOffset', [0, 0])
                        uv = [[off[0] + math.cos(angle)*u*scale[0] - math.sin(angle)*v*scale[1], 1-(off[1] + math.sin(angle)*u*scale[0] + math.cos(angle)*v*scale[1])] for u, v in uv]
                    if determinant(world) < 0:
                        indices = [i for k in range(0, len(indices), 3) for i in (indices[k], indices[k+2], indices[k+1])]
                    if 'imageIndex' in mat:
                        self.image(mat['imageIndex'])
                    output.append({'positions': [point(world, v) for v in positions], 'normals': [normal(world, n) for n in normals] if normals else None,
                                   'uv': uv, 'indices': indices, 'material': mat, 'sourceNode': index, 'sourceMesh': node['mesh']})
                    check(len(output) <= 128, 'bundle exceeds 128 primitive instances')
            for child in node.get('children', []):
                visit(child, world, depth+1)
        for root in scene.get('nodes', []):
            visit(root, IDENTITY, 0)
        return output


def image_extension(data):
    check(len(data) <= 64*1024*1024, 'image exceeds 64 MiB')
    if data.startswith(b'\x89PNG\r\n\x1a\n'):
        check(len(data) >= 33 and data[12:16] == b'IHDR', 'invalid PNG header')
        w, h = struct.unpack_from('>II', data, 16)
        check(0 < w <= 8192 and 0 < h <= 8192, 'PNG dimensions exceed 8192')
        check(data[25] != 4, 'gray+alpha PNG unsupported by engine texture loader; export RGB/RGBA')
        return 'png'
    check(data.startswith(b'\xff\xd8\xff'), 'only PNG/JPEG base-color images supported')
    return 'jpg'


def obj_parts(path):
    """OBJ geometry import; MTL is deliberately explicit rather than silently discarded."""
    positions, uv, normals, indices, output_p, output_u, output_n = [], [], [], [], [], [], []
    warnings = {'OBJ has no standard units; caller controls scale/orientation.', 'OBJ materials are not imported; use GLB for base-color material/texture preservation.'}
    for line in read_file(path).decode('utf-8-sig').splitlines():
        tokens = line.partition('#')[0].split()
        if not tokens:
            continue
        tag, values = tokens[0], tokens[1:]
        if tag in ('v', 'vt', 'vn'):
            n = 2 if tag == 'vt' else 3
            check(len(values) == n, 'OBJ expects plain v/vt/vn (no homogeneous coordinates or vertex colors)')
            value = finite([float(x) for x in values], n, tag)
            {'v': positions, 'vt': uv, 'vn': normals}[tag].append(value)
            check(len(positions) <= MAX_VERTICES, 'OBJ exceeds 200000 source vertices')
        elif tag == 'f':
            check(len(values) == 3, 'OBJ input must be triangulated')
            for token in values:
                fields = token.split('/')
                check(len(fields) <= 3, 'invalid OBJ face')
                def get(text, data):
                    i = int(text)
                    check(i != 0 and -len(data) <= i <= len(data), 'OBJ index out of range')
                    return data[i-1 if i > 0 else i]
                output_p.append(get(fields[0], positions))
                output_u.append(get(fields[1], uv) if len(fields) > 1 and fields[1] else [0, 0])
                output_n.append(get(fields[2], normals) if len(fields) > 2 and fields[2] else None)
                indices.append(len(indices))
            check(len(indices) <= 3*MAX_TRIANGLES, 'OBJ exceeds 100000 triangles')
        else:
            check(tag in ('o', 'g', 's', 'mtllib', 'usemtl'), 'unsupported OBJ record: ' + tag)
    check(indices, 'OBJ contains no triangles')
    if not all(n is not None for n in output_n):
        output_n = None
    return [{'positions': output_p, 'uv': output_u, 'normals': output_n, 'indices': indices,
             'material': {'baseColorFactor': [1, 1, 1, 1], 'baseColorTexture': '', 'doubleSided': False}}], warnings


def import_model(source, destination, *, size=None, scale=1.0, rotate=(0, 0, 0), pivot='floor', scene=None, producer='unspecified', task_id='', prompt=None):
    source, destination = Path(source).resolve(), Path(destination).resolve()
    check(not destination.exists(), 'destination already exists; use a new bundle directory')
    finite(list(rotate), 3, 'rotate')
    check(math.isfinite(scale) and scale > 0, 'scale must be positive and finite')
    check(size is None or math.isfinite(size) and size > 0, 'size must be positive and finite')
    check(pivot in ('floor', 'center', 'origin'), 'invalid pivot')
    check(source.suffix.lower() in ('.glb', '.obj'), 'source must be .glb or triangulated .obj')
    source_data = read_file(source)
    glb = None
    if source.suffix.lower() == '.glb':
        glb = Glb(source)
        parts = glb.parts(scene)
        warnings = glb.warnings
    else:
        check(scene is None, '--scene only applies to GLB')
        parts, warnings = obj_parts(source)
    source_bounds = bounds(parts)
    rotation = rotation_xyz(rotate)
    for part in parts:
        part['positions'] = [point(rotation, v) for v in part['positions']]
        if part['normals']:
            part['normals'] = [normal(rotation, n) for n in part['normals']]
    oriented = bounds(parts)
    extent = [b-a for a, b in zip(oriented['min'], oriented['max'])]
    check(max(extent) > 1e-9, 'zero-size geometry')
    actual_scale = size/max(extent) if size is not None else scale
    offset = [(a+b)/2 for a, b in zip(oriented['min'], oriented['max'])] if pivot != 'origin' else [0, 0, 0]
    if pivot == 'floor':
        offset[1] = oriented['min'][1]
    for part in parts:
        part['positions'] = [[(v[i]-offset[i])*actual_scale for i in range(3)] for v in part['positions']]
        check(all(math.isfinite(x) and abs(x) <= 100000 for v in part['positions'] for x in v), 'coordinates exceed runtime bounds after normalization')
    manifest = {'format': 'silic2-static-model', 'schemaVersion': 1,
                'source': {'file': source.name, 'sha256': sha(source_data), 'bytes': len(source_data), 'producer': producer, 'taskId': task_id},
                'normalization': {'rotationDegreesXYZ': list(rotate), 'scale': actual_scale, 'pivot': pivot, 'sourcePivotAfterRotation': offset, 'sourceBounds': source_bounds,
                                  'coordinates': 'right-handed +Y up; caller chooses forward using rotation', 'uvConvention': 'OBJ bottom-left; GLB V flipped for vertically-flipped engine texture loader'},
                'bounds': bounds(parts), 'parts': [], 'warnings': []}
    if prompt:
        prompt_path = Path(prompt)
        manifest['source']['promptFile'] = prompt_path.name
        manifest['source']['promptSha256'] = sha(read_file(prompt_path))
    destination.parent.mkdir(parents=True, exist_ok=True)
    temporary = Path(tempfile.mkdtemp(prefix='.'+destination.name+'-', dir=destination.parent))
    try:
        (temporary/'parts').mkdir()
        (temporary/'textures').mkdir()
        for number, part in enumerate(parts):
            mat = part['material']
            obj = f'parts/part-{number:03d}.obj'
            descriptor = {k: mat[k] for k in ('baseColorFactor', 'baseColorTexture', 'doubleSided')}
            descriptor.update({'obj': obj, 'bounds': bounds([part]), 'sourceNode': part.get('sourceNode'), 'sourceMesh': part.get('sourceMesh')})
            if 'imageIndex' in mat:
                data, extension = glb.image(mat['imageIndex'])
                texture = f'textures/basecolor-{mat["imageIndex"]:03d}.{extension}'
                (temporary/texture).write_bytes(data)
                descriptor.update(baseColorTexture=texture, textureSha256=sha(data))
            # Expand triangle corners: exact seams, normals and optional double-sided duplication.
            corners = []
            for k in range(0, len(part['indices']), 3):
                tri = part['indices'][k:k+3]
                face = cross(sub(part['positions'][tri[1]], part['positions'][tri[0]]), sub(part['positions'][tri[2]], part['positions'][tri[0]]))
                check(math.sqrt(dot(face, face)) >= 1e-7, 'degenerate/too-small triangle after normalization; repair/export geometry')
                face = unit(face)
                sides = [(tri, 1)] + ([(list(reversed(tri)), -1)] if mat['doubleSided'] else [])
                for side, sign in sides:
                    for index in side:
                        n = part['normals'][index] if part['normals'] else face
                        corners.append((part['positions'][index], part['uv'][index] if part['uv'] else [0, 0], [sign*x for x in n]))
            # Loader has 200k v/vn and 100k triangle limits; split needs explicit DCC remediation.
            check(len(corners) <= MAX_VERTICES, 'expanded primitive exceeds 200000 vertices (double-sided export may double it); decimate/split offline')
            with (temporary/obj).open('w', encoding='utf-8', newline='\n') as f:
                f.write('# Silic2 static import; scene transforms and normalization baked.\n')
                for p, _, _ in corners:
                    f.write('v ' + ' '.join(f'{x:.9g}' for x in p) + '\n')
                for _, uv, _ in corners:
                    f.write('vt ' + ' '.join(f'{x:.9g}' for x in uv) + '\n')
                for _, _, n in corners:
                    f.write('vn ' + ' '.join(f'{x:.9g}' for x in n) + '\n')
                for k in range(0, len(corners), 3):
                    f.write('f '+' '.join(f'{i}/{i}/{i}' for i in range(k+1, k+4))+'\n')
            obj_data = read_file(temporary/obj)
            check(len(obj_data) <= 32*1024*1024, 'output OBJ exceeds runtime 32 MiB limit')
            descriptor.update(vertices=len(corners), triangles=len(corners)//3, sha256=sha(obj_data))
            manifest['parts'].append(descriptor)
        manifest['triangles'] = sum(p['triangles'] for p in manifest['parts'])
        manifest['vertices'] = sum(p['vertices'] for p in manifest['parts'])
        check(manifest['triangles'] <= MAX_TRIANGLES, 'bundle exceeds 100000 total triangles; decimate source offline')
        if any(p['doubleSided'] for p in manifest['parts']):
            warnings.add('Double-sided primitives were expanded with reversed faces/normals; triangle budget includes both sides.')
        warnings.add('Visual mesh is not an approved collision proxy; author collision separately and validate routes before placement.')
        manifest['warnings'] = sorted(warnings)
        (temporary/'model.import.json').write_text(json.dumps(manifest, indent=2, ensure_ascii=False)+'\n', encoding='utf-8')
        # Never overwrite an existing source/output bundle.
        check(not destination.exists(), 'destination appeared during import')
        os.rename(temporary, destination)
    finally:
        if temporary.exists():
            shutil.rmtree(temporary)
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('destination', type=Path, help='new bundle directory (must not exist)')
    fit = parser.add_mutually_exclusive_group()
    fit.add_argument('--size', type=float, help='longest final AABB dimension, after rotation')
    fit.add_argument('--scale', type=float, default=1.0, help='uniform scale when --size is absent')
    parser.add_argument('--rotate', type=float, nargs=3, default=[0, 0, 0], metavar=('X', 'Y', 'Z'), help='degrees, X then Y then Z')
    parser.add_argument('--pivot', choices=('floor', 'center', 'origin'), default='floor')
    parser.add_argument('--scene', type=int)
    parser.add_argument('--producer', default='unspecified', help='provenance label, e.g. tripo-studio; not verified by tool')
    parser.add_argument('--task-id', default='')
    parser.add_argument('--prompt', type=Path, help='record prompt filename and SHA256, no network request')
    args = vars(parser.parse_args())
    try:
        manifest = import_model(**args)
        print(json.dumps({'manifest': str(args['destination']/'model.import.json'), 'triangles': manifest['triangles'], 'bounds': manifest['bounds'], 'warnings': manifest['warnings']}, indent=2))
    except (ImportFailure, OSError, ValueError, TypeError, KeyError, struct.error) as exc:
        print('Import failed: '+str(exc), file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
