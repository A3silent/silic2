# Forest crystal arena — accepted concept implementation

User approved implementation 2026-10-31 based on the revised cloud-veiled circular-eye concept.
This supersedes the previous Crystal orbital/megawall layout, not the shared combat rules.
Approved concept: artifacts/acceptance/forest-arena/approved-concept.png.

- Circular arena diameter approximately 20 seconds at current 10m/s sprint = 200m.
- Single colossal central crystal spire, about140m high, irregular twisted fictional runes.
- Elaborate flat circular engraved/drawn totem beneath it, no raised altar.
- Limestone with sandy/gravel mineral pattern, broad open fighting floor.
- Surrounding forest with three distinct pine silhouettes, dense shrubs/roots; no masonry perimeter wall.
- Keep original Crystal cosmic sky background, add one giant round watching eye in the NORTH (-Z)
  framed by layered swirling clouds, per approved second concept. No eye tracking gameplay required.
- Environment illumination only from sky ambience and central crystal; existing combat cues remain readable.
- Retain enemy/weapon/movement/camera-lean/audio,90s generation+cleanup, pause/reset and seed rules.
- Replace the current Crystal entry with the forest map, preserve its prior map as a separate legacy preset/file.
  Hollow stays unchanged. No puzzle, pickups or new progression.
- Use Tripo for required original model assets, initial batch3 pine types+spire, source/raw provenance retained.
  Long full briefs and bounded actual API prompts with negative constraints; low-resolution large forms,
  not photoreal microdetail. ATLAS alone owns online generation/credit calls. Do not spend before prompt
  review: the user's prior explicit request to inspect all prompts before generation remains applicable.
- Sky eye/clouds, runes/totem and ground shading may use scene code/shaders rather than unnecessary3D assets.
- Scene boundary must correspond to visible woodland/trunks/thickets, not invisible masonry walls.
- Extend map-scoped viewing distance and spatial/nav coverage as needed; adapt spawn layout to large arena
  without silently changing combat values. Functional traversal/contacts/clearance, editor roundtrip,
  old-map compatibility, actual320x200 views and brief gameplay checks. No performance benchmarks per user.

One writer per file; runtime capacity4 incl ATLAS; asset binaries stay local, no commit/push.

Implementation delivered 2026-10-31: real Tripo assets, forest preset and editor integration,
96 final functional checks, editor roundtrip and10 App flow checks passed. See
`docs/forest-crystal-arena.md` for scope, evidence, credits and playtest limitations.

User material revision 2026-10-31: eye/nearby clouds should use nebular color gradients
closer to approved concept; central crystal should become translucent with self-emission
and stylized internal scattering rays; ground sigils should be warped and visibly pixelated
rather than straight radial lines. Engine transparent-pass extension is authorized.
Geometry/collision/outer forest remain unchanged; no new Tripo generation or performance tests.

User rendering revision 2026-10-31: try a640×400 internal framebuffer and reduce exterior
forest rendering costs after observed slowdown. Preserve placement, collision and current art.
This request permits short controlled measurements to diagnose the reported slowdown; it does
not reopen the full milestone stress benchmark. Separate resolution cost from forest optimization,
prefer resource sharing/batching/culling before visual simplification, and report measured limits.

Follow-up: user selected480×300 as the next default internal resolution to try; retain the
completed forest resource-sharing and instancing optimization.

User revision 2026-11-13: remove every decorative black ground rune/totem/pattern,
retaining the natural floor material. Make the forest arena much darker, with a
strong luminous central crystal. Preserve distant scene silhouettes rather than
restricting visibility to a small circular pool. Use matching pale crystal-blue
weapon energy, player projectiles, player aura and nearby rising particles.
Restore the earlier upward-floating particle character around the player, with
bounded emission, pause/reset/map-switch lifecycle and no aiming obstruction.
Represent first-person self-emission through luminous weapon seams, continuous
cool light on nearby surfaces and rising blue particles; no new hands/body model
is requested. This explicitly adds player light to the earlier sky/crystal-only
lighting direction and supersedes the ground engraving requirement. Keep the
forest layout/collision/navigation, 480x300 rendering, mesh instancing/culling,
0.03s firing interval, 0.75m visual bullets, speed195 and existing combat rules.
Hollow environment remains unchanged; the shared rifle/projectile palette may
follow the new crystal-blue direction in either map. No paid assets or benchmark.

User revision 2026-11-13: open the skyline completely by removing the authored
trees, woodland roots and shrubs, and procedural distant pine silhouettes. A
low visible rock rim preserves arena containment without forest collision.
Make the eye and stars clearly visible against the dark sky, and frame the
eye clear of the crystal at arrival. Use an original generated tile of dark
blue-gray slate on the floor, with no symbols or vegetation. Preserve the
central crystal, ground dimensions, combat layout, player glow and other maps.

User revision 2026-11-17: make the sky eye about 15% smaller, with a gentle
local tremor and contraction around its existing anchor. Stars vary their
brightness independently while staying fixed in the sky. Preserve the eye's
readability, dark palette and timed transformation. The effects follow combat
time so pause freezes them and a restart reproduces their starting frame.

Follow-up: run the eye motion at 20% of its previous rate and use 20% of the
previous translation and contraction amplitudes, for movement that is barely
perceptible. Keep its resting size and star twinkling unchanged.
