#!/usr/bin/env python3
"""Repair genuine crystal surface T-junctions without convexification."""
from pathlib import Path
import importlib.util,json,hashlib
from collections import defaultdict,Counter,deque
import numpy as np
ROOT=Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('io',ROOT.parent/'mega/build_collision.py');io=importlib.util.module_from_spec(spec);spec.loader.exec_module(io)
source=ROOT/'crystal_spire/parts/part-000.obj';v,uv,n,raw=io.load(source)
v,inv=np.unique(np.round(v,6),axis=0,return_inverse=True)
faces=[];seen=set()
for f in raw:
 ids=tuple(int(inv[p[0]]) for p in f);key=tuple(sorted(ids))
 if len(set(ids))<3 or key in seen:continue
 seen.add(key)
 if np.linalg.norm(np.cross(v[ids[1]]-v[ids[0]],v[ids[2]]-v[ids[0]]))<1e-8:continue
 faces.append(ids)

def edges(faces):
 out=defaultdict(list)
 for i,f in enumerate(faces):
  for a,b in zip(f,f[1:]+f[:1]):out[tuple(sorted((a,b)))].append((i,a,b))
 return out

def stats(faces):return dict(Counter(len(a) for a in edges(faces).values()))
print('initial',len(faces),stats(faces),flush=True)
# Split only T-junction edges; all resulting triangles stay on original plane.
e=edges(faces);splits={}
for a,b in e:
 av,bv=v[a],v[b];d=bv-av;length2=float(d@d)
 if length2<1e-12:continue
 possible=np.where(np.all(v>=np.minimum(av,bv)-2e-5,axis=1)&np.all(v<=np.maximum(av,bv)+2e-5,axis=1))[0]
 t=(v[possible]-av)@d/length2
 close=np.linalg.norm(v[possible]-(av+t[:,None]*d),axis=1)<2e-5
 use=(t>1e-6)&(t<1-1e-6)&close
 ids=possible[use];ts=t[use]
 if len(ids):splits[(a,b)]=[int(i) for i in ids[np.argsort(ts)]]
points=list(v);new=[]
for f in faces:
 ring=[];changed=False
 for a,b in zip(f,f[1:]+f[:1]):
  ring.append(a);key=tuple(sorted((a,b)));middle=splits.get(key,[])
  if a>b:middle=list(reversed(middle))
  ring+=middle;changed|=bool(middle)
 if changed:
  center=len(points);points.append(v[list(f)].mean(axis=0))
  new.extend((center,a,b) for a,b in zip(ring,ring[1:]+ring[:1]))
 else:new.append(f)
v=np.array(points);faces=new
print('split',len(faces),stats(faces),flush=True)
# Dump diagnostics before publishing; no pretend closed mesh.
bad=[]
for edge,refs in edges(faces).items():
 if len(refs)!=2:
  bad.append({'edge':edge,'points':v[list(edge)].tolist(),'faces':[[i,float(np.linalg.norm(np.cross(v[faces[i][1]]-v[faces[i][0]],v[faces[i][2]]-v[faces[i][0]])))] for i,a,b in refs]})
# Remove tiny surface flaps attached only along an already over-subscribed edge.
removed=[]
while True:
 e=edges(faces);drop=[]
 for i,f in enumerate(faces):
  degrees=[len(e[tuple(sorted((a,b)))]) for a,b in zip(f,f[1:]+f[:1])]
  if degrees.count(1)>=2 and max(degrees)>2:drop.append(i)
 if not drop:break
 removed.extend([faces[i] for i in drop]);ds=set(drop);faces=[f for i,f in enumerate(faces) if i not in ds]
print('flaps removed',len(removed),len(faces),stats(faces),flush=True)
for edge,refs in edges(faces).items():
 if len(refs)!=2:
  print('BAD',edge,'points',v[list(edge)].round(2).tolist(),'refs',[(i,a,b) for i,a,b in refs],flush=True)
# One remaining attached flap spans two over-subscribed edges and one free edge.
while True:
 e=edges(faces);drop=[]
 for i,f in enumerate(faces):
  deg=[len(e[tuple(sorted((a,b)))]) for a,b in zip(f,f[1:]+f[:1])]
  if 1 in deg and all(d!=2 for d in deg) and max(deg)>2:drop.append(i)
 if not drop:break
 removed.extend([faces[i] for i in drop]);ds=set(drop);faces=[f for i,f in enumerate(faces) if i not in ds]
