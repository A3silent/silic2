# Forest Tripo assets — offline import contract

Status (2026-10-31): all four approved Tripo sources downloaded and imported.
Genuine previews, source/material data, the engine CPU bundle loader, actual JPEG
decoding, hashes and exact offline reproduction have been checked. Geometry and
collision are frozen for NEBULA/VECTOR scene and runtime checks; those checks are
not claimed by this asset-only record. See `asset-delivery.json`.

## Original design targets

| Runtime directory | Target height | Approximate spread | Source face target |
| --- | --- | --- | --- |
| pine_sparse | 22 m | 7 m | 2000 |
| pine_dense | 18 m | 8 m | 2500 |
| pine_crooked | 20 m | 12 x 11 m | 3000 |
| crystal_spire | 140 m | 12–18 m base | 4000 |

Reference: `tools/tripo/forest-prompts/REVIEW.md`. Full briefs and submitted prompts
are distinct; prompt hashes must match the approved source files. Original GLBs
remain under `artifacts/tripo/forest/raw`; do not modify source data in place.

## Receipt and inspection

1. Receive the actual task ID and local source path from ATLAS. Record GLB SHA256,
   size and approved prompt hash. Never call Tripo or request another conversion.
2. Run `python3 res/models/tripo/forest/inspect_glb.py ACTUAL_SOURCE.glb`.
   This uses the existing shared importer's parser, without changing that tool.
   Inspect scene-transformed bounds, triangle counts, material flags, UVs and
   embedded image metadata. Opaque-only, uncompressed static geometry is required.
   Reject unsupported alpha, skin/animation, morphs and compression; ask VECTOR
   for an importer fix only when an actual supported-source defect is found.
3. View the genuine generated preview and orthographic/turntable evidence before
   selecting rotation. Numeric longest axis alone does not establish root-up.
   Confirm a complete single tree/spire, root and tip, three distinct silhouettes,
   no attached ground platform, and foliage with real volume rather than alpha
   cards. Do not silently mark a wrong asset accepted.
4. Preserve uniform tree proportions. The actual spire requires the explicitly
   approved local narrowing documented below. After orientation, scale by target height divided
   by actual Y extent. `--size` means **longest** extent, not always height; use
   `--scale` when those differ. Use `--pivot floor`, giving minimum Y=0 and XZ
   bounding-box center at origin. The crooked tree's trunk may be offset from
   that origin; report its actual center to the scene author.
5. First import to a new review directory; the importer refuses overwrites.
   Command shape (replace placeholders, do not run literally):

   ```text
   python3 tools/assets/import_static_model.py ACTUAL_SOURCE.glb NEW_REVIEW_DIR
     --scale VERIFIED_UNIFORM_SCALE --pivot floor --rotate RX RY RZ
     --producer tripo-api --task-id ACTUAL_TASK_ID
     --prompt tools/tripo/forest-prompts/ASSET_ID/prompt.en.txt
   ```

6. Inspect extracted textures through an actual image decoder and visual preview,
   including orientation/UV seams and broad tonal range. Existing importer flips
   GLB V for the engine texture convention; do not flip it again. Preserve source
   texture bytes and hashes unless an explicitly documented offline derivative is
   necessary. No new generated texture, paid PBR processing or runtime dependency.
7. Verify all manifest paths remain package-relative, actual OBJ bounds/triangles
   match declarations, geometry/normals are finite, and the C++ bundle loader can
   read it. Report imported parts, per-part textures/material factors, source and
   runtime triangles separately. Double-sided source flags expand reverse faces;
   do not hide that increase or remove them without visual evidence.
8. After asset review, publish each new complete bundle at the runtime directory
   above. Do not create a pretend manifest pointing at absent geometry. Existing
   work is never overwritten; provenance includes original source, rotation,
   uniform scale and actual resulting dimensions.

## Runtime handoff

NEBULA consumes every manifest part as one map model with the same transform;
baseColorFactor.rgb goes to map color and baseColorTexture resolves relative to
its bundle. Target material names are `forest_pine` and `forest_spire`, with
neutral imported color and authored UVs. The environment shader provides sky and
central-crystal lighting, including dynamic rune emission; baked lights are not
required. Do not map all trees to the same geometry under different names.

Collision is authored separately from render triangles. Pine proxies match real
solid low trunks/roots, not a whole-crown box; high foliage is not a ground wall.
The perimeter's visible thickets and root masses are the environment author's
continuous boundary. The crystal proxy matches the real primary solid and base,
leaves no detached invisible extensions, and uses the same normalized frame as
its visual. Actual source topology determines the proxy, not the target prompt.
Any proxy touching navigation is reviewed with VECTOR. No automatic acceptance
of nonmanifold generated geometry as collision.

