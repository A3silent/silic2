"""Deterministic forest layout and visibly colliding woodland roots.
Default mode requires real Tripo bundles. Explicit --development permits placeholders.
No network, generation API or game RNG consumption.
"""
from pathlib import Path
import math,json,random,sys,os,hashlib
ROOT=Path(__file__).resolve().parent
REPO=ROOT.parents[2]
REAL='--development' not in sys.argv
bundles={}
if REAL:
 for species in ['pine_sparse','pine_dense','pine_crooked','crystal_spire']:
  folder=REPO/'res/models/tripo/forest'/species
  if not (folder/'model.import.json').is_file() or not (folder/'collision.obj').is_file():
   raise SystemExit('Real bundle + approved collision required before map generation: '+str(folder))
  bundles[species]=(folder,json.loads((folder/'model.import.json').read_text()))
rng=random.Random(3102026)
class Mesh:
 def __init__(self):self.v=[];self.f=[]
 def tri(self,a,b,c):
  i=len(self.v);self.v.extend([a,b,c]);self.f.append((i+1,i+3,i+2))
 def spindle(self,center,radius,height,sides=7,lean=(0,0),phase=0,shoulder=.43,verticalShoulder=False):
  cx,cy,cz=center;bottom=[];middle=[]
  for i in range(sides):
   a=2*math.pi*i/sides+phase;r=radius*(1+.16*math.sin(i*7.1+phase))
   bottom.append((cx+r*math.cos(a),cy,cz+r*math.sin(a)))
   middle.append((cx+r*math.cos(a),cy+height*shoulder,cz+r*math.sin(a)) if verticalShoulder else (cx+r*.85*math.cos(a)+lean[0]*.4,cy+height*shoulder,cz+r*.85*math.sin(a)+lean[1]*.4))
  top=(cx+lean[0],cy+height,cz+lean[1])
  for i in range(sides):
   j=(i+1)%sides;self.tri(bottom[i],middle[j],middle[i]);self.tri(bottom[i],bottom[j],middle[j]);self.tri(middle[i],middle[j],top);self.tri((cx,cy,cz),bottom[j],bottom[i])
 def save(self,name):
  p=ROOT/name;p.parent.mkdir(parents=True,exist_ok=True)
  p.write_text('# Original procedural forest development geometry; not Tripo.\n'+'\n'.join('v '+' '.join(f'{x:.6f}' for x in v) for v in self.v)+'\n'+'\n'.join('f '+' '.join(map(str,f)) for f in self.f)+'\n')

# Continuous irregular woodland barrier made from the same visible root/foliage masses.
# Twelve sectors keep broadphase/minimap bounds local; no invisible circle collider.
for sector in range(12):
 m=Mesh()
 for j in range(10):
  a=(sector*10+j+.1)*2*math.pi/120;r=105.8+rng.uniform(-.7,.9);x,z=r*math.cos(a),r*math.sin(a)
  m.spindle((x,-.7,z),6.7+rng.uniform(-.2,.5),4.9+rng.uniform(-.4,1.2),9,(rng.uniform(-1,1),rng.uniform(-1,1)),a,.82,True)
  for k in range(3):
   q=a+rng.uniform(-.055,.055);rr=r+rng.uniform(0,3);m.spindle((rr*math.cos(q),.2,rr*math.sin(q)),2.0+rng.random(),2.4+rng.random()*2,5,(rng.uniform(-1.5,1.5),rng.uniform(-1.5,1.5)),q)
 m.save(f'woodland/roots-{sector:02}.obj')
for species,height in [('pine_sparse',22),('pine_dense',18),('pine_crooked',20)]:
 m=Mesh();m.spindle((0,0,0),.65,height*.91,7,(1 if species=='pine_crooked' else 0,0))
 tiers=5 if species=='pine_sparse' else 8 if species=='pine_dense' else 4
 for i in range(tiers):
  y=height*(.27+.62*i/tiers);radius=(3.9 if species=='pine_dense' else 4.8 if species=='pine_crooked' else 3.2)*(1-i/(tiers+1));x=1.2*math.sin(i*1.5) if species=='pine_crooked' else 0
  m.spindle((x,y,0),radius,height*.25,8 if species=='pine_dense' else 6,(0,0),i*.7)
 m.save(f'fallback/{species}.obj')
m=Mesh();m.spindle((0,0,0),7,140,7,(3,-2),.13);m.save('fallback/crystal_spire.obj')

base=json.loads((REPO/'res/maps/crystal_megastructure_arena.json').read_text())
base.update(contentVersion='forest-crystal-real-assets-v1' if REAL else 'forest-crystal-development-v1',environmentPreset='forest_crystal')
base['worldSettings']={'gravity':-9.8,'ambientLight':[.27,.28,.35],'backgroundColor':[.008,.014,.036],'farPlane':600}
base['environment']={'architecture':'forest_crystal','transformationStart':45,'transformationDuration':4,'particleCount':96,'landmarkPosition':[0,0,0],'landmarkScale':[14,140,14]}
base['geometry']={'brushes':[],'objects':[{'id':1,'name':'Open limestone clearing','shape':'plane','position':[0,0,0],'size':[220,0,220],'rotation':[0,0,0],'collision':True,'type':'floor','material':'forest_ground','color':[.56,.54,.58]}],'models':[]}
models=base['geometry']['models']
def model(i,name,asset,pos,material,color,scale=None,rotation=None,collision=''):
 models.append(dict(id=i,name=name,asset=asset,collisionAsset=collision,position=pos,rotation=rotation or [0,0,0],scale=scale or [1,1,1],material=material,color=color))
