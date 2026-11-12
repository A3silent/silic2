"""Real Tripo world-scale structural fragments. No online generation."""
from pathlib import Path
import json,hashlib,math
import numpy as np
ROOT=Path(__file__).resolve().parent

def read(path):
 v=[];uv=[];n=[];f=[]
 for line in path.read_text().splitlines():
  q=line.split()
  if not q:continue
  if q[0]=='v':v.append(list(map(float,q[1:4])))
  elif q[0]=='vt':uv.append(list(map(float,q[1:3])))
  elif q[0]=='vn':n.append(list(map(float,q[1:4])))
  elif q[0]=='f':f.append([[int(k)-1 for k in x.split('/')] for x in q[1:]])
 return np.array(v),np.array(uv),np.array(n),f

def build(name,source,height,rz=0,ry=0,x_min=None):
 v,uv,n,f=read(source)
 if x_min is not None:f=[q for q in f if all(v[k[0],0]>=x_min for k in q)]
 z=math.radians(rz);y=math.radians(ry);R=np.array([[math.cos(y),0,math.sin(y)],[0,1,0],[-math.sin(y),0,math.cos(y)]])@np.array([[math.cos(z),-math.sin(z),0],[math.sin(z),math.cos(z),0],[0,0,1]])
 used=np.unique([k[0] for q in f for k in q]);v=v@R.T;n=n@R.T;scale=height/np.ptp(v[used,1]);v*=scale;lo=v[used].min(axis=0);hi=v[used].max(axis=0);pivot=np.array([(lo[0]+hi[0])/2,lo[1],(lo[2]+hi[2])/2]);v-=pivot
 out=ROOT/name;out.mkdir(exist_ok=True);lines=['# Actual Tripo geometry, world-metre authored structural fragment.'];maps=[]
 for prefix,arr,idx in [('v',v,0),('vt',uv,1),('vn',n,2)]:
  refs=sorted(set(k[idx] for q in f for k in q));maps.append({a:i+1 for i,a in enumerate(refs)})
  for a in refs:lines.append(prefix+' '+' '.join(format(float(x),'.9g') for x in arr[a]))
 for q in f:lines.append('f '+' '.join('/'.join(str(maps[i][k[i]]) for i in range(3)) for k in q))
 p=out/'visual.obj';p.write_text('\n'.join(lines)+'\n');r={'source':str(source.relative_to(ROOT.parent)),'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'x_min_source_cut':x_min,'height':height,'uniform_scale':scale,'rotation_z':rz,'rotation_y':ry,'floor_pivot':pivot.tolist(),'visual_triangles':len(f),'bounds':{'min':v[used].min(axis=0).tolist(),'max':v[used].max(axis=0).tolist()},'visual_sha256':hashlib.sha256(p.read_bytes()).hexdigest()};(out/'provenance.json').write_text(json.dumps(r,indent=2)+'\n');print(name,r['bounds'],len(f))

if __name__=='__main__':
 build('ship_wall',ROOT.parent/'derelict_ship/parts/wreck-0.obj',12,90,90)
 build('rib_wall',ROOT.parent/'alien_skeleton/parts/torso-derived.obj',11,x_min=3.0)
 build('crystal_wall',ROOT.parent/'mineral_cluster/parts/part-000.obj',13)
