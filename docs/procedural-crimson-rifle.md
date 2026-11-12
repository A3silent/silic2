# Crimson Carapace — procedural rifle

Implemented from the user-approved black/red concept on2026-11-12. The reference is
`artifacts/acceptance/procedural-rifle/approved-concept.png`.

## Geometry and source selection

`src/character/procedural_rifle.cpp` builds the entire rifle in CPU code: faceted chassis,
six repeated gill openings, overlapping armor shards, curved magazine, grip/trigger guard,
pointed stock, split muzzle and open-top`\_/`optic. Layered stock plates, beveled end faces
and a dorsal red channel keep near-camera surfaces readable. No hands are generated.

The current model contains1288triangles,3864expanded vertices and8material groups. Two groups
are energy seams/cores; a separate reticle group is unlit and excluded from bloom. The static
vertex buffers total123648bytes before other renderer resources. Geometry is generated once
and cached on the GPU, with no per-frame rifle mesh generation or sorting.

`buildProceduralRifle()` is deterministic and performs no file, texture, network or GL access.
`resolveWeaponPresentation()` is the shared runtime/test selector. The default JSON selects
`source: "procedural"`; absent, invalid or disabled optional configuration also resolves to
the new rifle. Only explicit`source: "imported"`requests the retained legacy import adapter.
Unavailable legacy resources fall back to the new procedural model.

All default mesh/texture/manifest path fields are empty. The approved concept is reference
artwork, not a texture applied to the gun. `ImportedWeapon` remains the name of the reusable
cached mesh renderer; its use does not imply that the default gun imports external art.

## Material and sight

The weapon material uses a fixed black/charcoal palette, cool facet highlights, stylized
local red bounce, broad scarlet seams and pale red-hot cores. A bounded neighborhood in the
private viewmodel compositor supplies a small red halo. This is weapon-local presentation,
not a global world-lighting or bloom change.

The private target encodes empty/opaque/glow-source information in alpha. Composition restores
opaque coverage and preserves the scene's depth. The red aiming disk is not a bloom source;
the open aperture stays clear. Internal framebuffer textures are generated render targets,
not authored image-texture dependencies.

Hold RMB to aim; release to return. The existing0.16-second/32-degree ADS, sensitivity scaling,
center-ray shooting, axial recoil, pause/reset rules and480×300internal resolution remain.
The normal hip pose moves down0.012units to improve near-camera framing. Full-ADS alignment
is unchanged: the red disk covers4pixels centered at(240,150), including sliding/recoil.

## Ownership and verification

KINETIC owns geometry, weapon fitting and material/composite shaders. VECTOR integrates the
build and functional checks; ATLAS owns scope, image review and serial GPU scheduling.

ATLAS reviewed side, three-quarter, hip and ADS views, requested one targeted refinement to
the large armor planes and emissive cores, and accepted the final art. The dedicated preview
also checks firing, private-depth/GL-state preservation, missing-config initialization and
160×100→480×300target resizing. Evidence is in
`artifacts/acceptance/procedural-rifle/final-art/`.

CPU checks exercise the actual builder and selector: deterministic vertices/materials,
finite unit normals, valid winding/indices, nondegenerate triangles, exact bounds, bounded
mesh size, absent external-art paths,100ADS anchor poses, default/missing/invalid/disabled
configuration, and explicit legacy OBJ compatibility. Existing ADS CPU firing/HUD checks
cover centered shots, cooldown, hip parity, near obstacles and hit markers.

There were no model-service calls, performance benchmarks or Git publication in this task.
The concept sheet represents visual intent; final in-game screenshots show the actual
low-resolution engine result.

Final integrated verification: Debug and Release builds and both procedural-rifle/ADS CPU
targets passed. The formal App route passed17/17checks, including moving continuous fire,
pause/release, death/restart and Hollow/forest switching. It runs the actual App with synthetic
input in a hidden native window and explicitly calls the production focus callback; OS focus
event delivery and human weapon feel are not covered by this route. Internal rendering was
480×300, output2560×1440Retina, VSync/audio enabled.

ATLAS reviewed final`app/hip.png`,`app/ads.png`and`app/ads-moving-fire.png`and independently
verified all128protected map/model/player/enemy/audio/environment files unchanged. Evidence:
`artifacts/acceptance/procedural-rifle/atlas-verification.json`, the App JSONL and build logs.
Both`./silic2`and`./silic2-release`were rebuilt; restart the game to use the new default rifle.

## Smaller sight and continuous energy channels

User requested a40percent smaller sight and repairs to two interrupted lines marked in
actual screenshots. The original optic geometry is scaled to0.6about the existing eye anchor;
a narrow adapter joins its raised base back to the gun. The red disk keeps its readable radius,
and its depth follows the reduced housing. The ADS anchor and weapon pose remain unchanged.

