"""Deterministic open Crystal layout with a visibly solid mineral rim.
Default mode requires the real Tripo spire. Explicit --development permits a fallback.
No network, generation API or game RNG consumption.
"""
from pathlib import Path
import math,json,sys,os
ROOT=Path(__file__).resolve().parent
REPO=ROOT.parents[2]
REAL='--development' not in sys.argv
SPIRE=REPO/'res/models/tripo/forest/crystal_spire'
if REAL:
 if not (SPIRE/'model.import.json').is_file() or not (SPIRE/'collision.obj').is_file():
  raise SystemExit('Real spire bundle + approved collision required before map generation: '+str(SPIRE))
 bundle=json.loads((SPIRE/'model.import.json').read_text())
class Mesh:
 def __init__(self):self.v=[];self.f=[]
 def tri(self,a,b,c):
  i=len(self.v);self.v.extend([a,b,c]);self.f.append((i+1,i+3,i+2))
 def spindle(self,center,radius,height,sides=7,lean=(0,0),phase=0):
  cx,cy,cz=center;bottom=[];middle=[]
  for i in range(sides):
   a=2*math.pi*i/sides+phase;r=radius*(1+.16*math.sin(i*7.1+phase))
   bottom.append((cx+r*math.cos(a),cy,cz+r*math.sin(a)))
   middle.append((cx+r*.85*math.cos(a)+lean[0]*.4,cy+height*.43,cz+r*.85*math.sin(a)+lean[1]*.4))
  top=(cx+lean[0],cy+height,cz+lean[1])
  for i in range(sides):
   j=(i+1)%sides;self.tri(bottom[i],middle[j],middle[i]);self.tri(bottom[i],bottom[j],middle[j]);self.tri(middle[i],middle[j],top);self.tri((cx,cy,cz),bottom[j],bottom[i])
 def save(self,name):
  p=ROOT/name;p.parent.mkdir(parents=True,exist_ok=True)
  p.write_text('# Original procedural forest development geometry; not Tripo.\n'+'\n'.join('v '+' '.join(f'{x:.6f}' for x in v) for v in self.v)+'\n'+'\n'.join('f '+' '.join(map(str,f)) for f in self.f)+'\n')

# Closed mineral sections share identical seam vertices. Keep the near face
# vertical and above the sprint-jump apex; the outer shoulder tapers into the floor.
RIM_SECTORS=12
FACETS_PER_SECTOR=4
def rim_station(index):
 a=2*math.pi*(index%(RIM_SECTORS*FACETS_PER_SECTOR))/(RIM_SECTORS*FACETS_PER_SECTOR)
 r=100.65+.85*math.sin(a*5+.65)+.42*math.sin(a*11)
 h=2.95+.2*math.sin(a*7+.3)+.1*math.sin(a*11-.1)
 return [(round(radius*math.cos(a),5),round(y,5),round(radius*math.sin(a),5))
         for radius,y in [(r,-.55),(r,h),(r+2.9,h*.72),(r+6.2,-.55)]]

def rim_section(sector):
 stations=[rim_station(sector*FACETS_PER_SECTOR+i) for i in range(FACETS_PER_SECTOR+1)]
 vertices=[coordinate for station in stations for point in station for coordinate in point]
 faces=[]
 def quad(a,b,c,d):
  faces.extend((a,b,c,a,c,d))
 for i in range(FACETS_PER_SECTOR):
  for side in range(4):
   a=i*4+side;d=i*4+(side+1)%4
   quad(a,a+4,d+4,d)
 quad(0,1,2,3)
 end=FACETS_PER_SECTOR*4
 quad(end,end+3,end+2,end+1)
 return dict(id=100+sector,material='forest_rim',color=[.39,.41,.48],type='wall',
             vertices=vertices,faces=faces)

