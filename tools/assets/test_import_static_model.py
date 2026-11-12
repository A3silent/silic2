"""Real tiny GLBs built in temporary directories; never generated-art substitutes."""
import copy
import json
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest
import import_static_model as importer

import zlib

def png_rgb():
    def chunk(kind, data):
        return struct.pack('>I', len(data))+kind+data+struct.pack('>I', zlib.crc32(kind+data)&0xffffffff)
    return b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR', struct.pack('>IIBBBBB',1,1,8,2,0,0,0))+chunk(b'IDAT', zlib.compress(b'\0\x20\x40\x80'))+chunk(b'IEND', b'')


def fixture():
    # Interleaved POSITION, NORMAL, UV; separate unsigned indices and embedded RGB PNG.
    rows = [(0,0,0, 0,0,1, 0,.25), (2,0,0, 0,0,1, 1,.25), (0,1,0, 0,0,1, 0,.75)]
    binary = b''.join(struct.pack('<8f', *row) for row in rows)
    binary += struct.pack('<3H', 0,1,2)+b'\0\0'
    image = png_rgb()
    binary += image
    doc = {'asset': {'version':'2.0'}, 'scene':0, 'scenes':[{'nodes':[0]}], 'nodes':[{'mesh':0}],
           'meshes':[{'primitives':[{'attributes':{'POSITION':0,'NORMAL':1,'TEXCOORD_0':2}, 'indices':3, 'material':0}]}],
           'buffers':[{'byteLength':len(binary)}],
           'bufferViews':[{'buffer':0,'byteOffset':0,'byteLength':96,'byteStride':32}, {'buffer':0,'byteOffset':96,'byteLength':6}, {'buffer':0,'byteOffset':104,'byteLength':len(image)}],
           'accessors':[{'bufferView':0,'componentType':5126,'count':3,'type':'VEC3','byteOffset':0},
                        {'bufferView':0,'componentType':5126,'count':3,'type':'VEC3','byteOffset':12},
                        {'bufferView':0,'componentType':5126,'count':3,'type':'VEC2','byteOffset':24},
                        {'bufferView':1,'componentType':5123,'count':3,'type':'SCALAR'}],
           'images':[{'bufferView':2,'mimeType':'image/png'}], 'textures':[{'source':0}],
           'materials':[{'pbrMetallicRoughness':{'baseColorFactor':[.5,.6,.7,1], 'baseColorTexture':{'index':0}}}]}
    return doc, binary


def write_glb(path, doc, binary):
    text = json.dumps(doc).encode()
    text += b' ' * (-len(text)%4)
    binary += b'\0' * (-len(binary)%4)
    path.write_bytes(struct.pack('<III',0x46546c67,2,28+len(text)+len(binary))+struct.pack('<II',len(text),0x4e4f534a)+text+struct.pack('<II',len(binary),0x004e4942)+binary)


def obj_arrays(path):
    result = {'v':[], 'vt':[], 'vn':[], 'f':[]}
    for line in path.read_text().splitlines():
        row=line.split()
        if row and row[0] in result:
            result[row[0]].append([float(x) for x in row[1:]] if row[0]!='f' else [int(x.split('/')[0])-1 for x in row[1:]])
    return result


class ImportTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory(prefix='silic2-import-')
        self.root=Path(self.temp.name)
        self.source=self.root/'source.glb'
        self.output=self.root/'bundle'
        self.doc,self.binary=fixture()

    def tearDown(self):
        self.temp.cleanup()

    def run_import(self, **kwargs):
        write_glb(self.source,self.doc,self.binary)
        manifest=importer.import_model(self.source,self.output,**kwargs)
        probe=os.environ.get('SILIC2_BUNDLE_CHECK')
        self.assertTrue(probe, 'run make -f Makefile.map asset-import-test for mandatory C++ roundtrip')
        result=subprocess.run([probe,str(self.output/'model.import.json')],capture_output=True,text=True)
        self.assertEqual(result.returncode,0,result.stderr)
        return manifest, obj_arrays(self.output/manifest['parts'][0]['obj'])

    def rejected(self, message, **kwargs):
        write_glb(self.source,self.doc,self.binary)
        with self.assertRaisesRegex(importer.ImportFailure,message):
            importer.import_model(self.source,self.output,**kwargs)
        self.assertFalse(self.output.exists())

    def test_interleaved_uv_texture_provenance_engine_roundtrip(self):
        manifest,obj=self.run_import(size=4,producer='synthetic-fixture',task_id='fixture-only')
        self.assertEqual(manifest['bounds'],{'min':[-2,0,0], 'max':[2,2,0]})
        self.assertEqual(obj['vt'][0],[0,.75])
        self.assertEqual(obj['vt'][2],[0,.25])
        self.assertEqual((self.output/manifest['parts'][0]['baseColorTexture']).read_bytes(),png_rgb())
        self.assertEqual(manifest['source']['sha256'],importer.sha(self.source.read_bytes()))
        self.assertEqual(manifest['parts'][0]['baseColorFactor'],[.5,.6,.7,1])
        self.assertEqual(obj['vn'],[[0,0,1]]*3)

    def test_hierarchy_nonuniform_scale_mirror_winding(self):
        self.doc['nodes']=[{'translation':[5,2,-3],'children':[1]},{'mesh':0,'scale':[-2,3,1]}]
        manifest,obj=self.run_import(pivot='origin')
        self.assertEqual(manifest['bounds'],{'min':[1,2,-3],'max':[5,5,-3]})
        a,b,c=(obj['v'][i] for i in obj['f'][0])
        face=importer.unit(importer.cross(importer.sub(b,a),importer.sub(c,a)))
        self.assertGreater(importer.dot(face,obj['vn'][0]),.999)

    def test_inverse_transpose_preserves_non_axis_normal(self):
        a=importer.node_transform({'scale':[2,1,.5]})
        n=importer.unit([1,1,1])
        self.assertEqual(importer.normal(a,n),importer.unit([.5,1,2]))

    def test_column_major_matrix_rotation_and_pivot(self):
        self.doc['nodes'][0]['matrix']=[1,0,0,0,0,1,0,0,0,0,1,0,7,8,9,1]
        manifest,_=self.run_import(rotate=[0,90,0],size=.8,pivot='center')
        self.assertAlmostEqual(manifest['bounds']['max'][2]-.0,.4)
        self.assertAlmostEqual(manifest['bounds']['min'][1],-.2)

    def test_unindexed_face_normals_and_double_sided(self):
        p=self.doc['meshes'][0]['primitives'][0]
        del p['indices'];del p['attributes']['NORMAL']
        self.doc['materials'][0]['doubleSided']=True
        manifest,obj=self.run_import()
        self.assertEqual(manifest['triangles'],2)
        self.assertEqual(obj['vn'][0],[0,0,1]);self.assertEqual(obj['vn'][3],[0,0,-1])

    def test_repeated_mesh_nodes_material_parts(self):
        self.doc['nodes']=[{'mesh':0},{'mesh':0,'translation':[0,0,3]}]
        self.doc['scenes'][0]['nodes']=[0,1]
        manifest,_=self.run_import()
        self.assertEqual(len(manifest['parts']),2)

    def test_normalized_uv_and_texture_transform(self):
        self.binary+=b'\0'*((-len(self.binary))%4)
        offset=len(self.binary)
        self.binary+=bytes([0,64,255,64,0,192])
        self.doc['buffers'][0]['byteLength']=len(self.binary)
        self.doc['bufferViews'].append({'buffer':0,'byteOffset':offset,'byteLength':6})
        self.doc['accessors'][2]={'bufferView':3,'componentType':5121,'count':3,'type':'VEC2','normalized':True}
        self.doc['materials'][0]['pbrMetallicRoughness']['baseColorTexture']['extensions']={'KHR_texture_transform':{'offset':[.2,.3],'scale':[.5,.5]}}
        _,obj=self.run_import()
        self.assertAlmostEqual(obj['vt'][0][0],.2)
        self.assertAlmostEqual(obj['vt'][0][1],1-(.3+.5*64/255),places=7)

    def test_rejects_draco(self):
        self.doc['extensionsUsed']=['KHR_draco_mesh_compression']
        self.rejected('unsupported extension')

    def test_rejects_sparse(self):
        self.doc['accessors'][0]['sparse']={'count':1}
        self.rejected('sparse')

    def test_rejects_alpha(self):
        self.doc['materials'][0]['alphaMode']='BLEND'
        self.rejected('transparent')

    def test_rejects_skin_animation(self):
        self.doc['animations']=[{}]
        self.rejected('animated')

    def test_rejects_bad_accessor_range(self):
        self.doc['accessors'][0]['count']=4
        self.rejected('accessor exceeds')

    def test_rejects_bad_indices(self):
        self.binary=self.binary[:96]+struct.pack('<3H',0,1,99)+self.binary[102:]
        self.rejected('index out of range')

    def test_rejects_degenerate_and_atomic_cleanup(self):
        self.binary=self.binary[:96]+struct.pack('<3H',0,1,1)+self.binary[102:]
        self.rejected('degenerate')
        self.assertEqual(sorted(x.name for x in self.root.iterdir()),['source.glb'])

    def test_rejects_cycle_and_singular_transform(self):
        self.doc['nodes'][0]['children']=[0]
        self.rejected('cyclic')
        del self.doc['nodes'][0]['children']
        self.doc['nodes'][0]['scale']=[0,1,1]
        self.rejected('singular')

    def test_existing_output_never_overwritten(self):
        self.output.mkdir(); (self.output/'keep.txt').write_text('preserve')
        write_glb(self.source,self.doc,self.binary)
        with self.assertRaisesRegex(importer.ImportFailure,'already exists'):
            importer.import_model(self.source,self.output)
        self.assertEqual((self.output/'keep.txt').read_text(),'preserve')

    def test_obj_roundtrip_preserves_uv_and_normal(self):
        _,obj=self.run_import()
        output2=self.root/'roundtrip'
        manifest=importer.import_model(self.output/'parts/part-000.obj',output2,pivot='origin')
        again=obj_arrays(output2/manifest['parts'][0]['obj'])
        self.assertEqual(obj,again)
        self.assertTrue(any('materials are not imported' in s for s in manifest['warnings']))

    def test_cpp_bundle_rejects_path_escape_bounds_and_missing(self):
        manifest,_=self.run_import()
        probe=os.environ['SILIC2_BUNDLE_CHECK']
        for key,value in [('obj','../../source.glb'),('obj','parts/missing.obj')]:
            bad=copy.deepcopy(manifest);bad['parts'][0][key]=value
            (self.output/'bad.json').write_text(json.dumps(bad))
            result=subprocess.run([probe,str(self.output/'bad.json')],capture_output=True,text=True)
            self.assertNotEqual(result.returncode,0)
        bad=copy.deepcopy(manifest);bad['bounds']['max'][0]+=1
        (self.output/'bad.json').write_text(json.dumps(bad))
        result=subprocess.run([probe,str(self.output/'bad.json')],capture_output=True,text=True)
        self.assertNotEqual(result.returncode,0);self.assertIn('bounds',result.stderr)
        link=self.output/'parts/escape.obj'
        link.symlink_to(self.source)
        bad=copy.deepcopy(manifest);bad['parts'][0]['obj']='parts/escape.obj'
        (self.output/'bad.json').write_text(json.dumps(bad))
        result=subprocess.run([probe,str(self.output/'bad.json')],capture_output=True,text=True)
        self.assertNotEqual(result.returncode,0);self.assertIn('escapes',result.stderr)


if __name__=='__main__':
    unittest.main()
