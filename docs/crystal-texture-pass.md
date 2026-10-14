# Crystal texture pass

User request (2026-10-13): review surfaces that would benefit from textures and
assign original texture generation to NEBULA. This pass follows the accepted
Crystal combat world; no geometry or gameplay change is needed.

| Priority | Surface / material | Texture contribution | Keep in shader |
| --- | --- | --- | --- |
| 1 | Foundation, bridge, ramps / combat_foundation, combat_deck | Worn orbital metal plates, broad seams, restrained abrasion; shared family at controlled world scale | Region palette, traversable bridge edge inlays, lighting, rift warnings |
| 2 | Leaning supports, pale walls, low cover / combat_ivory | Mineralized pale ceramic/alloy, broad hairline fractures and erosion patches | Form shading, local illumination and phase tint |
| 3 | Dark outer shell / combat_brown | Oxidized dark structural panels, layered erosion, restrained alien contamination | Large regional color contrast, shadow and transformation |
| 4 | Keel, vault, invaded shell / combat_crystal | Coarse internal crystal cleavage and dark fractures | Mesh facets, restrained emissive seams and animated phase |

The aim is to distinguish material construction and wear, not increase random
surface noise. At 320x200, broad low-contrast motifs should survive while enemy
silhouettes, warm warning cues and projectiles remain dominant. Sky and animated
warnings are not replaced with static images. Texture repetition and oblique
sampling must be inspected in actual GL, including native 320x200 captures.

NEBULA owns original generation with the built-in imagegen tool, workspace assets,
complete prompts/provenance, Crystal shader/renderer integration and fixed-camera
before/after validation. Generated source images are preserved beside runtime
textures. Any conversion/downsampling is documented. Static OBJ geometry currently
contains no UV records, so world/surface projection must be deliberate. Current
combatSurface also bypasses main's texture-multiplied base; successful file loading
alone does not demonstrate actual surface sampling.

Acceptance: inspect four runtime tiles and repeated previews; prove material
sampling in before/after images; preserve missing-texture fallback, dynamic phase
and rift cues; compare Hollow against baseline; record texture memory/draw calls
and required build checks. Only NEBULA owns renderer changes in this pass.

## Delivery and review

NEBULA generated all four original images using the built-in imagegen tool; no
CLI/API fallback or procedural replacement was used. The unmodified outputs are
in [originals](../res/texture/crystal_combat/originals). Complete generation prompts:
[metal](../res/texture/crystal_combat/prompts/metal_deck.txt),
[ceramic](../res/texture/crystal_combat/prompts/mineral_ceramic.txt),
[shell](../res/texture/crystal_combat/prompts/dark_shell.txt),
[crystal](../res/texture/crystal_combat/prompts/crystal_cleavage.txt).
The runtime PNGs are BOX-area-downsampled 64×64 images; no manual painting or
programmatic texture synthesis replaced the generated originals.

| Material | Runtime asset |
| --- | --- |
| Metal deck / foundation | [metal_deck-64.png](../res/texture/crystal_combat/metal_deck-64.png) |
| Mineral ceramic | [mineral_ceramic-64.png](../res/texture/crystal_combat/mineral_ceramic-64.png) |
| Dark shell | [dark_shell-64.png](../res/texture/crystal_combat/dark_shell-64.png) |
| Crystal cleavage | [crystal_cleavage-64.png](../res/texture/crystal_combat/crystal_cleavage-64.png) |

The Crystal material profile binds four cached textures. Three world-space
projections blend by surface normal, with 5–8m repeating scale and bounded
mean-normalized albedo modulation. Existing broad procedural bands are reduced
only when a texture is active. Phase tint, crystal emission, deck edge inlays and
rift warnings remain dynamic. Missing assets use the previous material fallback.
Neither map schema nor collision/navigation changes are required.

ATLAS reviewed original images, reduced/repeated tiles and actual GL screenshots.
The materials show wear and fractures while the near Chaser silhouette and warm
Sentry cue remain distinguishable in the fixed actor view. This is visual review,
not human combat/readability testing. Tiles are not claimed mathematically seamless.

![Runtime tiles](acceptance/crystal-textures/runtime-contact-sheet.png)
![Fixed production actor view after textures](acceptance/crystal-textures/checks/actors-after.png)

Validation:

- [GPU controls](acceptance/crystal-textures/checks/sampling.jsonl) replaced each
  uploaded texture with flat data, observed changed pixels, then restored it to
  identical output. All four materials are sampled, not merely loaded.
- [GL checks](acceptance/crystal-textures/checks/validation.log) passed per-material
  toggles, missing-file fallback, production actors, freeze and unchanged Map.
- ATLAS independently checked [68 protected files and 3 Hollow PPM comparisons](acceptance/crystal-textures/atlas-review.json);
  all are unchanged. Geometry, gameplay, actors, weapons and audio are retained.
- [Debug](acceptance/crystal-textures/build-debug.log) and
  [Release](acceptance/crystal-textures/build-release.log) applications rebuilt.
- No new geometry or draw calls. Four 64×64 RGB textures with mipmaps use 65,532
  bytes of nominal RGB texel data (driver storage may differ).
- [Paired fixed-view render samples](acceptance/crystal-textures/checks/paired-render-summary.json):
  140 samples per mode, median 0.885ms without textures and 0.893ms with textures.
  This short render diagnostic does not re-certify full-game performance; previous
  isolated long-frame findings remain applicable.

Launch the updated map from repository root:

```sh
./silic2-release res/maps/starry_crystal_arena.json --seed 42
```

No commit or push was requested for this pass.

NEBULA的[素材说明与来源](../res/texture/crystal_combat/README.md)、
[技术验证](../src/environment/validation/TEXTURES.md)、
[最终冻结hash](acceptance/crystal-textures/final-input-hashes.json)均已归档并核对。
