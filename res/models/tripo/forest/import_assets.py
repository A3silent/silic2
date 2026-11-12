#!/usr/bin/env python3
"""Reproduce approved forest imports from real local GLBs, into new directories.
No network calls. Destination asset directories must not exist.
"""
from pathlib import Path
import argparse,hashlib,importlib.util,json,math
ROOT=Path(__file__).resolve().parent
REPO=ROOT.parents[3]
spec=importlib.util.spec_from_file_location('static_import',REPO/'tools/assets/import_static_model.py')
mod=importlib.util.module_from_spec(spec);spec.loader.exec_module(mod)

def narrow_spire(folder,manifest):
 original=manifest['bounds'];extent=[original['max'][i]-original['min'][i] for i in range(3)]
 scale=17/max(extent[0],extent[2]);all_vertices=[]
 for part in manifest['parts']:
  path=folder/part['obj'];lines=[];vertices=[]
  for line in path.read_text().splitlines():
   words=line.split()
   if words and words[0]=='v':
    v=[float(x) for x in words[1:4]];v[0]*=scale;v[2]*=scale;vertices.append(v)
    line='v '+' '.join(format(x,'.9g') for x in v)
   elif words and words[0]=='vn':
    n=[float(x) for x in words[1:4]];n[0]/=scale;n[2]/=scale;length=math.sqrt(sum(x*x for x in n))
    line='vn '+' '.join(format(x/length,'.9g') for x in n)
   lines.append(line)
  path.write_text('\n'.join(lines)+'\n')
  part['sha256']=hashlib.sha256(path.read_bytes()).hexdigest()
  part['bounds']={'min':[min(v[i] for v in vertices) for i in range(3)],'max':[max(v[i] for v in vertices) for i in range(3)]}
  all_vertices+=vertices
 manifest['bounds']={'min':[min(v[i] for v in all_vertices) for i in range(3)],'max':[max(v[i] for v in all_vertices) for i in range(3)]}
 manifest['artisticDerivation']={'approvedBy':'ATLAS 2026-10-31','reason':'Generated source proportions are too broad: uniform 140m height produces ~83m base, not approved slender spire. Locally narrow XZ to17m maximum while preserving source GLB, height, UVs and source-face count.','afterUniformNormalizationScaleXYZ':[scale,1,scale],'sourceUniform140mBounds':original,'normalTransform':'inverse transpose of positive diagonal scale, then normalize','changesToSourceFile':False}
 manifest['warnings'].append('Art-directed nonuniform XZ narrowing applied after importer uniform normalization; see artisticDerivation. Collision must use same final coordinate frame.')
 (folder/'model.import.json').write_text(json.dumps(manifest,indent=2)+'\n')

if __name__=='__main__':
 parser=argparse.ArgumentParser(description=__doc__)
 parser.add_argument('--source-root',type=Path,default=REPO/'artifacts/tripo/forest/raw')
 parser.add_argument('--output-root',type=Path,required=True)
 args=parser.parse_args()
 approved=json.loads((REPO/'artifacts/tripo/forest/generation-manifest.json').read_text())
 for a in approved['assets']:
  assert not (args.output_root/a['runtime_id']).exists(), 'Refusing to overwrite existing bundle'
 for a in approved['assets']:
  folder=args.output_root/a['runtime_id']
  prompt=REPO/'tools/tripo/forest-prompts'/a['id']/'prompt.en.txt'
  assert hashlib.sha256(prompt.read_bytes()).hexdigest()==a['sha256']['prompt.en.txt']
  manifest=mod.import_model(args.source_root/a['runtime_id']/'model.glb',folder,size=a['target_dimensions_xyz_m'][1],pivot='floor',rotate=(0,0,0),producer='tripo-api',task_id=a['response']['task_id'],prompt=prompt)
  if a['runtime_id']=='crystal_spire':narrow_spire(folder,manifest)
  print(a['runtime_id'],manifest['bounds'],flush=True)
