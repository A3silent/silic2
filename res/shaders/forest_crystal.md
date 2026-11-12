# Forest crystal transparent material

`forest_crystal.frag` reuses `map.vert` (GLSL330). FOREST-ASSETS owns this material;
VECTOR owns renderer ordering, GPU copies, state management and App integration.
No mesh, source texture, collision, sky or ground shader is changed here.

## Pass contract

Render the opaque world, enemies and projectiles first. VECTOR then finds the
nearest visible crystal surface using a color-disabled depth pass and draws its
color with depth EQUAL and premultiplied surface plus independent emission blending (`ONE`,
`ONE_MINUS_SRC_ALPHA`). The GPU copy removes reverse duplicate triangles. Restore
all modified state before viewmodel and HUD. Foreground opaque geometry must win
the depth test; background color remains in the framebuffer and truly shows
through the crystal. The fragment shader never discards or writes gl_FragDepth.

| Input | Meaning |
| --- | --- |
| FragPos / Normal / TexCoord | Existing world-space map vertex outputs |
| cameraPosition | Actual world camera position |
| brushMinimum / brushSize | Entire crystal world AABB, size clamped positive |
| objectColor / texture1 / useTexture | Existing authored base-color factor and UV texture |
| time / phase | Paused/resettable simulation time and existing0–1 transformation |
| depthOnly | True: immediate color-zero return for cheap depth-only traversal |

The present140m crystal has an axis-aligned world AABB. If future content rotates
or scales a crystal differently, the interior field remains an AABB approximation;
this shader does not silently claim a fitted volume or arbitrary SDF.

## Appearance and limits

Six fixed view-ray samples integrate a broad internal core and three sparse
slanting light channels. World-normal facets and a restrained Fresnel edge retain
its angular silhouette. Broad blue-violet color and a dim original-UV rune
contribution avoid a white overexposed block. The fragment RGB is `surface * alpha + emission`; emission is independently
bounded and is not multiplied by opacity again. Scalar opacity varies roughly
0.49–0.78, with internal attenuation and stronger grazing edges. Animation is
slow and simulation-time driven; no random frame noise, additional shake or
bright flashes.

This is **stylized internal scattering**, not physical subsurface transport,
spectral transmission or multiple-interface refraction. It does not sample a
scene-color buffer or bend the background. Hardware blending provides real
background visibility; the AABB field provides only the internal light impression.
The shader does not create beams outside the model or new world lights. Existing
central-crystal scene lighting remains the environment implementation's source.
Nearest-surface-only composition intentionally avoids accumulating many translucent
layers from the generated cluster. Rear/internal surfaces are not individually
refracted; when viewed from inside, it remains a stylized readable material.

## Checks and review

`glslangValidator -S frag res/shaders/forest_crystal.frag` and linking with
`res/shaders/map.vert` pass. These are compile checks, not actual GPU acceptance.
VECTOR's runtime test must confirm background color transmission, foreground
opaque occlusion, stable opacity despite reverse render faces, restored GL state,
and pause/reset behavior. Actual320×200 review should inspect a near basal facet,
a mid-distance main column and the upper tip before/after transformation. Check
that forest/enemy shapes remain visible through broad faces while runes, facets
and internal light still read. No performance benchmark is requested.


## Integrated-view revision (2026-10-31, before density adjustment)

Initial real GPU pictures established background transmission but showed an
underlit glass surface. This revision separates radiance from surface alpha,
widens the core/three soft channels and restricts upper-peak integration length
instead of averaging light over the entire wide basal AABB. Geometry, collision
and alpha limits are unchanged. GPU image review and the blend-contract test
must use this revised shader and ONE / ONE_MINUS_SRC_ALPHA together.

## Observed integrated result (2026-10-31, before density adjustment)

VECTOR's actual GL run passes14/14 checks in
`artifacts/acceptance/forest-material-pass/render/checks.jsonl`, including real
background transmission, untouched foreground opaque markers, tinted visible
background markers, stable opacity with reverse source triangles, restored GL
state, simulation-time pause/reset, phase change and legacy-map no-op behavior.
Shader SHA256:
`e8f16f09dd829c050e65926f6fcb62d814db54c91910a1b5e9716ebdb563b5bd`.

NEBULA inspected the actual320×200 `actual-eye-spire.png` and
`actual-crystal-near.png` in that render directory after the premultiplied change.
The broad blue inner light now reads at both distances; dark violet facets and
transmitted forest silhouettes remain visible without a white emissive slab.
The three analytic channels merge into soft longitudinal internal light layers
at this resolution: the evidence does **not** establish three individually
countable bright beams. These are rendering checks and art review, not a human
combat playtest or performance measurement.


## Denser mineral body (2026-11-12)

User requested a more solid-looking crystal. Base opacity rises from0.23 to0.49,
with limits0.49–0.78 instead of0.23–0.60. Absorption and grazing-angle variation
remain; background contribution is consequently bounded to22–51percent before
other framebuffer processing. The slight reduction in Fresnel opacity weight
(0.20 to0.17) keeps edge/body separation without making edges fully opaque.

The premultiplied pipeline and independently bounded blue-core emission are
unchanged; this does not increase core brightness or alter the model/collision.
Only the alpha expression changed. Source/GLSL checks accompany this revision;
new GPU/App review is delegated to VECTOR alongside the current ADS integration.
The14/14 report and shader hash above describe the earlier transparent revision,
not acceptance of this new density adjustment. No new performance test or API call.
