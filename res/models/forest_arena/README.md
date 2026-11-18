# Forest crystal arena authoring

This directory owns the deterministic Crystal layout and its explicit development
fallback. The current map is treeless: a low mineral rim contains the arena, the
original central crystal remains, and the renderer adds the [slate tile](slate-texture.md)
to the floor. `forest_crystal` remains the serialized preset for compatibility.

## Rebuild

Run from the repository root:

```sh
python3 res/models/forest_arena/generate.py
```

The default generator requires the central crystal Tripo manifest and collision
file. It checks these before writing anything and refuses to overwrite the map
if either is missing. Only explicit `--development` permits placeholder output. During
asset authoring, wait for the asset owner's approval of collision files: file
existence alone does not certify that a proxy has been accepted.

The output is `res/maps/starry_crystal_arena.json`; `layout-state.json` records
whether it references actual assets or explicit placeholders. The previous Crystal
megastructure is preserved byte-for-byte in `res/maps/crystal_megastructure_arena.json`
(SHA256 `2fe44617d6aa298ac4be2b05f62e35f17c4b6b4a55e7744b04d219eb10710f14`).
Both maps share a directory so the archived map's relative assets remain valid.

## Historical forest scene and provenance

The following describes the earlier wooded revision. The live map and generator
no longer reference the trees, roots, or forest silhouettes.

- Open limestone clearing radius approximately 100m; flat floor 220 × 220m.
- Real central crystal 140m high, 16.2145 × 17m footprint. The approved local
  nonuniform silhouette correction, original GLB and normal transform are recorded
  by the asset owner in the crystal bundle. Runtime fictional runes and restrained
  blue-violet facet emission are shader work, not baked Tripo light.
- 48 original inner trees: 16 each of sparse, dense and crooked pine, using authored heights
  22/18/20m with uniform instance scales 0.85–1.21. Actual crown widths are about
  12.7/13.1/17.1m before instance scale. Twelve trees stand at radius 100–103m to
  expose trunks between roots; the others form the next layer around 108–119m.
- Woodland contact consists of 12 local sectors of uneven, overlapping root and
  shrub volumes. Main masses reach roughly 3.8–5.4m, interspersed with lower
  branching forms. The same outward-facing triangles are visible and collidable;
  there is no hidden cylinder or masonry perimeter. Main root sides remain
  vertical to a varying 3.0–4.3m shoulder, then narrow into irregular crowns.
  This prevents movement contacts from walking players up the original sloped
  roots and jumping beyond the floor. The initial failure is retained in evidence.
- Distant forest uses 384 deterministic opaque crossed silhouettes in one static
  environment VBO, in three rows around radii 135/177/219m. These are original
  low-detail approximations, not more full Tripo meshes. Near trees retain their
  actual texture and shape. Tree collision uses one closed trunk/root loft per asset (320/256/256
  triangles, following real wood to 12/8/8m). Fine detached root tips, branches
  and needles are decorative; no low invisible cutoff platform is added.
  The crystal uses 4,120 triangles repaired into one closed outward surface,
  preserving the actual mineral silhouette. See each bundle collision report.
- Environmental lighting comes from one central crystal point source and sky
  ambient. Near pines receive extra diffuse sky response in their material branch;
  no hidden lamp or global lighting change is introduced. Existing combat warnings
  retain their own emission and temporary lighting.
- Flat floor engravings combine six concentric systems of paired rims, fictional
  branching glyphs, beads, broken spokes, interlaced radial seals and an outer
  undulating circuit. There is no raised altar or additional floor collision.
- Sky keeps the Crystal directional star cells and nebula ribbons. Exactly one
  fixed northern circular iris/pupil sits in irregular layered cloud folds.
- Separate `forest_crystal` preset and `forest_*` shaders scope this direction.
  The old Crystal and Hollow presentation paths remain available.

## Navigation and validation

The map uses 128 navigation points on rings of radii 24/48/72/90m. The player
starts south at `(0,0,60)`. Four rifts are at cardinal radius 40m; three Sentries
start near radius 30m. Combat values are unchanged. The outer clearing is much
larger than enemy projectile reach, so moving to its edge can reduce pressure;
this accepted limitation is not an AI tuning claim.

Scene preview: `src/environment/validation/forest_preview.cpp`.
Evidence: `artifacts/acceptance/forest-arena/`. Each native 320 × 200 image is
captured from the actual renderer; the elevated diagnostic view is explicitly
labelled and is not a gameplay camera. Fixed-time repeat, phase variation,
restart and legacy-map switching are checked by the preview.

`development-cpu` describes the earlier placeholder version (72 checks, including
190m sprint in 19.0003s). `real-visual-review-map.json` is an intermediate image
review map with provisional collision, not final gameplay proof. Final map and
actual collision require the dedicated final functional report.

No performance benchmark, extra online generation, commit or push is performed
by this generator or this environment pass.

Original inner-scene placement freeze: `c1282e8628420df2e635c3a24117eb4b0c397f891c00acd6799d63d47e3f1a0f`.
`placement-report.json` in the evidence directory records asset hashes, all tree
transforms and static counts. That accepted inner scene contains 61 model instances, 128 nav
points and approximately 251,012 visible near-scene triangles including retained
reverse faces, plus the single batched far forest. These are resource counts,
not a measured frame-time claim.

Final functional validation after the visible boundary correction: **96/96 passed**
(`final-cpu/forest-report.json`), including all eight sprint-jump directions retained
at approximately radius 98–101m, actual model contacts, navigation and editor
roundtrip. Earlier failed escape trajectories and root meshes are preserved under
`final-cpu/initial-failure/`. Boundary OBJ hashes are listed in `placement-report.json`: a
map JSON hash alone does not distinguish geometry revisions.

## Three additional outer forest layers

The current generator adds **216 real Tripo pines**, bringing the forest to 264
actual trees and the map to 277 model instances. Three nominal radii of 132, 155
and 180m contain 60, 72 and 84 trees respectively. Each radius varies by up to 6m;
angular offsets, independent yaw, slight root embedding and uniform scales
0.88–1.45 break up the rows. Each species gains 72 instances. These are the same
three real textured assets, not more silhouette cards. Existing far silhouettes
remain as intermediate filler and the last backdrop beyond the real trees.

Every added outer tree has an empty collision asset and lies beyond the accepted
woodland boundary. The original 61 model records and every other map field remain
exactly unchanged. Existing collider resource hashes are unchanged. The independent
outer RNG seed `310202603` cannot perturb inner placements or boundary geometry.

Current map SHA256:
`cc195fbfdb1d7b7e1dba8daaf2b689d7677c3702324bcb94612f15907320ddab`.
Evidence: `artifacts/acceptance/forest-outer-rings/verification.json` records static
preservation, exact repeat generation and circle counts. The dedicated
`src/environment/validation/forest_outer_preview.cpp` loads the actual map and
captures matched player-height edge/panorama and labelled elevated diagnostic
views under `before/` and `after/`. This pass changes data only; restarting or
reloading the map is sufficient, with no game recompilation required.

No additional Tripo generation or performance benchmark was run for these layers.
