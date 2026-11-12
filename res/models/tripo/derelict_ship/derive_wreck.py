"""Partition real Tripo hull into three dislocated, UV-preserving broken pieces.
Every original triangle belongs to exactly one part; no cap or new geometry.
"""
from pathlib import Path
import hashlib,json
root=Path(__file__).resolve().parent;source=root/'parts/part-000.obj';v=[];uv=[];n=[];faces=[]
for line in source.read_text().splitlines():
 a=line.split()
 if not a:continue
 if a[0]=='v':v.append(tuple(map(float,a[1:4])))
 elif a[0]=='vt':uv.append(tuple(map(float,a[1:3])))
 elif a[0]=='vn':n.append(tuple(map(float,a[1:4])))
 elif a[0]=='f':faces.append([tuple(int(x)-1 for x in q.split('/')) for q in a[1:]])
groups=[[],[],[]]
for f in faces:
 z=sum(v[q[0]][0] for q in f)/3
 groups[0 if z < -4 else 2 if z > 4 else 1].append(f)
m=json.loads((root/'model.import.json').read_text());template=m['parts'][0];parts=[];records=[]
for i,group in enumerate(groups):
 vertices=[v[q[0]] for f in group for q in f];lo=[min(p[d] for p in vertices) for d in range(3)];hi=[max(p[d] for p in vertices) for d in range(3)];pivot=[(lo[d]+hi[d])/2 for d in range(3)]
 compact=[tuple(p[d]-pivot[d] for d in range(3)) for p in vertices]
 bounds={'min':[min(p[d] for p in compact) for d in range(3)],'max':[max(p[d] for p in compact) for d in range(3)]}
 lines=['# Original Tripo triangle partition; every face retained once across three pieces.']
 for p in compact:lines.append('v '+' '.join(format(x,'.9g') for x in p))
 for f in group:
  for q in f:lines.append('vt '+' '.join(format(x,'.9g') for x in uv[q[1]]))
 for f in group:
  for q in f:lines.append('vn '+' '.join(format(x,'.9g') for x in n[q[2]]))
 for k in range(len(group)):lines.append('f '+' '.join(f'{j}/{j}/{j}' for j in range(k*3+1,k*3+4)))
 obj=root/f'parts/wreck-{i}.obj';obj.write_text('\n'.join(lines)+'\n')
 part=dict(template);part.update(obj=f'parts/wreck-{i}.obj',bounds=bounds,vertices=len(compact),triangles=len(group),sha256=hashlib.sha256(obj.read_bytes()).hexdigest(),sourcePivot=pivot);parts.append(part)
 records.append({'part':i,'selection':'centroid X<-4' if i==0 else 'centroid X>4' if i==2 else '-4<=centroid X<=4','sourcePivot':pivot,'triangles':len(group)})
assert sum(len(g) for g in groups)==len(faces)
m['parts']=parts;m['bounds']={'min':[min(p['bounds']['min'][d] for p in parts) for d in range(3)],'max':[max(p['bounds']['max'][d] for p in parts) for d in range(3)]};m['triangles']=len(faces);m['vertices']=len(faces)*3
m['derivation']={'operation':'Unique triangle partition by centroid local X at -4m and +4m, rebased each piece to its own bounds center. All original triangles and UV preserved; runtime places pieces independently.','sourceObj':'parts/part-000.obj','sourceObjSha256':hashlib.sha256(source.read_bytes()).hexdigest(),'pieces':records,'retainedTriangles':len(faces),'newTriangles':0,'note':'Pieces have independent pivots; this manifest is a source record, not a whole-ship assembled transform. Use sourcePivot to reconstruct assembly.'}
m['warnings'].append('Dislocated hull partition has uncapped fracture edges; decorative exterior only, no collision proxy.')
(root/'model.derived.json').write_text(json.dumps(m,indent=2)+'\n');print(json.dumps(records))
