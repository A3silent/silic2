# Map format v2

The game and `tools/map_editor` compile the same `src/engine/map.cpp` and
`src/engine/simple_json.cpp`. `include/engine/map.h` is the shared public interface.
There is no separate editor serializer. JSON is an authoring format; compiled
brushes and navigation are derived at load time.

## Root document

| Field | Meaning |
| --- | --- |
| `schemaVersion` | `2`; missing means the legacy v1 input adapter |
| `gameMode` | `legacy` or `arena_survival`, required in v2 |
| `environmentPreset` | `none` or `starry_crystal`, required in v2 |
| `contentVersion` | Content identity for run records, independent of schema version |
| `worldSettings` | `gravity`, `ambientLight`, `backgroundColor` retained from v1 |
| `geometry.brushes` | Existing authored mesh arrays |
| `geometry.objects` | Parametric boxes and horizontal planes |
| `entities` | Player/enemy/item/trigger markers and typed arena spawn slots |
| `lights` | Authored lights, optionally anchored to geometry or an entity |
| `arena.floorBrushId` | Geometry ID defining the arena floor and XZ extent |
| `environment` | `transformationStart` (default 45), `transformationDuration` (4), `particleCount` (256) |

Positions use world units with Y up. `geometry.objects[].position` is the center,
and `size` is full XYZ size. Entity spawn positions are feet positions. Entity
rotations are XYZ Euler degrees. Game physics parameters retain the existing
player configuration; the legacy `worldSettings.gravity` field is preserved but
does not override that configuration.

`arena_survival` keeps this sample's fixed encounter rules: one player start,
four `rift` slots 0–3 and three `sentry_spawn` slots 0–2. Slot identity connects
the director, HUD and effects; cardinal labels do not force fixed world positions.
All spawn markers must be on and inside the arena floor, with geometry clearance.
The encounter schedule remains gameplay code in this iteration.

## Geometry

Example object entry (a fragment of `geometry.objects`):

```json
{
  "id": 7,
  "name": "West cover",
  "shape": "box",
  "position": [-9, 0.7, 0],
  "size": [2, 1.4, 4],
  "type": "wall",
  "material": "arena_living_crystal",
  "color": [0.1, 0.4, 0.5]
}
```

Boxes require positive XYZ dimensions. Planes use `shape: "plane"`, positive X/Z
dimensions and zero Y size; `type: "floor"` or `"ceiling"` selects their facing.
Parametric object rotation is unsupported and rejected. Existing arbitrary mesh
vertices are retained through `geometry.brushes`; do not silently convert those
meshes into boxes. Mesh `vertices`, `texCoords` and `faces` remain flat numeric
arrays. Geometry IDs are unique across both arrays.

The arena's explicit floor must be horizontal. Its bounds determine arena extent;
other relevant brush bounds determine navigation obstacles. Moving an authored
object updates its compiled mesh and collision/navigation input together. The
visual mesh is not an editable second copy of the parametric object.

## Light anchors and extensions

An optional light `anchor` contains exactly one of `brushId` or `entityName`, plus
`offset: [x,y,z]`. Brush anchors resolve from the geometry bounds center; entity
anchors resolve from the entity position. Offsets are world-space units, not
scaled local coordinates. Missing or ambiguous references are rejected. Use
`getSourceLights()` for editing and `getLights()` for resolved rendering positions.

Unknown extension fields survive load/edit/save at root, world, geometry, object,
brush, entity and light levels. Known fields are validated, including vector
lengths, finite numbers, triangle indices, unique IDs and enum values. Malformed
JSON, unsupported versions and invalid references fail with an error location.

This authoring implementation bounds spatial components to ±100000 world units,
JSON arrays to 200000 entries and arena navigation obstacles to 64. Zero-area
triangles are rejected. These are explicit load constraints, not silent clamps;
normal sample dimensions are far below them. Arena navigation uses bounded
geometry-derived waypoints rather than iterating over the entire world extent.

## Compatibility and mutation

Ordinary v1 maps retain legacy gameplay. The published
`contentVersion: "starry-crystal-arena-v1"` has a dedicated migration: named
`rift_north/east/south/west` and `sentry_0/1/2` trigger entities become typed slots,
preserving authored positions. This migration does not infer gameplay from
materials. It requires the original complete marker set.

`loadFromFile` and `loadFromJson` validate a temporary document before replacing
the current map. On failure the previous scene remains usable, and
`getLastError()` explains the problem. Editors can modify a `toJson()` draft and
submit it through `loadFromJson`, or use the typed update methods. Never mutate
`getBrushes()` to edit compiled objects. The editor records complete document
states for undo/redo, including metadata and selection.

After accepting a new map, the game rebuilds GPU resources and the navigation
definition from it. Edits are loaded through the normal map-loading path; this
iteration does not add live game hot reload or dynamic topology during combat.

Implementation status and cross-program evidence: [upgrade record](map-editor-upgrade.md).
