"""Offline local convex proxy groups for real Tripo cover fragments.
Requires numpy and scipy only in an isolated authoring environment; no runtime dep.
"""
from pathlib import Path
import json,hashlib,math
from collections import Counter
import numpy as np
import scipy
from scipy.spatial import ConvexHull,QhullError
ROOT=Path(__file__).resolve().parent
EPS=.045

def load(path):
 v=[];uv=[];norm=[];faces=[]
 for line in path.read_text().splitlines():
  p=line.split()
  if not p:continue
  if p[0]=='v':v.append([float(x) for x in p[1:4]])
  elif p[0]=='vt':uv.append([float(x) for x in p[1:3]])
  elif p[0]=='vn':norm.append([float(x) for x in p[1:4]])
  elif p[0]=='f':faces.append([[int(x)-1 for x in q.split('/')] for q in p[1:]])
 return np.array(v),np.array(uv),np.array(norm),faces

def distances(samples,triangles):
 # Exact point-to-triangle distance, minimum over this source region's faces.
 a,b,c=triangles[:,0],triangles[:,1],triangles[:,2];ab=b-a;ac=c-a
 normal=np.cross(ab,ac);nn=np.einsum('ij,ij->i',normal,normal)
 d00=np.einsum('ij,ij->i',ab,ab);d01=np.einsum('ij,ij->i',ab,ac);d11=np.einsum('ij,ij->i',ac,ac);den=d00*d11-d01*d01
 results=[]
 for p in samples:
  ap=p-a;dn=np.einsum('ij,ij->i',ap,normal);projection=ap-normal*(dn/np.maximum(nn,1e-24))[:,None]
  d20=np.einsum('ij,ij->i',projection,ab);d21=np.einsum('ij,ij->i',projection,ac)
  u=(d11*d20-d01*d21)/np.maximum(den,1e-24);v=(d00*d21-d01*d20)/np.maximum(den,1e-24)
  d=np.where((u>=-1e-7)&(v>=-1e-7)&(u+v<=1+1e-7),dn*dn/np.maximum(nn,1e-24),np.inf)
  for s,t in [(a,b),(b,c),(c,a)]:
   edge=t-s;f=np.clip(np.einsum('ij,ij->i',p-s,edge)/np.maximum(np.einsum('ij,ij->i',edge,edge),1e-24),0,1)
   q=p-(s+edge*f[:,None]);d=np.minimum(d,np.einsum('ij,ij->i',q,q))
  results.append(math.sqrt(float(np.min(d))))
 return np.array(results)

def convex(points):
 # Extremal points in 42 fixed directions reduce microdetail while keeping a
 # true closed hull. All selected points are on the original fragment.
 dirs=[]
 for i in range(42):
  y=1-2*(i+.5)/42;r=math.sqrt(max(0,1-y*y));phi=i*math.pi*(3-math.sqrt(5));dirs.append([r*math.cos(phi),y,r*math.sin(phi)])
 dirs.extend([[1,0,0],[-1,0,0],[0,1,0],[0,-1,0],[0,0,1],[0,0,-1]])
 selected=np.unique(np.argmax(points@np.array(dirs).T,axis=0));pts=points[selected]
 hull=ConvexHull(pts)
 fs=[]
 for f,e in zip(hull.simplices,hull.equations):
  f=list(map(int,f));n=np.cross(pts[f[1]]-pts[f[0]],pts[f[2]]-pts[f[0]])
  if np.dot(n,e[:3])<0:f[1],f[2]=f[2],f[1]
  fs.append(f)
 # Every local convex shell must have exactly two incident faces per edge.
 edges=Counter(tuple(sorted((a,b))) for f in fs for a,b in zip(f,f[1:]+f[:1]))
 assert all(n==2 for n in edges.values())
 return pts,np.array(fs,dtype=int),hull

def partition(triangles,depth=0):
 points=np.unique(triangles.reshape(-1,3),axis=0)
 try:pts,faces,hull=convex(points)
 except QhullError:return [],len(triangles)
 hp=pts[faces];samples=np.concatenate([hp.mean(axis=1),(hp[:,0]+hp[:,1])/2,(hp[:,1]+hp[:,2])/2,(hp[:,2]+hp[:,0])/2])
 error=float(max(distances(samples,triangles)))
 # Under-coverage is conservatively sampled at all original vertices against
 # convex support planes (not claimed as a continuous Hausdorff bound).
 under=float(np.maximum(points@hull.equations[:,:3].T+hull.equations[:,3],0).max())
 if (error>EPS or under>.03) and len(triangles)>12 and depth<9:
  centers=triangles.mean(axis=1);axis=int(np.argmax(np.ptp(centers,axis=0)));order=np.argsort(centers[:,axis],kind='stable');mid=len(order)//2
  left,dropa=partition(triangles[order[:mid]],depth+1);right,dropb=partition(triangles[order[mid:]],depth+1)
  if left and right:return left+right,dropa+dropb
 return [(pts,faces,{'sampled_surface_error_m':error,'max_vertex_outside_plane_m':under,'source_triangles':len(triangles),'volume':float(hull.volume),'depth':depth})],0

