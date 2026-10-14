# Crystal editor validation — 2026-10-13

Owner: PRISM. Scope: `tools/map_editor/**`; shared Map/OBJ/spatial implementation
is VECTOR's code. No game HUD or adjacent old editor was edited. No commit/push.

## CPU coverage

CTest covers legacy geometry/texture metadata, v2 arena, Hollow, the preserved
exploration fixture, new hybrid authoring, and the complete Crystal combat map.
The hybrid case exercises ramp local dimensions versus rotated world bounds,
OBJ instances/TRS/duplication/history, missing resources and zero-scale rollback,
external Save As/reopen/continued edits, and navigation node/link history.
The full-map case compares every compiled visual/collision vertex and face after
external saving and shared reload, including navigation counts and game mode.

Build/test logs, screenshots, raw CSV and failed attempts are archived by ATLAS
in [the editor evidence directory](../../docs/acceptance/crystal-combat/editor/).
The original staging directory is `/tmp/silic2-crystal-editor-acceptance/`. The delivered editor
build uses RelWithDebInfo; Debug is checked separately. The final rebuild includes
VECTOR's convex/coplanar quad validation patch. Shared loader validation changes
are independent of the UI layout adjustment described below.

## Actual ImGui/OpenGL checks

`gui-attempt-3` completed successfully on the frozen input map SHA256
`001ac8d47a498c5e3262b34d97269c4c9a0e5dea44574b9f7a3581d9910e50ac`:
85 navigation nodes, 23 original model instances. Tested binary SHA256 was
`0ce541c0c273787f5e3edc092b65c35d8cb933e16fc2d3cc0a2c5cad2fc3d01a`.

The script opened the real editor and injected synthetic ImGui pointer/keyboard
input through the normal controls: OBJ file import; rotation and scale buttons;
copy/undo/redo; a new ramp and undo/redo; Save As outside the repository; Open of
that saved file; two Inspector drags with live geometry previews; and exactly
one commit/one undo per drag. The full map remained present throughout. The
working copy gained two instances and a ramp before the drag measurements.

The logical windows were 1440×900 and 800×600; actual Retina framebuffers were
2880×1800 and 1600×1200. A separate successful `legacy-gui` run verified picking,
arrow movement, copy/history, the unsaved-work dialog, Cancel after resize and
shared save/reload. All captured GL error checks passed; both processes exited 0.

Screenshots in both folders were actually inspected. Navigation links are an
intentional through-geometry inspection overlay; zoom/focus is needed to separate
crowded node labels in the overview. The preview uses simple mesh colors and
neither game textures nor final atmosphere/combat effects.

`gui-attempt-1` and `gui-attempt-2` are retained failed attempts: the script used
Ctrl+A in a macOS text field, so an absolute Save As path appended to the default
filename. The save failed transactionally. Correcting only the harness to use the
macOS Super modifier produced the passing third attempt. No source map was lost
or overwritten. A failed local PNG conversion between attempts is not GL evidence;
PNG conversion ultimately used macOS `sips`.

ATLAS's image review requested a bounded layout follow-up: move Position/Size/
Scale/Rotation labels above full-width three-component fields and wrap the live
preview notice. `--layout-smoke` checks each field stays inside the Inspector,
performs a rotation drag/undo and captures a selected model at both sizes. The final `layout-final` run exited 0; all fields passed the Inspector-boundary
checks at 1440 and 800, both rotation drags remained previews until release,
and each single undo restored the exact original JSON. The three captured images
were checked, including the selected model at 800. Final binary SHA256:
`c79a79f375bcbf436b4f3f19ef099508b4b13f7d9164324db25282932e186eb2`.

The first layout run is preserved separately as `layout-attempt-1`. Its test
incorrectly expected `revision == oldRevision + 1` after an undo; revisions use a
monotonic serial while undo restores an older revision. The harness now checks
revision advances on release, remains unchanged during the drag, and exactly one
undo restores the original JSON. No application behavior was changed to satisfy
that assertion. Both final builds passed all seven CTest cases; the delivered
binary's final repeat is in `ctest-delivered.log`.

## Measured response, with spikes retained

`gui-attempt-3/ui-frames.csv` contains all 147 raw frame durations. Its
`timing-summary.json` is derived without deleting screenshot/validation spikes:

| Segment | Median | Maximum |
| --- | ---: | ---: |
| Position drag, 12 frames | 3.1555 ms | 42.3877 ms (screenshot frame) |
| Rotation drag, 12 frames | 3.32935 ms | 34.6806 ms (screenshot frame) |
| Position release/validation frame | 73.8417 ms | same single frame |
| Rotation release/validation frame | 74.9317 ms | same single frame |
| Entire scripted run | — | 240.059 ms |

These are short editor interaction measurements, not game performance or human
usability results. The preview performs no shared map rebuild while dragging;
release performs one validated transaction. Earlier Debug measurements on a
144-node greybox took about 2.9 seconds per transaction and motivated deferring
validation. Historical 148/85-node CPU samples used different map/build states;
they are diagnostic only and must not be merged with the final GUI sample.

## Boundaries

Static OBJ only: triangulated or strictly convex coplanar quads, positive scale,
one material/color per instance. No animated assets, MTL texture rendering or
vertex DCC. Save As rebases references but does not copy/embed asset files.
Navigation and spawn warnings remain editable; they are not waived for game
launch. Geometry edits do not automatically reshape navigation. Human mouse/
keyboard usability and Windows/Linux execution remain unverified. GPU ownership
was handed back to VECTOR after the main editor run; the small layout follow-up
also completed in its separately authorized window, and its GL process exited.

Final frozen-map CPU transaction sample (`final-cpu`): 85 nodes / 23 models,
12 operations, median 85.1017 ms and maximum 114.919 ms. The input snapshot,
build identity, CSV and hash list are retained. This binary differs from the
final layout binary only in the corrected layout-harness revision assertion.

ATLAS independently reviewed the final 800-wide selected-model screenshot and
accepted the layout. Editor GL processes have exited; this delivery is frozen.