def imported(ident,species,pos,material,scale=None,rotation=None,collidable=True):
 folder,bundle=bundles[species]
 for partIndex,part in enumerate(bundle['parts']):
  asset=os.path.relpath(folder/part['obj'],REPO/'res/maps')
  proxy=os.path.relpath(folder/'collision.obj',REPO/'res/maps') if partIndex==0 and collidable else ''
  model(ident+partIndex,'Tripo '+species,asset,pos,material,part.get('baseColorFactor',[1,1,1])[:3],scale,rotation,proxy)
  if part.get('baseColorTexture'):models[-1]['texture']=os.path.relpath(folder/part['baseColorTexture'],REPO/'res/maps')
  models[-1]['forestAsset']=species

for i in range(12):
 path=f'../models/forest_arena/woodland/roots-{i:02}.obj';model(100+i,'Visible tangled woodland edge',path,[0,0,0],'forest_thicket',[.17,.21,.17],collision=path)
for i in range(48):
 a=2*math.pi*(i+.15*rng.random())/48;r=(100+rng.uniform(-.2,2.8)) if i%4==0 else 111+rng.uniform(-3,8);s=.85+rng.random()*.36;sp=['pine_sparse','pine_dense','pine_crooked'][i%3]
 pos=[r*math.cos(a),0,r*math.sin(a)];rot=[0,rng.uniform(0,360),0]
 if REAL:imported(2000+i*16,sp,pos,'forest_pine',[s,s,s],rot)
 else:model(200+i,'DEVELOPMENT fallback '+sp,f'../models/forest_arena/fallback/{sp}.obj',pos,'forest_pine',[.25,.29,.22],[s,s,s],rot)
if REAL:imported(500,'crystal_spire',[0,0,0],'forest_spire')
else:
 path='../models/forest_arena/fallback/crystal_spire.obj';model(500,'DEVELOPMENT fallback central crystal',path,[0,0,0],'forest_spire',[.19,.21,.33],collision=path)
# Additional real outer trees use a separate RNG so the accepted inner scene stays exact.
# Broad radial/azimuth variation and species rotation avoid concentric fence rows.
outerRng=random.Random(310202603)
outerRings=[(132,60),(155,72),(180,84)]
outerIndex=0
for ring,(radius,count) in enumerate(outerRings):
 for i in range(count):
  a=2*math.pi*(i+(ring*.381)+outerRng.uniform(-.34,.34))/count
  r=radius+outerRng.uniform(-6,6)
  s=outerRng.uniform(.88,1.34)+(ring*.055)
  species=['pine_sparse','pine_dense','pine_crooked'][(i+ring)%3]
  pos=[r*math.cos(a),outerRng.uniform(-.35,.15),r*math.sin(a)]
  rot=[0,outerRng.uniform(0,360),0]
  ident=4000+outerIndex*16
  first=len(models)
  if REAL:imported(ident,species,pos,'forest_pine',[s,s,s],rot,collidable=False)
  else:model(ident,'DEVELOPMENT outer '+species,f'../models/forest_arena/fallback/{species}.obj',pos,'forest_pine',[.25,.29,.22],[s,s,s],rot)
  for item in models[first:]:item.update(outerForestRing=ring+1,decorative=True)
  outerIndex+=1
# Initial authored proposal; shared gameplay owner reviews spawn and navigation.
base['entities']=[{'type':'player_start','name':'South forest arrival','position':[0,0,60],'rotation':[0,-90,0]}]
for i,p in enumerate([[0,0,40],[40,0,0],[0,0,-40],[-40,0,0]]):base['entities'].append({'type':'rift','name':f'Forest rift {i}','slot':i,'position':p,'rotation':[0,0,0],'scale':[1.5,1.5,1.5]})
for i,p in enumerate([[-26,0,-15],[26,0,-15],[0,0,30]]):base['entities'].append({'type':'sentry_spawn','name':f'Forest sentry {i}','slot':i,'position':p})
base['lights']=[{'type':'point','position':[0,12,0],'color':[.61,.60,.89],'intensity':1.7,'range':135}]
nodes=[];links=[]
for ring,r in enumerate([24,48,72,90]):
 for i in range(32):
  a=2*math.pi*i/32;ident=ring*32+i+1;nodes.append({'id':ident,'position':[round(r*math.cos(a),5),0,round(r*math.sin(a),5)]});links.append({'from':ident,'to':ring*32+(i+1)%32+1,'bidirectional':True})
  if ring:links.append({'from':ident,'to':ident-32,'bidirectional':True})
route=[[0,0,48],[46,0,0],[0,0,-46],[-46,0,0],[0,0,48]]
base['spatial']={'mode':'triangles','navigation':{'nodes':nodes,'links':links},'regions':[{'id':'forest_clearing','name':'RUNESTONE CLEARING','min':[-100,-1,-100],'max':[100,145,100]}],'route':route}
(REPO/'res/maps/starry_crystal_arena.json').write_text(json.dumps(base,indent=2)+'\n')
(ROOT/'layout-state.json').write_text(json.dumps({'seed':3102026,'developmentPlaceholders':not REAL,'radius':100,'spireHeight':140,'nearTreeCount':48,'outerTreeCount':216,'outerRings':[{'radius':r,'count':c} for r,c in outerRings],'edgeSectors':12,'note':'Actual approved Tripo sources and per-bundle local collision.' if REAL else 'Pending actual approved Tripo sources; do not call this final imported art.'},indent=2)+'\n')
print('Generated forest map with '+('actual approved Tripo assets' if REAL else 'explicit DEVELOPMENT placeholders')+'; legacy unchanged.')