def create(name,source,height,rotation_z=0):
 v,uv,norm,faces=load(source);angle=math.radians(rotation_z);rotation=np.array([[math.cos(angle),-math.sin(angle),0],[math.sin(angle),math.cos(angle),0],[0,0,1]])
 v=v@rotation.T;norm=norm@rotation.T;scale=height/np.ptp(v[:,1]);v*=scale;lo=v.min(axis=0);hi=v.max(axis=0);pivot=np.array([(lo[0]+hi[0])/2,lo[1],(lo[2]+hi[2])/2]);v-=pivot
 welded,inverse=np.unique(np.round(v,8),axis=0,return_inverse=True);parent=list(range(len(welded)))
 def find(x):
  while parent[x]!=x:parent[x]=parent[parent[x]];x=parent[x]
  return x
 face_vertices=[];unique_faces=[];seen=set()
 for f in faces:
  idx=tuple(int(inverse[q[0]]) for q in f);face_vertices.append(idx)
  key=tuple(sorted(idx))
  if len(set(idx))<3 or key in seen:continue
  seen.add(key);unique_faces.append(idx)
  for q in idx[1:]:parent[find(q)]=find(idx[0])
 groups={}
 for f in unique_faces:groups.setdefault(find(f[0]),[]).append(f)
 keep=[];hulls=[];dropped=[]
 for comp,fs in groups.items():
  points=welded[np.unique(np.array(fs))];extent=np.ptp(points,axis=0)
  if extent.max()<.07 or len(fs)<5 or extent.min()<.002:
   dropped.append({'component':comp,'triangles':len(fs),'reason':'sub-7cm or nearly planar microdetail removed from BOTH interior visual and collider'});continue
  hs,drop=partition(welded[np.array(fs)])
  if not hs: dropped.append({'component':comp,'triangles':len(fs),'reason':'no volumetric hull; removed from BOTH interior visual and collider'});continue
  if drop:raise RuntimeError('partial source coverage')
  keep.append(comp);hulls+=hs
 out=ROOT/name;out.mkdir(exist_ok=True);visible=[f for f,idx in zip(faces,face_vertices) if find(idx[0]) in keep];lines=['# Interior cover: real Tripo surface; local uniform scale and floor pivot.']
 for f in visible:
  for q in f:lines.append('v '+' '.join(format(float(x),'.9g') for x in v[q[0]]))
 for f in visible:
  for q in f:lines.append('vt '+' '.join(format(float(x),'.9g') for x in uv[q[1]]))
 for f in visible:
  for q in f:lines.append('vn '+' '.join(format(float(x),'.9g') for x in norm[q[2]]))
 for i in range(len(visible)):lines.append('f '+' '.join(f'{j}/{j}/{j}' for j in range(i*3+1,i*3+4)))
 (out/'visual.obj').write_text('\n'.join(lines)+'\n');lines=['# Closed outward local convex shells; no whole-object bbox.'];offset=0;metadata=[]
 for points,fs,stats in hulls:
  for p in points:lines.append('v '+' '.join(format(float(x),'.9g') for x in p))
  for f in fs:lines.append('f '+' '.join(str(int(x)+offset+1) for x in f))
  offset+=len(points);metadata.append(stats)
 (out/'collision.obj').write_text('\n'.join(lines)+'\n')
 report={'source':str(source.relative_to(ROOT.parent)),'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'uniform_scale':scale,'rotation_z':rotation_z,'floor_pivot_before_rebase':pivot.tolist(),'visual_triangles':len(visible),'collision_triangles':sum(len(h[1]) for h in hulls),'closed_convex_shells':len(hulls),'original_components':len(groups),'dropped_components':dropped,'local_bounds':{'min':v.min(axis=0).tolist(),'max':v.max(axis=0).tolist()},'max_sampled_surface_error_m':max(h[2]['sampled_surface_error_m'] for h in hulls),'max_vertex_outside_plane_m':max(h[2]['max_vertex_outside_plane_m'] for h in hulls),'shells':metadata,'numpy_version':np.__version__,'scipy_version':scipy.__version__,'method':'Connected source components, recursive longest-centroid-axis split if hull surface error>.045m. Fixed-direction extreme points form individually closed outward convex shells. No overall skeleton hull, no voxel stairs. Error samples triangle centroids and edge midpoints; not a continuous worst-case guarantee.'}
 report['visual_sha256']=hashlib.sha256((out/'visual.obj').read_bytes()).hexdigest();report['collision_sha256']=hashlib.sha256((out/'collision.obj').read_bytes()).hexdigest();(out/'proxy-report.json').write_text(json.dumps(report,indent=2)+'\n');print(name,report['visual_triangles'],report['collision_triangles'],len(hulls),report['max_sampled_surface_error_m'],report['max_vertex_outside_plane_m'],flush=True)

create('ship_low',ROOT.parent/'derelict_ship/parts/wreck-0.obj',1.2,90)
create('ribs_low',ROOT.parent/'alien_skeleton/parts/torso-derived.obj',1.2)
create('crystal_low',ROOT.parent/'mineral_cluster/parts/part-000.obj',1.6)
