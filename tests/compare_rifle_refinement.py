#!/usr/bin/env python3
"""Compare actual pre/post generator dumps, not authored scale constants.
Usage: compare_rifle_refinement.py BASELINE_JSON CURRENT_JSON REPORT_JSON
Dumps contain the expanded vertices/faces/color of buildProceduralRifle().
The upper-frame spatial extrema exclude the lower support. Gun-body checks
exclude only the old/new optic-base envelope and high optic tip geometry.
"""
import collections
import json
import math
from pathlib import Path
import sys

before, after = (json.loads(Path(p).read_text()) for p in sys.argv[1:3])

def part(model, color):
    matches = [p for p in model['parts'] if max(abs(a-b) for a,b in zip(p['color'],color)) < 1e-5]
    assert len(matches) == 1, f'Unique material {color} required'
    return matches[0]

def triangles(p, indices=None):
    indices = p['faces'] if indices is None else indices
    return [tuple(tuple(round(c, 8) for c in p['vertices'][i]) for i in indices[n:n+3])
            for n in range(0,len(indices),3)]

def projected_frame(model):
    p = part(model,(.11,.10,.14))
    # The lower narrow support cannot control the frame width or upper extent.
    vertices = [v for v in p['vertices'] if v[1] >= .095]
    assert vertices, 'Actual frame vertices above the retained eye anchor required'
    tan = math.tan(math.radians(70)/2)
    pixels = [(240+150*x/((.5-z)*tan), 150-150*(y-.095)/((.5-z)*tan)) for x,y,z in vertices]
    return [max(p[0] for p in pixels)-min(p[0] for p in pixels), 150-min(p[1] for p in pixels)]

old_size, new_size = projected_frame(before), projected_frame(after)
ratio = [n/o for n,o in zip(new_size,old_size)]
checks = [{'name':'actual_upper_frame_projected_width_and_height_above_anchor_60_percent',
           'passed':all(abs(v-.6)<.035 for v in ratio),
           'before_pixels':old_size,'after_pixels':new_size,'ratios':ratio}]
old_dot=before['parts'][before['reticlePart']]
new_dot=after['parts'][after['reticlePart']]
def reticle_geometry(p):
    vertices=p['vertices']
    center=[sum(v[i] for v in vertices)/len(vertices) for i in range(3)]
    local=[tuple(v[i]-center[i] for i in range(3)) for v in vertices]
    tan=math.tan(math.radians(70)/2)
    pixels=[(240+150*x/((.5-z)*tan),150-150*(y-.095)/((.5-z)*tan)) for x,y,z in vertices]
    bounds=[(min(v[i] for v in pixels),max(v[i] for v in pixels)) for i in range(2)]
    return local,[(lo+hi)/2 for lo,hi in bounds],[hi-lo for lo,hi in bounds]
old_local,_,_=reticle_geometry(old_dot)
new_local,center,diameter=reticle_geometry(new_dot)
shape_kept=len(old_local)==len(new_local) and all(abs(a-b)<1e-7 for ov,nv in zip(old_local,new_local) for a,b in zip(ov,nv))
checks.append({'name':'reticle_local_shape_center_and_projected_readability',
               'passed':shape_kept and abs(center[0]-240)<1e-5 and abs(center[1]-150)<1e-5 and all(1.5<=d<=3.5 for d in diameter),
               'projected_center':center,'projected_diameter':diameter,
               'note':'Allows the reticle depth to move with the resized frame; compares its shape relative to its own center.'})
# Exclude the optic base spatial envelope and high pointed tips. This does not
# depend on the generator's triangle ordering or fixed per-beam triangle counts.
for label,color in [('armor',(.065,.060,.078)),('facet',(.17,.163,.20)),
                    ('edge',(.34,.33,.39))]:
    def body(model,p):
        result=[]
        optic_vertices={tuple(round(c,8) for c in v) for v in part(model,(.11,.10,.14))['vertices']}
        for triangle in triangles(p):
            optic_tip = label=='edge' and all(v[1]>.095 for v in triangle)
            optic_base = label in ('armor','facet') and all(abs(v[0])<=.04 and .04<=v[1]<=.09 and .04<=v[2]<=.12 for v in triangle)
            optic_facet = label=='facet' and all(v in optic_vertices for v in triangle)
            if not optic_tip and not optic_base and not optic_facet:
                result.append(triangle)
        return collections.Counter(result)
    original=body(before,part(before,color)); revised=body(after,part(after,color))
    removed=sum((original-revised).values()); added=sum((revised-original).values())
    checks.append({'name':label+'_gun_body_outside_optic_base_unchanged','passed':removed==0 and added==0,
                   'removed_triangles':removed,'added_triangles':added})
report={'source':'actual buildProceduralRifle geometry before and after; 70-degree viewmodel projection',
        'checks':checks,'passed':all(c['passed'] for c in checks)}
Path(sys.argv[3]).write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
raise SystemExit(0 if report['passed'] else 1)