Delivery includes actual bounds, root/trunk offset, parts and triangle counts,
texture formats/dimensions, source hashes, import command and limitations.
Scene previews and functional checks are coordinated by ATLAS; no performance
benchmarks, no game-code edits, no API calls and no force-adding binary assets.


## Actual delivered assets

| Asset | Actual X/Y/Z size (m) | Source triangles | Render triangles | Collision triangles |
| --- | --- | --- | --- | --- |
| pine_sparse | 11.840 / 22 / 12.701 | 1976 | 3952 | 320 |
| pine_dense | 13.139 / 18 / 12.928 | 2461 | 4922 | 256 |
| pine_crooked | 17.104 / 20 / 16.556 | 2793 | 5586 | 256 |
| crystal_spire | 16.215 / 140 / 17 | 4065 | 8130 | 4120 |

All four are one material/part with genuine UVs/normals and one embedded JPEG
2048×2048 extracted byte-for-byte. Each source is OPAQUE and double-sided, so the
shared importer expands reverse render faces. No transparency/PBR/rig pipeline
was added. The three real tree shapes are visibly distinct but wider than the
requested targets; retain uniform proportions and place them using these bounds.

The spire source was much too broad: ordinary scaling to140m yields roughly
79×83m base width. ATLAS approved local X/Z narrowing by0.20478547854785478,
leaving height140m and maximum base width17m. Normals receive inverse-transpose
scaling and renormalization; UVs, texture bytes, triangle count and original GLB
are preserved. `artisticDerivation` records this exception in its manifest.
The generated rune markings are retained; animated brightness belongs to NEBULA.

## Final collision design

`collision.obj` in each asset package is the agreed entry point, in the same
normalized coordinate frame as the visual. It is deliberately separate from the
shared import manifest schema. Each final proxy is one closed, consistently
outward surface: every indexed edge has two opposite-directed incident faces.
These are actual file checks, not assumptions from the generation prompt.

- Crystal: retain the genuine nonconvex surface, deduplicate reverse render faces,
  split source T-junctions on their existing planes, remove four tiny attached
  flaps, and separate three coincident seam contacts topologically. Orient the
  resulting single closed component consistently; signed volume is positive.
  No convex envelope fills the spaces between base shards. Three source seam
  contacts remain geometrically coincident; this is not a claim that all generated
  geometry is globally free of self-intersections. Runtime contact checks matter.
- Trees: one connected root/main-trunk loft per species, measured from actual
  horizontal sections using16 azimuth rays. The old pine follows its real bent
  trunk. Main solid height is12m for sparse and8m for dense/crooked, inside visible
  wood/crown well above normal jump reach. There is no hidden standing cap at2.5m.
  The central root-neck outline atY0.65m extends down to the floor; independent
  thin root toes, secondary branches and needles are decorative. Root-neck
  interpolation is an intentional simplification, and reachable-side traversal
  must be checked by VECTOR for the12 near-edge trees. Do not claim every tiny
  visible root has its own collider.

The earlier overlapping convex/triangle-shell experiments were discarded. The
production rebuild entry now calls only the final surface repair and single-tree
loft builders; it cannot regenerate the discarded21k/32k triangle shell proxies.
Per-asset `collision-report.json` records sections, closure, counts and limits.

## Reproduction and evidence

```sh
# New output only; this script refuses to overwrite existing asset bundles.
python3 res/models/tripo/forest/import_assets.py --output-root /tmp/forest-review-new

# Authoring environment already exists from the earlier Tripo pass.
# numpy2.5.4/scipy1.18.1 are offline-authoring dependencies, never game dependencies.
artifacts/acceptance/tripo-interior/mesh-tools/bin/python res/models/tripo/forest/build_proxies.py
```

`import_assets.py` reproduces all four current manifests, geometry and textures
byte-for-byte from the genuine local GLBs and approved task/prompt records.
Rebuild collision only after acquiring file ownership again; scene authoring may
be reading these frozen files. `inspect_glb.py` is read-only and may be rerun.

`import-validation.json` records all four successful real C++ loader results and
JPEG decoding. `asset-delivery.json` records task IDs, source/manifest/collision
hashes, dimensions, counts and the spire transformation. Genuine generated
previews were inspected before import. Actual game screenshots and gameplay
checks belong to the coordinated scene/runtime acceptance, not this offline pack.
No performance benchmark or additional Tripo call was made by FOREST-ASSETS.
