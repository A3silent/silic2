#!/usr/bin/env python3
"""Author the combat greybox and explicit three-dimensional navigation graph."""
from pathlib import Path
from math import sin,cos,pi,hypot,dist
import json
ROOT=Path(__file__).resolve().parents[2]
MODELS=ROOT/'res/models/crystal_combat'
objects=[dict(id=1,name='Continuous lower foundation',shape='plane',position=[0,0,0],size=[64,0,64],material='combat_foundation',color=[.18,.24,.27],type='floor')]
models=[];nid=10
palette={'combat_deck':[.25,.31,.35],'combat_crystal':[.09,.22,.29],'combat_ivory':[.46,.49,.43],'combat_brown':[.19,.13,.10]}
def model(name,asset,position,rotation=(0,0,0),scale=(1,1,1),material='combat_deck'):
 global nid
 value=dict(id=nid,name=name,asset='../models/crystal_combat/'+asset+'.obj',collisionAsset='../models/crystal_combat/'+asset+'.obj',position=list(position),rotation=list(rotation),scale=list(scale),material=material,color=palette[material]);models.append(value);nid+=1
for angle in range(180,360,30):model('Upper broken-ring walk '+str(angle),'ring_deck_30',(0,6,0),(0,-angle,0))
for x in [-15,15]:model('West ascent' if x<0 else 'East ascent','ramp_6x22',(x,0,7.5),scale=(1,1,15/22))
for angle in range(0,360,45):
 # The physical perimeter curves, with differing rooflines and open sky.
 height=[1.5,1.7,2.0,2.3,2.1,1.45,1.5,1.7][angle//45]
 model('Orbital outer shell '+str(angle),'curved_wall_45',(0,0,0),(0,-angle,0),(1,height,1),'combat_ivory' if angle<90 else 'combat_brown' if angle<225 else 'combat_crystal')
model('Central invaded keel','crystal_shell',(0,0,2),material='combat_crystal')
model('Lower ravine crystal vault','crystal_vault',(0,0,-23),scale=(1,1,.72),material='combat_crystal')
for z in [-12,0,12]:model('Leaning orbital buttress '+str(z),'leaning_support',(-26,0,z),material='combat_ivory')
model('South tactical cover','low_cover',(0,0,12),rotation=(0,20,0),material='combat_ivory')
model('East tactical cover','low_cover',(23,0,-9),rotation=(0,-25,0),material='combat_ivory')
# Conservative lower navigation excludes solid footprints with capsule clearance;
# the vault opening is deliberately retained, unlike an enclosing AABB proxy.
def lower_ok(p):
 x,y,z=p
 if hypot(x,z)>27.25:return False
 if any(abs(x-side*15)<3.85 and -.85<z<14.99 for side in [-1,1]):return False
 if -4.5<x<5.2 and -2.7<z<6.5:return False
 if -3<x<3.3 and 10<z<14:return False
 if 19.8<x<26 and -11.5<z<-6.5:return False
 if -27.8<x<-23.4 and any(abs(z-t)<1.9 for t in [-12,0,12]):return False
 if 4.5<abs(x)<7.6 and -26.8<z<-19.2:return False
 return True
nodes=[];links=[]
def node(p):
 p=[round(x,5) for x in p]
 for n in nodes:
  if dist(n['position'],p)<.015:return n['id']
 i=len(nodes)+1;nodes.append(dict(id=i,position=p));return i
def link(a,b):
 if a==b:return
 if a>b:a,b=b,a
 if not any(l['from']==a and l['to']==b for l in links):links.append(dict(**{'from':a,'to':b},bidirectional=True))
def lower_clear(a,b):return all(lower_ok([a[k]+(b[k]-a[k])*i/30 for k in range(3)]) for i in range(31))
for x in range(-24,25,6):
 for z in range(-24,25,6):
  if lower_ok((x,0,z)):node((x,0,z))
for x in [-21,21]:
 for z in [-16,-8,4,12,18]:
  if lower_ok((x,0,z)):node((x,0,z))
ground=list(nodes)
for a in ground:
 for b in ground:
  if a['id']<b['id'] and dist(a['position'],b['position'])<8.6 and lower_clear(a['position'],b['position']):link(a['id'],b['id'])
def ground_anchor(p):
 i=node(p)
 near=sorted(ground,key=lambda n:dist(p,n['position']))
 count=0
 for n in near:
  if dist(p,n['position'])<8.1 and lower_clear(p,n['position']):link(i,n['id']);count+=1
  if count==3:break
 assert count,(p,'no ground connection')
 return i
upper=[]
for degree in range(180,361,15):
 a=degree*pi/180;upper.append(node((15*cos(a),6,15*sin(a))))
for a,b in zip(upper,upper[1:]):link(a,b)
for x,top in [(-15,upper[0]),(15,upper[-1])]:
 previous=top
 for z in [3,6,9,12,15]:
  p=(x,6*(15-z)/15,z);i=node(p);link(previous,i);previous=i
 # Foot connects to the broad southern court two metres beyond the slope.
 # This leaves a real wide route around each ramp and the curved outer shell.
 foot=ground_anchor((x,0,17));link(previous,foot)
entities=[dict(type='player_start',name='South arrival',position=[0,0,18],rotation=[0,-90,0])]
ground_anchor((0,0,18))
for slot,p in enumerate([(-23,0,4),(23,0,4),(0,6,-15),(0,0,22)]):
 entities.append(dict(type='rift',name='Rift '+str(slot),slot=slot,position=list(p),rotation=[0,0,0],scale=[1,1,1]))
 if p[1]==0:ground_anchor(p)
for slot,p in enumerate([(-21,0,-8),(22,0,-4),(15*cos(315*pi/180),6,15*sin(315*pi/180))]):
 entities.append(dict(type='sentry_spawn',name='Sentry '+str(slot),slot=slot,position=[round(v,5) for v in p]))
 if p[1]==0:ground_anchor(p)
route=[[0,0,18],[8,0,19],[15,0,17],[15,0,15]]
route += [[15,round(6*(15-z)/15,5),z] for z in [12,9,6,3,0]]
route += [[round(15*cos(a*pi/180),5),6,round(15*sin(a*pi/180),5)] for a in range(345,179,-15)]
route += [[-15,round(6*(15-z)/15,5),z] for z in [3,6,9,12,15]]
route += [[-15,0,17],[-20,0,17],[-22,0,8],[-22,0,5],[-22,0,-6],[-18,0,-17],[-9,0,-16],[0,0,-16],[0,0,-23],[0,0,-15],[0,0,-8],[8,0,-8],[10,0,2],[8,0,16],[0,0,18]]
regions=[]
for id,name,lo,hi in [('ring_court','BROKEN RING COURT',[-12,-1,-10],[12,4,18]),('ravine','LOWER CRYSTAL RAVINE',[-11,-1,-28],[11,4,-10]),('upper_walk','UPPER ORBITAL WALK',[-19,5,-19],[19,9,2]),('support_court','LEANING SUPPORT COURT',[-29,-1,-18],[-19,12,19]),('sky_passage','OPEN SKY PASSAGE',[19,-1,-19],[29,5,20])]:regions.append(dict(id=id,name=name,min=lo,max=hi))
lights=[]
for position,color,intensity,reach in [([0,12,2],[.44,.58,.74],1.1,25),([0,4,-22],[.22,.6,.62],.9,15),([-23,8,4],[.65,.46,.30],.8,18),([23,6,3],[.72,.82,.68],1.1,18),([0,10,-15],[.4,.43,.72],.8,18)]:lights.append(dict(type='point',position=position,color=color,intensity=intensity,range=reach))
doc=dict(schemaVersion=3,version='1.0',gameMode='arena_survival',environmentPreset='starry_crystal',contentVersion='crystal-combat-world-v1',worldSettings=dict(gravity=-9.8,ambientLight=[.3,.35,.42],backgroundColor=[.008,.014,.036]),environment=dict(transformationStart=45,transformationDuration=4,particleCount=128,architecture="crystal_combat",landmarkPosition=[0,8,2],landmarkScale=[6,12,6]),arena=dict(floorBrushId=1),geometry=dict(brushes=[],objects=objects,models=models),entities=entities,lights=lights,spatial=dict(mode='triangles',navigation=dict(nodes=nodes,links=links),regions=regions,route=route))
(ROOT/'res/maps/starry_crystal_arena.json').write_text(json.dumps(doc,indent=2)+'\n')
print(f'Greybox: {len(models)} models, {len(nodes)} navigation nodes, {len(links)} links, {len(route)} route points')
