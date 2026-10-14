# Cosmic horror presentation pass

User direction, 2026-10-08: darker space horror, especially the atmosphere of
White Knuckle and Eclipsium, with Devil Daggers as a combat reference. Monsters
should feel eldritch and difficult to classify. This revision supersedes the
bright cyan crystal art direction in the two-minute sample and role templates;
the sample's gameplay scope and acceptance gates remain in force.

Later user decision, 2026-10-08: retain this horror arena and all current creature
changes, and offer the original crystal arena separately. The default horror map
is `res/maps/hollow_orbit_arena.json` with preset `hollow_orbit`;
`res/maps/starry_crystal_arena.json` with preset `starry_crystal` restores the
pre-horror environment. Both share the current enemies, weapon, HUD palette,
audio and gameplay. The art direction below applies to Hollow Orbit; preserve the
original crystal geometry, lighting and shaders for the crystal preset.

## Direction

An abandoned orbital excavation structure has become an organism. Near-black
mineral, oxidized metal, bone-colored seams and dry tissue meet enormous distant
structures disappearing into a starless void. Existing central/side crystal meshes
read as fossilized organs or calcified growth, retaining their actual collision
geometry. Space is suggested by scale, a sparse sky and an occluded celestial mass.

The first pass must visibly change shape language, materials, motion and sound.
A global exposure reduction alone does not satisfy the request. Use broad areas
of darkness with controlled local illumination, rough surfaces, offset ribs,
holes and incomplete silhouettes. Preserve floor/cover separation and the running
loop at the existing 320x200 internal resolution. Decoration remains subordinate
to threats. Use original procedural assets; referenced games supply principles,
not copied characters, textures, logos or sound clips.

Palette: charcoal blue/green-black for space; oxidized desaturated browns/grays for
architecture; pale bone for creature surfaces and weapon wear; sparse sickly
green for dormant organic details. Amber/vermilion remains exclusive enough to
read attack charge, locked aim, incoming projectiles and spawn warnings. Hit
feedback is brief cold/ivory light, distinct from sustained warm attack cues.

## Deliverables and boundaries

- NEBULA: sparse threatening sky; colossal ribbed/industrial silhouettes outside
  the playable volume; mottled mineral/metal floor and cover; ossified center;
  irregular rift apertures; slow, restrained tissue-like movement. The existing
  timed transformation reveals an incompatible second structure or changes the
  apparent celestial presence. Keep all authored floor, obstacles, slots, light
  anchors, timings and transforms authoritative. No camera obstruction, route
  changes, forced shake, whiteout or new collision geometry.
- KINETIC: Chaser remains tall/narrow within its 0.6x1.8x0.6 body envelope: a
  hollow asymmetric rib-cage with folded appendages and a vertical facial cleft.
  Sentry remains squat within 1.2x1.0x1.2: an open calcareous cup/organ with a
  recessed aperture that gathers and locks its attack. Make their silhouettes
  visibly different at gameplay scale. Small presentation-only contractions
  express movement/charge; center of mass and shot origins remain honest. The
  existing gun becomes a worn mechanical instrument with restrained bone/tissue
  inserts, clear barrel and sightline. Replace jewel-like debris/slide chips with
  appropriate fragments/dust; retain bounded effects and actual gameplay cues.
- ECHO: original dark industrial/organic ambience and transformation, rougher
  bodily warning/charge textures, restrained UI/result cues. Preserve exact cue
  durations, spatial information, priority/ducking, pause/reset and clipping
  protections. Ambient texture must leave room for attacks. Preload as before.
- PRISM: understated bone/gray/dirty amber interface, less luminous cyan. Retain
  typography, hierarchy, minimap visibility filtering, state text and all controls.
  No decorative distortion over aiming information or changing text layout.
- VECTOR: integrate public changes only when necessary, update presentation
  identity/content version, verify unchanged gameplay/map compatibility, perform
  final runtime and performance regression after specialists finish.

ATLAS owns this direction, role pointers, scheduling, visual review and delivery
record. Scope is the existing arena and two enemy types; new AI, hitboxes, weapons,
sanity mechanics and rewards are outside this pass. The map preset's existing
serialized name may remain for compatibility. Saved user maps remain readable.

## Verification and handoff

1. Each specialist hands over changed paths, deterministic previews or audio
   measurements, resource limits and integration needs. Serialize actual GL and
   device runs through ATLAS; CPU/offline work can proceed in parallel.
2. Capture actual scene before/after with comparable camera/time. Inspect READY,
   combat and transformation plus both enemy charge/hit states and first-person
   weapon. Darker mood must retain ground/cover boundaries, distinct enemy forms,
   charge lock and projectile cues. Visual inspection is not a human playtest.
3. Run appropriate existing module tests and Debug/Release regressions. Verify
   default and legacy map loading, pause/reset and GL errors. The real runtime
   flow retains all assertions. Check output matches the final binary and assets.
4. Final Release performance uses the established three-run uncapped setup with
   actual audio: p95 <=20ms, p99 <=33.3ms, no three consecutive frames >50ms.
   Stop tests when required checks pass; repeat only after relevant fixes.
5. Archive representative actual screenshots and evidence under docs/acceptance;
   label subjective human readability/listening pending. This request does not
   automatically publish another Git commit or push.

## Reference observations

Primary source pages consulted 2026-10-08:

- [White Knuckle / Dark Machine Games](https://store.steampowered.com/app/3195790/White_Knuckle/):
  its concrete megastructure, decay and precision movement suggest heavy material
  and scale while keeping movement targets identifiable.
- [Eclipsium / Critical Reflex](https://www.criticalreflex.com/eclipsium):
  a sunless, self-consuming world, pale guiding light and a looming presence
  suggest diseased space, sparse illumination and environmental unease.
- [Devil Daggers / Sorath](https://store.steampowered.com/app/422970/Devil_Daggers/):
  an abyssal arena with fluid movement and retro rendering supports the combat
  constraint: atmosphere must coexist with rapidly recognizable threats.

The specific orbital-organism setting and creature proposals above are Silic2
design decisions derived from these references, not descriptions of those games.
