# Procedural Crimson Carapace rifle

User approved implementation of the final concept on2026-11-12.
Reference:`artifacts/acceptance/procedural-rifle/approved-concept.png`.

## Accepted design

- Black, faceted overlapping armor with strong large-scale silhouette.
- Long segmented handguard, repeated red gill-shaped energy openings and a split muzzle.
- Angular receiver, curved magazine, distinct grip and pointed stock.
- Bright crimson seams with a hot core and localized glow; adjacent dark panels catch red light.
- Open-top`\_/`optic with a small floating red dot in the aperture center. No top bridge.
- No hands or arms. Readable hip and ADS views at480×300.

## Implementation limits

Generate the complete default rifle mesh and material data in code. Cache geometry at
initialization; no Tripo model, external mesh or image-texture dependency on the default path.
The concept image is reference artwork, not a runtime texture. Existing import support and
source assets may remain for explicit compatibility; they must not provide the new default.

Use the current private viewmodel depth/composite pipeline where practical. Emission and
limited weapon-local glow may extend this pipeline; do not introduce global world bloom or
change environment lighting. Keep the sight aperture clear and the red dot centered during
movement, sliding and recoil. Pause freezes visual time; restart restores the idle appearance.

Keep current ADS input/transition/FOV/sensitivity, shooting, collision, bullet speed195,
movement, enemy presentation, audio, maps, crystal density and forest optimization.
No model-generation calls, performance benchmark, commit or push.

KINETIC owns procedural art/rendering; VECTOR owns build/integration/functional tests;
ATLAS owns acceptance and serial GPU scheduling. Validate mesh structure and determinism,
the absence of default external-asset dependencies, actual hip/ADS/fire views, centered dot,
world-depth/GL-state preservation and current App lifecycle checks.

User refinement: reduce the sight assembly's main geometry by40percent around the existing
aim anchor, retaining a readable centered dot and a narrow mounting support. Connect the
user-marked receiver/stock and dorsal/rear-cap energy routes into continuously emissive
surface-following lines in both hip and ADS views. Preserve body scale and real depth occlusion.

User follow-up on 2026-11-12: increase the current reduced sight width by35percent
only, preserving height, depth, the centered reticle and the rest of the rifle.
Validate the result and create a local Git version. This explicitly authorizes a
commit for this delivery; the repository's source-only asset policy still applies.
