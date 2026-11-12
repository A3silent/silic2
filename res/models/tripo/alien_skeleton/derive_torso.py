"""Remove recognizable skull/limbs from actual Tripo mesh; retain original assets.
This is a deterministic offline triangle selection, not a new generation.
"""
from pathlib import Path
import json,hashlib
root=Path(__file__).resolve().parent
source=root/'parts/part-000.obj';v=[];uv=[];n=[];faces=[]
for line in source.read_text().splitlines():
 s=line.split()
 if not s:continue
 if s[0]=='v':v.append(tuple(map(float,s[1:4])))
 elif s[0]=='vt':uv.append(tuple(map(float,s[1:3])))
 elif s[0]=='vn':n.append(tuple(map(float,s[1:4])))
 elif s[0]=='f':faces.append([tuple(int(q)-1 for q in p.split('/')) for p in s[1:]])
kept=[face for face in faces if all(v[p[0]][1]>=8 and v[p[0]][2]<=8 for p in face)]
vertices=[v[q[0]] for f in kept for q in f]
bounds={'min':[min(q[i] for q in vertices) for i in range(3)],'max':[max(q[i] for q in vertices) for i in range(3)]}
lines=['# Derived original Tripo rib/spine fragment: keep Y>=8, Z<=8; no new faces.']
for f in kept:
 for q in f:lines.append('v '+' '.join(format(x,'.9g') for x in v[q[0]]))
for f in kept:
 for q in f:lines.append('vt '+' '.join(format(x,'.9g') for x in uv[q[1]]))
for f in kept:
 for q in f:lines.append('vn '+' '.join(format(x,'.9g') for x in n[q[2]]))
for i in range(len(kept)):lines.append('f '+' '.join(f'{j}/{j}/{j}' for j in range(i*3+1,i*3+4)))
out=root/'parts/torso-derived.obj';out.write_text('\n'.join(lines)+'\n')
m=json.loads((root/'model.import.json').read_text());part=m['parts'][0];part['obj']='parts/torso-derived.obj';part['bounds']=bounds;part['vertices']=len(vertices);part['triangles']=len(kept);part['sha256']=hashlib.sha256(out.read_bytes()).hexdigest();m['bounds']=bounds;m['triangles']=len(kept);m['vertices']=len(vertices)
if 'totals' in m:m['totals'].update({'vertices':len(vertices),'triangles':len(kept)})
m['derivation']={'operation':'Select original triangles where every vertex Y>=8m and Z<=8m in normalized original bundle coordinates. No new geometry; preserve UVs/normals/double-sided faces.','sourceObj':'parts/part-000.obj','sourceObjSha256':hashlib.sha256(source.read_bytes()).hexdigest(),'removedTriangles':len(faces)-len(kept),'retainedTriangles':len(kept),'purpose':'Remove recognizable skull and legs; display incomplete ribs/spine, bury cut roots in inaccessible outer structure.'}
m['warnings'].append('Derived fragment has deliberately open cut edges; decorative only, not a collision mesh.')
(root/'model.derived.json').write_text(json.dumps(m,indent=2)+'\n');print(len(faces),'->',len(kept),bounds)
