"""Visible authored foundation behind the SE rib structure, inside the real floor."""
from pathlib import Path
import math
ROOT=Path(__file__).resolve().parent/'foundation';ROOT.mkdir(exist_ok=True)
def write(name,vertices,faces):
 s=['# Original low orbital retaining foundation, not a generated Tripo asset.']
 s+=['v '+' '.join(f'{x:.7f}' for x in p) for p in vertices]
 s+=['f '+' '.join(str(i+1) for i in q) for q in faces]
 (ROOT/(name+'.obj')).write_text('\n'.join(s)+'\n')
def box(name,a,b):
 x,y,z=a;X,Y,Z=b;v=[(x,y,z),(X,y,z),(X,Y,z),(x,Y,z),(x,y,Z),(X,y,Z),(X,Y,Z),(x,Y,Z)];quads=[(0,3,2,1),(4,5,6,7),(0,4,7,3),(1,2,6,5),(0,1,5,4),(3,7,6,2)];write(name,v,[q for a,b,c,d in quads for q in [(a,b,c),(a,c,d)]])
def arc(name,begin,end,height):
 v=[];f=[];steps=max(1,math.ceil((end-begin)/7.5))
 for i in range(steps):
  a=math.radians(begin+(end-begin)*i/steps);b=math.radians(begin+(end-begin)*(i+1)/steps);start=len(v)
  v += [(r*math.cos(t),y,r*math.sin(t)) for y in [0,height] for r,t in [(28,a),(29.4,a),(29.4,b),(28,b)]]
  for q in [(0,3,2,1),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7)]:
   aa,bb,cc,dd=q;f += [(start+aa,start+bb,start+cc),(start+aa,start+cc,start+dd)]
 write(name,v,[(a,c,b) for a,b,c in f])
arc('east_shell_return',0,18,10.5)
arc('south_shell_return',65,90,11.9)
box('rib_back_east',(30.5,0,8.0),(31.8,3.5,29.5))
box('rib_back_south',(11.5,0,28.2),(31.8,3.5,29.5))
box('rib_north_return',(26.4,0,8.0),(31.8,3.5,9.3))
box('rib_south_return',(11.5,0,25.2),(12.8,3.5,29.5))
