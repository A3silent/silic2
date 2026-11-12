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
EPS=.20

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

def thin_faces(triangles):
 out=[]
 for t in triangles:
  n=np.cross(t[1]-t[0],t[2]-t[0]);length=np.linalg.norm(n)
  if length<1e-10:continue
  n=n/length*.03;pts=np.concatenate([t-n,t+n]);fs=[]
  for a,b,c in [(0,2,1),(3,4,5),(0,1,4),(0,4,3),(1,2,5),(1,5,4),(2,0,3),(2,3,5)]:
   if np.dot(np.cross(pts[b]-pts[a],pts[c]-pts[a]),(pts[a]+pts[b]+pts[c])/3-pts.mean(axis=0))<0:b,c=c,b
   fs.append([a,b,c])
  out.append((pts,np.array(fs),{'sampled_surface_error_m':.03,'max_vertex_outside_plane_m':0,'source_triangles':1,'volume':float(length*.03),'depth':-1,'thin_source_face':True}))
 return out

def partition(triangles,depth=0):
 points=np.unique(triangles.reshape(-1,3),axis=0)
 try:pts,faces,hull=convex(points)
 except QhullError:return thin_faces(triangles),0
 hp=pts[faces];samples=np.concatenate([hp.mean(axis=1),(hp[:,0]+hp[:,1])/2,(hp[:,1]+hp[:,2])/2,(hp[:,2]+hp[:,0])/2])
 error=float(max(distances(samples,triangles)))
 # Under-coverage is conservatively sampled at all original vertices against
 # convex support planes (not claimed as a continuous Hausdorff bound).
 under=float(np.maximum(points@hull.equations[:,:3].T+hull.equations[:,3],0).max())
 if (error>EPS or under>.12) and len(triangles)>8 and depth<6:
  centers=triangles.mean(axis=1);axis=int(np.argmax(np.ptp(centers,axis=0)));order=np.argsort(centers[:,axis],kind='stable');mid=len(order)//2
  left,dropa=partition(triangles[order[:mid]],depth+1);right,dropb=partition(triangles[order[mid:]],depth+1)
  if left and right:return left+right,dropa+dropb
 if error>.35 or under>.20:return thin_faces(triangles),0
 return [(pts,faces,{'sampled_surface_error_m':error,'max_vertex_outside_plane_m':under,'source_triangles':len(triangles),'volume':float(hull.volume),'depth':depth})],0


def make(name):
 p=ROOT/name/'visual.obj';v,uv,n,faces=load(p);welded,inverse=np.unique(np.round(v,7),axis=0,return_inverse=True);parent=list(range(len(welded)))
 def find(x):
  while parent[x]!=x:parent[x]=parent[parent[x]];x=parent[x]
  return x
 unique=[];seen=set()
 for f in faces:
  idx=tuple(int(inverse[q[0]]) for q in f);key=tuple(sorted(idx))
  if len(set(idx))<3 or key in seen:continue
  seen.add(key);unique.append(idx)
  for q in idx[1:]:parent[find(q)]=find(idx[0])
 groups={}
 for f in unique:groups.setdefault(find(f[0]),[]).append(f)
 hulls=[];omitted=[]
 for fs in groups.values():
  tri=welded[np.array(fs)];extent=np.ptp(tri.reshape(-1,3),axis=0)
  if len(fs)<4 or extent.max()<.20 or extent.min()<.001:omitted.append(len(fs));continue
  hs,drop=partition(tri);hulls+=hs
  if not hs:omitted.append(len(fs))
 lines=['# World-scale individually closed hulls; bounded six-level subdivision.'];offset=0
 for points,fs,stats in hulls:
  for a in points:lines.append('v '+' '.join(format(float(x),'.9g') for x in a))
  for f in fs:lines.append('f '+' '.join(str(int(x)+offset+1) for x in f))
  offset+=len(points)
 out=p.parent/'collision.obj';out.write_text('\n'.join(lines)+'\n');r={'visual_sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'collision_sha256':hashlib.sha256(out.read_bytes()).hexdigest(),'collision_triangles':sum(len(h[1]) for h in hulls),'closed_shells':len(hulls),'thin_source_face_shells':sum(1 for h in hulls if h[2].get('thin_source_face')),'omitted_decorative_components_triangles':omitted,'max_sampled_surface_error_m':max(h[2]['sampled_surface_error_m'] for h in hulls),'max_vertex_outside_plane_m':max(h[2]['max_vertex_outside_plane_m'] for h in hulls),'shells':[h[2] for h in hulls],'method':'World-metre connected components with bounded depth6 centroid partition and 0.20m sample target. Hulls exceeding 0.35m sampled distance or 0.20m support-plane error become 0.06m-thick closed source-face slabs. No continuous bound claimed; omitted tiny remnants remain decorative.'};(p.parent/'proxy-report.json').write_text(json.dumps(r,indent=2)+'\n');print(name,r['collision_triangles'],r['closed_shells'],r['max_sampled_surface_error_m'],flush=True)
if __name__=='__main__':
 for name in ['ship_wall','rib_wall','crystal_wall']:make(name)
