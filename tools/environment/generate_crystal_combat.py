#!/usr/bin/env python3
"""Original, deterministic metre-scale Y-up modules for Crystal Combat.

Every module is a closed triangle mesh, except explicitly named floor sheets.
Visual and collision files initially share geometry so visible openings are real.
No third-party assets, textures or runtime generation dependency.
"""
from pathlib import Path
from math import sin, cos, pi
import json

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'res/models/crystal_combat'

class Mesh:
    def __init__(self):
        self.vertices = []
        self.faces = []
    def triangle(self, a, b, c):
        i=len(self.vertices)+1
        self.vertices.extend((a,b,c))
        self.faces.append((i,i+1,i+2))
    def quad(self,a,b,c,d):
        self.triangle(a,b,c);self.triangle(a,c,d)
    def box(self, lo, hi, lean=0):
        x,y,z=lo;X,Y,Z=hi
        v=[(x,y,z),(X,y,z),(X,y,Z),(x,y,Z),
           (x+lean,Y,z),(X+lean,Y,z),(X+lean,Y,Z),(x+lean,Y,Z)]
        # outward faces, Y is up
        for a,b,c,d in [(0,1,2,3),(4,7,6,5),(0,4,5,1),(1,5,6,2),(2,6,7,3),(3,7,4,0)]:
            self.quad(v[a],v[b],v[c],v[d])
    def save(self,name):
        path=OUT/(name+'.obj')
        with path.open('w') as f:
            f.write('# Silic2 original procedural module; Y up, metres; CCW outward\n')
            for p in self.vertices:f.write('v '+' '.join(f'{x:.6f}' for x in p)+'\n')
            for face in self.faces:f.write('f '+' '.join(map(str,face))+'\n')
        return {'file':path.name,'vertices':len(self.vertices),'triangles':len(self.faces),
                'minimum':[min(v[k] for v in self.vertices) for k in range(3)],
                'maximum':[max(v[k] for v in self.vertices) for k in range(3)]}

def arc(inner,outer,height,thickness,angle,segments):
    m=Mesh()
    def p(r,a,y):return (r*cos(a),y,r*sin(a))
    for i in range(segments):
        a=angle*i/segments;b=angle*(i+1)/segments
        t=[p(inner,a,height),p(inner,b,height),p(outer,b,height),p(outer,a,height)]
        btm=[(x,y-thickness,z) for x,y,z in t]
        m.quad(*t);m.quad(*reversed(btm))
        m.quad(t[0],btm[0],btm[1],t[1]);m.quad(t[2],btm[2],btm[3],t[3])
        if i==0:m.quad(t[3],btm[3],btm[0],t[0])
        if i==segments-1:m.quad(t[1],btm[1],btm[2],t[2])
    return m

def ramp():
    m=Mesh()
    # Broad 6m ascent: north crest at z=-11 y6, south foot at z11 y0.
    top=[(-3,6,-11),(-3,0,11),(3,0,11),(3,6,-11)]
    bottom=[(-3,-.6,-11),(-3,-.6,11),(3,-.6,11),(3,-.6,-11)]
    m.quad(*top);m.quad(*reversed(bottom))
    for i in range(4):j=(i+1)%4;m.quad(top[i],bottom[i],bottom[j],top[j])
    return m

def shell():
    m=Mesh();n=9
    # Uneven broad facets form one navigational landmark, not a crystal cluster.
    rings=[]
    for y,radial,shift in [(0,3.2,0),(2.8,3.7,.4),(6.8,1.5,1.1),(8.0,.45,1.3)]:
        rings.append([(cos(i*2*pi/n)*radial*(1+.12*sin(i*4.1))+shift,y,sin(i*2*pi/n)*radial) for i in range(n)])
    for j in range(3):
        for i in range(n):k=(i+1)%n;m.quad(rings[j][i],rings[j+1][i],rings[j+1][k],rings[j][k])
    for i in range(1,n-1):m.triangle(rings[0][0],rings[0][i],rings[0][i+1]);m.triangle(rings[-1][0],rings[-1][i+1],rings[-1][i])
    return m

def canopy():
    m=Mesh()
    # Thick half-vault with an actual 10m-wide open underpass, 6m crown.
    # Each panel is closed. Cross-section is intentionally faceted.
    for i in range(7):
        a=pi*i/7;b=pi*(i+1)/7
        def p(t,r,z):return (cos(t)*r,sin(t)*r,z)
        v=[p(a,6,-4),p(b,6,-4),p(b,6,4),p(a,6,4),p(a,6.8,-4),p(b,6.8,-4),p(b,6.8,4),p(a,6.8,4)]
        for q in [(0,3,2,1),(4,5,6,7),(0,1,5,4),(3,7,6,2),(0,4,7,3),(1,2,6,5)]:m.quad(*(v[k] for k in q))
    return m

def main():
    OUT.mkdir(parents=True,exist_ok=True)
    modules={
        'ring_deck_30':arc(12,18,0,.8,pi/6,4),
        'curved_wall_45':arc(28,29.4,7,7,pi/4,6),
        'ramp_6x22':ramp(), 'crystal_shell':shell(), 'crystal_vault':canopy(),
    }
    support=Mesh();support.box((-1.1,0,-1.1),(1.1,11,1.1),4.2);modules['leaning_support']=support
    cover=Mesh();cover.box((-2,0,-.8),(2,1.2,.8),.3);modules['low_cover']=cover
    manifest={name:mesh.save(name) for name,mesh in modules.items()}
    (OUT/'manifest.json').write_text(json.dumps({'origin':'original procedural Silic2 assets','units':'metres','up':'Y','modules':manifest},indent=2)+'\n')
    print(json.dumps(manifest,indent=2))

if __name__=='__main__':main()