base=json.loads((REPO/'res/maps/crystal_megastructure_arena.json').read_text())
base.update(contentVersion='open-crystal-survival-v1' if REAL else 'open-crystal-survival-development-v1',environmentPreset='forest_crystal')
base['arena']['encounterProfile']='crystal_survival'
base['worldSettings']={'gravity':-9.8,'ambientLight':[.028,.045,.07],'backgroundColor':[.002,.004,.01],'farPlane':600}
base['environment']={'architecture':'forest_crystal','transformationStart':45,'transformationDuration':4,'particleCount':96,'landmarkPosition':[0,0,0],'landmarkScale':[14,140,14]}
base['geometry']={'brushes':[rim_section(i) for i in range(RIM_SECTORS)],'objects':[{'id':1,'name':'Open limestone clearing','shape':'plane','position':[0,0,0],'size':[220,0,220],'rotation':[0,0,0],'collision':True,'type':'floor','material':'forest_ground','color':[.56,.54,.58]}],'models':[]}
models=base['geometry']['models']
def model(i,name,asset,pos,material,color,scale=None,rotation=None,collision=''):
 models.append(dict(id=i,name=name,asset=asset,collisionAsset=collision,position=pos,rotation=rotation or [0,0,0],scale=scale or [1,1,1],material=material,color=color))
if REAL:
 for partIndex,part in enumerate(bundle['parts']):
  asset=os.path.relpath(SPIRE/part['obj'],REPO/'res/maps')
  proxy=os.path.relpath(SPIRE/'collision.obj',REPO/'res/maps') if partIndex==0 else ''
  model(500+partIndex,'Tripo crystal_spire',asset,[0,0,0],'forest_spire',part.get('baseColorFactor',[1,1,1])[:3],collision=proxy)
  if part.get('baseColorTexture'):models[-1]['texture']=os.path.relpath(SPIRE/part['baseColorTexture'],REPO/'res/maps')
  models[-1]['forestAsset']='crystal_spire'
else:
 m=Mesh();m.spindle((0,0,0),7,140,7,(3,-2),.13);m.save('fallback/crystal_spire.obj')
 path='../models/forest_arena/fallback/crystal_spire.obj';model(500,'DEVELOPMENT fallback central crystal',path,[0,0,0],'forest_spire',[.19,.21,.33],collision=path)
# Initial authored proposal; shared gameplay owner reviews spawn and navigation.
base['entities']=[{'type':'player_start','name':'South forest arrival','position':[0,0,60],'rotation':[0,-90,0]}]
for i,p in enumerate([[0,0,40],[40,0,0],[0,0,-40],[-40,0,0]]):base['entities'].append({'type':'rift','name':f'Forest rift {i}','slot':i,'position':p,'rotation':[0,0,0],'scale':[1.5,1.5,1.5]})
for i in range(8):
 a=math.radians(22.5+i*45)
 p=[round(78*math.cos(a),5),0,round(78*math.sin(a),5)]
 base['entities'].append({'type':'rift','name':f'Outer crystal rift {i+1}','slot':i+4,'position':p,'rotation':[0,0,0],'scale':[1.5,1.5,1.5]})
for i,p in enumerate([[-26,0,-15],[26,0,-15],[0,0,30]]):base['entities'].append({'type':'sentry_spawn','name':f'Forest sentry {i}','slot':i,'position':p})
base['lights']=[{'type':'point','position':[0,12,0],'color':[.28,.72,1.],'intensity':.85,'range':108}]
nodes=[];links=[]
for ring,r in enumerate([24,48,72,90]):
 for i in range(32):
  a=2*math.pi*i/32;ident=ring*32+i+1;nodes.append({'id':ident,'position':[round(r*math.cos(a),5),0,round(r*math.sin(a),5)]});links.append({'from':ident,'to':ring*32+(i+1)%32+1,'bidirectional':True})
  if ring:links.append({'from':ident,'to':ident-32,'bidirectional':True})
route=[[0,0,48],[46,0,0],[0,0,-46],[-46,0,0],[0,0,48]]
base['spatial']={'mode':'triangles','navigation':{'nodes':nodes,'links':links},'regions':[{'id':'forest_clearing','name':'RUNESTONE CLEARING','min':[-100,-1,-100],'max':[100,145,100]}],'route':route}
(REPO/'res/maps/starry_crystal_arena.json').write_text(json.dumps(base,indent=2)+'\n')
(ROOT/'layout-state.json').write_text(json.dumps({'developmentPlaceholders':not REAL,'radius':100,'spireHeight':140,'rimSectors':RIM_SECTORS,'rimFacets':RIM_SECTORS*FACETS_PER_SECTOR,'note':'Approved Tripo spire and local collision.' if REAL else 'Explicit development spire fallback.'},indent=2)+'\n')
print('Generated open Crystal map with '+('actual approved Tripo spire' if REAL else 'explicit DEVELOPMENT fallback')+'; legacy unchanged.')