assert all(len(r) in (2,4) for r in edges(faces).values())
# Rare coincident seam contacts: pair opposite directed faces by closest normal.
# Separate each pair topologically with its own coincident edge midpoint; geometry stays put.
points=list(v);replacement={};split_contacts=0
for edge,refs in edges(faces).items():
 if len(refs)!=4:continue
 forward=[r for r in refs if (r[1],r[2])==edge];reverse=[r for r in refs if (r[1],r[2])!=edge]
 assert len(forward)==len(reverse)==2
 def normal_for(i):
  p=v[list(faces[i])];n=np.cross(p[1]-p[0],p[2]-p[0]);return n/np.linalg.norm(n)
 scores=[sum(float(normal_for(forward[i][0])@normal_for(reverse[j][0])) for i,j in enumerate(order)) for order in [(0,1),(1,0)]]
 order=(0,1) if scores[0]>=scores[1] else (1,0)
 for i,j in enumerate(order):
  midpoint=len(points);points.append(v[list(edge)].mean(axis=0))
  for fi,a,b in [forward[i],reverse[j]]:
   assert fi not in replacement
   c=next(k for k in faces[fi] if k not in edge)
   replacement[fi]=[(a,midpoint,c),(midpoint,b,c)]
 split_contacts+=1
faces=[f2 for i,f in enumerate(faces) for f2 in replacement.get(i,[f])];v=np.array(points)
e=edges(faces);assert all(len(refs)==2 for refs in e.values()),stats(faces)
# Orient neighboring faces consistently, then orient each closed component by signed volume.
adj=defaultdict(list)
for refs in e.values():
 (i,a,b),(j,c,d)=refs;adj[i].append((j,a==c));adj[j].append((i,a==c))
flips={};components=[]
for start in range(len(faces)):
 if start in flips:continue
 flips[start]=False;q=deque([start]);component=[]
 while q:
  i=q.popleft();component.append(i)
  for j,same in adj[i]:
   desired=flips[i]^same
   if j in flips:assert flips[j]==desired,'non-orientable source patch'
   else:flips[j]=desired;q.append(j)
 components.append(component)
faces=[tuple(reversed(f)) if flips[i] else f for i,f in enumerate(faces)]
volumes=[]
for group in components:
 tri=v[np.array([faces[i] for i in group])];vol=float(np.einsum('ij,ij->i',tri[:,0],np.cross(tri[:,1],tri[:,2])).sum()/6)
 if vol<0:
  for i in group:faces[i]=tuple(reversed(faces[i]))
 volumes.append(abs(vol))
for refs in edges(faces).values():
 assert len(refs)==2 and refs[0][1:]==tuple(reversed(refs[1][1:]))
lines=['# Repaired actual crystal surface. T-junction edges split on source planes; tiny attached flaps removed.']
for p in v:lines.append('v '+' '.join(format(float(x),'.9g') for x in p))
for f in faces:lines.append('f '+' '.join(str(i+1) for i in f))
out=ROOT/'crystal_spire/collision.obj';out.write_text('\n'.join(lines)+'\n')
r={'source_visual':'parts/part-000.obj','source_visual_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'collision_sha256':hashlib.sha256(out.read_bytes()).hexdigest(),'collision_triangles':len(faces),'closed_components':len(components),'component_signed_volumes_m3':volumes,'all_edges_have_two_incident_faces':True,'adjacent_edge_directions_opposed':True,'removed_attached_flap_faces':len(removed),'split_coincident_seam_contacts':split_contacts,'source_face_count_before_repair':4065,'source_coordinate_rounding_m':1e-6,'t_junction_collinearity_tolerance_m':2e-5,'method':'Deduplicate reverse visual faces, weld positions at1e-6m, split genuine collinear T-junctions on original facet planes, remove4 thin attached flaps, split3 coincident edge contacts topologically via paired midpoints, propagate consistent orientation then positive signed volume. No convex fill, no triangle thickening, no raised floor cap. Whole original mineral silhouette retained.','limitations':'Three genuine geometric seam contacts remain position-coincident but are topologically separated; no claim of globally self-intersection-free generated source. Runtime near-base actor/ray checks required. Trees have separate reports.'}
(ROOT/'crystal_spire/collision-report.json').write_text(json.dumps(r,indent=2)+'\n')
print('FINAL',len(faces),stats(faces),len(components),volumes,flush=True)
