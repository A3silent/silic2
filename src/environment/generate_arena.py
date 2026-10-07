#!/usr/bin/env python3
"""Export the original v1 sample mesh to an explicitly chosen historical file.

The canonical v2 arena is maintained through the shared Map module/editor. This
archival generator does not know schema v2 and must not overwrite the live map.
"""
import argparse
import json
from pathlib import Path

brushes = []
def brush(material, vertices, faces, color, kind='wall'):
    brushes.append(dict(id=len(brushes)+1, material=material, type=kind,
                        color=color, vertices=[v for p in vertices for v in p], faces=faces))

def box(material, lo, hi, color):
    x,y,z=lo; X,Y,Z=hi
    vertices=[(x,y,z),(X,y,z),(X,y,Z),(x,y,Z),(x,Y,z),(X,Y,z),(X,Y,Z),(x,Y,Z)]
    # Consistent outward-facing CCW faces, including bottom and top.
    faces=[0,1,2,0,2,3, 4,7,6,4,6,5, 0,4,5,0,5,1,
           1,5,6,1,6,2, 2,6,7,2,7,3, 3,7,4,3,4,0]
    brush(material,vertices,faces,color)

brush('arena_mineral_floor',[(-18,0,-18),(-18,0,18),(18,0,18),(18,0,-18)],
      [0,1,2,0,2,3],[.18,.20,.32],'floor')
box('arena_boundary',(-19,-.2,-19),(19,6,-18),[.14,.17,.29])
box('arena_boundary',(-19,-.2,18),(19,6,19),[.14,.17,.29])
box('arena_boundary',(-19,-.2,-18),(-18,6,18),[.14,.17,.29])
box('arena_boundary',(18,-.2,-18),(19,6,18),[.14,.17,.29])
# Three contiguous crystal lobes: solid below shoulder height, faceted crowns
# within the agreed 4x4x4.2 collision envelope. No walkable gaps or extra obstacles.
crystal_vertices=[]
crystal_faces=[]
for lo,hi,shoulder,peak,tipz in [(-2,-.65,3.05,3.7,.1),(-.65,.7,3.35,4.2,-.35),(.7,2,3.0,3.65,.25)]:
    offset=len(crystal_vertices)
    crystal_vertices += [(lo,0,-2),(hi,0,-2),(hi,0,2),(lo,0,2),
                         (lo,shoulder,-2),(hi,shoulder,-2),(hi,shoulder,2),(lo,shoulder,2),
                         ((lo+hi)*.5,peak,tipz)]
    faces=[0,1,2,0,2,3, 0,4,5,0,5,1, 1,5,6,1,6,2,
           2,6,7,2,7,3, 3,7,4,3,4,0, 4,8,5, 5,8,6, 6,8,7, 7,8,4]
    crystal_faces += [offset+i for i in faces]
brush('arena_living_crystal',crystal_vertices,crystal_faces,[.045,.34,.39])
box('arena_cover',(-10,0,-2),(-8,1.4,2),[.17,.18,.31])
box('arena_cover',(8,0,-2),(10,1.4,2),[.17,.18,.31])
rifts=[(0,0,-15),(15,0,0),(0,0,15),(-15,0,0)]
sentries=[(-11,0,-10),(11,0,-10),(10,0,10)]
entities=[dict(type='player_start',name='SouthStart',position=[0,0,13],rotation=[0,-90,0])]
entities += [dict(type='trigger',name='rift_'+direction,position=list(p)) for direction,p in zip(['north','east','south','west'],rifts)]
entities += [dict(type='trigger',name='sentry_'+str(i),position=list(p)) for i,p in enumerate(sentries)]
lights=[dict(type='point',position=[0,5.2,0],color=[.18,.53,.65],intensity=.65,range=18)]
lights += [dict(type='point',position=[p[0],2,p[2]],color=[.15,.3,.6],intensity=.35,range=9) for p in rifts]
asset=dict(version='1.0',contentVersion='starry-crystal-arena-v1',
           worldSettings=dict(gravity=-9.8,ambientLight=[.32,.36,.48],backgroundColor=[.008,.014,.036]),
           geometry=dict(brushes=brushes),entities=entities,lights=lights)
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',type=Path,required=True,help='Explicit legacy-v1 export path (not the canonical map)')
output=parser.parse_args().output
canonical=Path(__file__).resolve().parents[2]/'res/maps/starry_crystal_arena.json'
if output.resolve()==canonical.resolve():
    parser.error('Refusing to overwrite the canonical v2 arena; use the shared Map editor.')
output.write_text(json.dumps(asset,indent=2)+'\n')
print(output)
