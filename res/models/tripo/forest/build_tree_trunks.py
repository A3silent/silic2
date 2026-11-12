#!/usr/bin/env python3
"""Single closed root/main-trunk lofts from actual horizontal mesh sections.
Needles and small branches are decorative. No overlapping hulls or low trunk caps.
"""
from pathlib import Path
import importlib.util,json,hashlib,math
from collections import defaultdict,Counter
import numpy as np
ROOT=Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('p',ROOT.parent/'mega/build_collision.py');p=importlib.util.module_from_spec(spec);spec.loader.exec_module(p)

def cross(a,b):return a[...,0]*b[...,1]-a[...,1]*b[...,0]
def radii(segments,c,angles):
 a=segments[:,0]-c;e=segments[:,1]-segments[:,0];out=[]
 for angle in angles:
  d=np.array([math.cos(angle),math.sin(angle)]);den=cross(d,e)
  t=np.divide(cross(a,e),den,out=np.full(len(den),np.inf),where=abs(den)>1e-9)
  u=np.divide(cross(a,d),den,out=np.full(len(den),np.inf),where=abs(den)>1e-9)
  t[(u<-.0001)|(u>1.0001)|(t<.0001)]=np.inf
  out.append(float(t.min()))
 return np.array(out)

def section(tri,y,previous):
 segments=[]
 for t in tri:
  points=[]
  for a,b in zip(t,list(t[1:])+[t[0]]):
   if (a[1]<=y)!=(b[1]<=y):points.append((a+(b-a)*((y-a[1])/(b[1]-a[1])))[[0,2]])
  if len(points)==2 and np.linalg.norm(points[0]-points[1])>1e-6:segments.append(points)
 segments=np.array(segments);assert len(segments)
 # Connected cross-section contours distinguish main trunk from separate boughs.
 vertices,inv=np.unique(np.round(segments.reshape(-1,2),4),axis=0,return_inverse=True);edgeids=inv.reshape(-1,2);parent=list(range(len(vertices)))
 def find(i):
  while parent[i]!=i:parent[i]=parent[parent[i]];i=parent[i]
  return i
 for a,b in edgeids:parent[find(a)]=find(b)
 groups=defaultdict(list)
 for i,(a,b) in enumerate(edgeids):groups[find(a)].append(i)
 candidates=[];angles=np.arange(16)*2*math.pi/16
 for indices in groups.values():
  seg=segments[indices];pts=seg.reshape(-1,2)
  for c in [(pts.min(axis=0)+pts.max(axis=0))/2,pts.mean(axis=0)]:
   rs=radii(seg,c,angles)
   if not np.isfinite(rs).all():continue
   area=float(np.mean(rs**2))
   distance=0 if previous is None else float(np.linalg.norm(c-previous))
   candidates.append((area/(1+distance**2*4),c,rs,seg))
 assert candidates,('no closed trunk cross section',y)
 _,c,rs,seg=max(candidates,key=lambda q:q[0])
 ring=np.column_stack((c[0]+rs*np.cos(angles),np.full(16,y),c[1]+rs*np.sin(angles)))
 return ring,c,{'height_m':y,'center_xz':c.tolist(),'min_radius_m':float(rs.min()),'max_radius_m':float(rs.max()),'sample_points_on_actual_surface':True}

def make(name,ys):
 b=ROOT/name;m=json.loads((b/'model.import.json').read_text());part=m['parts'][0]
 v,uv,n,faces=p.load(b/part['obj']);unique=[];seen=set()
 for f in faces:
  t=v[[a[0] for a in f]];key=tuple(sorted(tuple(np.round(x,6)) for x in t))
  if key not in seen:seen.add(key);unique.append(t)
 tri=np.array(unique);rings=[];sections=[];prev=None
 # Start at root-neck, follow trunk upward; descend to root footprint separately.
 for y in ys:
  ring,prev,info=section(tri,y,prev);rings.append(ring);sections.append(info)
 # Original roots end on several disconnected ground toes. Anchor central root
 # neck vertically to floor instead of lofting toward an arbitrary distant toe.
 ground=rings[0].copy();ground[:,1]=0
 rings.insert(0,ground)
 sections.insert(0,{**sections[0],'height_m':0,'sample_points_on_actual_surface':False,'note':'Central low root-neck footprint extended down0.65m to floor; thin disconnected root toes decorative.'})
 points=np.concatenate(rings);faces=[]
 for j in range(len(rings)-1):
  for i in range(16):
   a=j*16+i;b0=j*16+(i+1)%16;c=(j+1)*16+i;d=(j+1)*16+(i+1)%16
   faces.extend([(a,c,b0),(b0,c,d)])
 lower=len(points);points=np.vstack([points,rings[0].mean(axis=0),rings[-1].mean(axis=0)])
 for i in range(16):faces.extend([(lower,i,(i+1)%16),(lower+1,(len(rings)-1)*16+(i+1)%16,(len(rings)-1)*16+i)])
 # Orient whole closed boundary outward.
 triangles=points[np.array(faces)];volume=float(np.einsum('ij,ij->i',triangles[:,0],np.cross(triangles[:,1],triangles[:,2])).sum()/6)
 if volume<0:faces=[tuple(reversed(f)) for f in faces];volume=-volume
 edges=defaultdict(list)
 for f in faces:
  for a,c in zip(f,f[1:]+f[:1]):edges[tuple(sorted((a,c)))].append((a,c))
 assert all(len(q)==2 and q[0]==tuple(reversed(q[1])) for q in edges.values())
 lines=['# Single closed actual-section root and main-trunk loft; high foliage/branches decorative.']
 for point in points:lines.append('v '+' '.join(format(float(x),'.9g') for x in point))
 for f in faces:lines.append('f '+' '.join(str(i+1) for i in f))
 out=ROOT/name/'collision.obj';out.write_text('\n'.join(lines)+'\n')
 report={'source_visual_sha256':hashlib.sha256((ROOT/name/part['obj']).read_bytes()).hexdigest(),'collision_sha256':hashlib.sha256(out.read_bytes()).hexdigest(),'collision_triangles':len(faces),'closed_components':1,'all_edges_have_two_incident_faces':True,'adjacent_edge_directions_opposed':True,'positive_volume_m3':volume,'sections':sections,'method':'16 rays on each selected connected actual horizontal main-trunk/root contour; connect corresponding azimuth samples into one closed nonconvex loft, cap at natural high trunk/crown region. No separate overlapping hulls. Low contact heights densely sampled.','limitations':'Secondary branches, foliage and disconnected thin root tips are decorative. Root-neck footprint extends vertically fromY0.65 to floor; does not follow each thin toe. Between measured heights the main trunk is linearly interpolated; radial16 sampling simplifies root edges, so runtime reachable-side checks remain required. High cap is inside visible wood/crown far above normal player reach; no cap at Y2.5m.'}
 (ROOT/name/'collision-report.json').write_text(json.dumps(report,indent=2)+'\n');print(name,len(faces),[(round(s['height_m'],2),round(s['max_radius_m'],2),[round(x,2) for x in s['center_xz']]) for s in sections],flush=True)
if __name__=='__main__':
 for name,end in [('pine_sparse',12),('pine_dense',8),('pine_crooked',8)]:make(name,[.65,1,1.5,2.5,4,6,8]+([10] if end>=10 else [])+([12] if end>=12 else []))