The red/green loop uses the actual mesh renderer at480×300with bright-red8-connectivity,
excluding dim bloom. It reproduced component gaps of19pixels in hip view and63pixels in ADS.
The receiver/stock and dorsal/rear-cap paths had separate geometric endpoints; some strips
also lay behind raised armor. The correction joins each main route, places it outside its
visible armor surfaces, and shares miter vertices at bends, with per-point normals around the
rear bevel. Separate gill openings retain their intended shape.

The same fixed-view checks now find a connected bright path in both marked regions, with
component gap0. Material shading, bloom strength and real depth testing are unchanged; no
through-armor overlay is used. ATLAS reviewed the repaired hip/ADS/side views. Dedicated
preview checks also cover centered red dot, recoil, private-depth/state restoration and target
resize. The earlier1222triangle/117312byte budget describes the initial version; the updated
budget is listed above. Evidence:`artifacts/acceptance/crimson-refinement/`.

Final refinement verification: actual upper-frame projection width57.65→34.03pixels and
upper height19.37→11.44pixels, about0.59034after perspective (geometry scale0.6). The red dot
remains centered at(240,150), with a2.25pixel projected diameter. Armor/Facet/Edge triangle
sets outside the sight are unchanged; dark-red channel backing is part of the allowed path
repair. Mesh/pose comparisons passed5/5, Debug/Release and both CPU targets passed, and the
final actual-App synthetic route passed17/17. ATLAS reviewed its hip/ADS screenshots and
independently confirmed128protected files unchanged. GPU work ended; restart the rebuilt game.

## Sight width adjustment — 2026-11-12

The user requested 35% more width relative to the reduced sight. The housing,
red frame insets and mounting support now scale by 1.35 along X around the same
anchor. Height and depth are unchanged; inverse-transpose scaling and normalization
preserve correct facet normals. The reticle is generated afterward and stays circular.

Actual generated geometry comparison passes 8/8 checks: 258 sight vertices receive
only the intended X change, while all 3,570 gun-body vertices, normals and indices
remain exact, including the continuous energy routes. Upper-frame projection at
480×300 increases from 34.03395 to 45.94584 pixels; height above the anchor stays
11.43639 pixels. Triangle count, material groups and static VBO bytes stay unchanged.

Matched before/after GL previews cover hip, ADS, recoil, side, three-quarter and
firing. ADS and recoil retain four reticle pixels centered at (240,150), continuous
bright energy paths, correct private depth/state restoration and target resizing.
ATLAS inspected the actual hip/ADS and side comparisons. Evidence is under
`artifacts/acceptance/crimson-sight-width/`.

This follow-up authorizes a local Git version of the current source, including the
previously delivered forest, camera lean and ADS work. Models, textures, sounds,
captures and build outputs remain local under the existing repository asset policy.

Final verification passed Debug and Release builds, normal game tests, procedural
rifle checks and ADS CPU checks. The real Release App synthetic route passed17/17
checks at480×300internal /2560×1440Retina output, with VSync and audio enabled.
ATLAS reviewed the final hip, ADS and moving-fire captures and independently checked
the source/binary hashes and raw App check results. All1,042baseline files outside
the seven assigned source/documentation changes remain unchanged. The only additional
shader edit removes whitespace from a blank line, with identical shader tokens.
Compact versioned evidence: [sight width acceptance](acceptance/sight-width-2026-11-12.json).
This is automated runtime validation; human feel, OS focus delivery and other
platforms remain untested in this pass. No performance benchmark was run.

## Tuning

Screenshot follow-up: the side and bottom sight rails are now 30% thicker, with
matching 30% wider red Energy/Core insets. The previous widened frame centerlines,
depth, mounting support, tip geometry, centered dot and gun body are preserved.
The production change is three width parameters in `sight()`. Actual generated
geometry checks pass7/7; matched hip/ADS/recoil previews retain the centered dot
and continuous energy lines. Evidence: `artifacts/acceptance/crimson-frame-thickness/`.

The 2026-11-13 upload snapshot preserves the subsequent local sight tuning:
housing width0.0163, Energy inset width0.00432 and Core width0.00145 before the
existing assembly transforms. The 30% figures above describe the earlier iteration.
Player projectiles now use red emission, a0.75m visual length (2.5times the0.3m
base mesh) and a0.03-second firing interval. Bullet width, speed195, damage and
swept collision are retained. The interval is half the previous0.06seconds;
the existing per-frame firing/cooldown sampling is unchanged.

- Shapes and material palette:`src/character/procedural_rifle.cpp`.
- Facet shading and red bounce:`res/shaders/character_imported.frag`.
- Local halo radius/strength:`res/shaders/character_overlay.frag`.
- Weapon pose:`src/character/weapon_asset.cpp`; preserve full-ADS sight centering.
