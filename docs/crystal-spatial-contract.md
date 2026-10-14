# Crystal static spatial contract

Implementation owner: VECTOR. Map/assets: NEBULA. Editor: PRISM.
This contract accompanies `plan/crystal-combat-world.md`; gameplay remains
`arena_survival`, with its four rifts and three sentry slots. Asset style does not
select gameplay. Version 1/2 maps retain their previous collision and navigation.

## Map selection and authoring

Use `schemaVersion: 3` and `spatial.mode: "triangles"` explicitly.
`environment.architecture: "crystal_combat"`, `landmarkPosition` and
`landmarkScale` describe presentation. Coordinates are meters, Y up.

Existing `geometry.objects` support box, plane and the new ramp, with center
`position`, full dimensions `size`, `rotation: [x,y,z]` in degrees and
`collision: true|false`. Ramp slopes rise from local +Z at -height/2 to local -Z
at +height/2. They are closed wedges. Rotation order is T * Rz * Ry * Rx.

`geometry.models` contains instances with unique geometry `id`, `name`, `asset`,
`collisionAsset`, `position`, `rotation`, positive `scale`, `material` and `color`.
An empty collisionAsset explicitly means decoration. Otherwise collision uses the
proxy's actual triangles. It may reference the same file as asset. No mesh AABB
is substituted for its openings. Instance transforms use T * Rz * Ry * Rx * S.

Assets use static Wavefront OBJ: v/vt/vn, triangle or convex quad f records,
positive or negative indices. Quads must be planar and strictly convex; concave,
self-crossing or twisted faces fail atomically. Export triangulated meshes for predictable results.
Objects/groups/smoothing/material declarations are ignored; material and color
come from the instance. No animated meshes, MTL textures or arbitrary polygons.
Collision walkable faces must wind outward/upward. Visuals compile to Brush
triangles with flat face normals. A multi-material model can be split into instances.

Paths resolve against the loaded map's directory. `loadFromJson(document, base)`
sets a source base explicitly; the one-argument overload preserves the Map's base.
Editor transactions must copy their current Map or pass its base. `saveToFile`
rewrites model/proxy references relative to the destination map, allowing an
independent fresh load outside the repository. It does not copy assets or make
an exported map self-contained.

## Navigation and diagnostics

`spatial.navigation.nodes` are `{id, position:[x,y,z]}` foot positions.
`links` are `{from:id, to:id, bidirectional:true}`. Different elevations require
separate IDs even at identical XZ. Links explicitly connect ramps, corners and
levels. Do not connect a bridge deck directly to the surface under it.

Links are validated against actual support and actor clearance all along the
segment. Unsupported links do not enter the runtime graph. Recommended main
routes: width >=4m, slope <=35 degrees, headroom >=2.3m, upper/lower separation
>=4m. Runtime limits are 512 nodes and 2048 links. Main paths need no jumps;
player-only drop/jump shortcuts can omit links while providing an enemy detour.

`spatial.regions` are `{id,name,min:[x,y,z],max:[x,y,z]}` for presentation and
inspection. `spatial.route` is a sequence of 3D foot positions for a reproducible
synthetic patrol, not a gameplay completion rule. `arena.floorBrushId` remains a
horizontal reference for overall bounds; it is not a single-height collision or
navigation surface. Spawn validation uses actual support and graph access.

Invalid resources, transforms and references fail transactionally. Support,
clearance and connectivity diagnostics remain loadable for editor repair; the
application rejects a playable map containing these diagnostics.

## Shared API

See `include/engine/map.h` and `include/engine/spatial_world.h` for exact signatures.
Map exposes visual `getBrushes()`, `getModels()`, `getCollisionBrushes()` and
`getSpatialSettings()`, immutable `getSpatialWorld()` and
`getSpatialDiagnostics()`. Collision Brush IDs match object/instance IDs.

SpatialWorld provides bounded triangle ray/sphere casts, height-aware support,
upright actor movement, supported/clear traversal and 3D graph path queries.
Triangle bounds accelerate candidate selection only. World queries are consumed
by players, enemies, shots and line-of-sight logic so openings agree.

Both game and editor compile `src/engine/map.cpp`, `simple_json.cpp`,
`mesh_asset.cpp` and `spatial_world.cpp`. No second editor importer is needed.
