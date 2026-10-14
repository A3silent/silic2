# Silic2 Map Editor

A small ImGui / GLFW / OpenGL editor for the Silic2 map format. The game and editor compile the **same Map and JSON implementation**; there is no separate editor parser. Legacy meshes, texture references, entity properties and unknown extension fields survive editing and saving.

## Build and launch

Run these commands from the game repository root after cloning the complete repository.

```sh
cmake -S tools/map_editor -B tools/map_editor/build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build tools/map_editor/build -j 4
./tools/map_editor/build/map_editor
```

Dependencies: CMake, a C++17 compiler, OpenGL and GLFW. The minimal ImGui source subset is included; see [dependency provenance](DEPENDENCIES.md). `SILIC2_ROOT` defaults to the repository root (`../..` relative to this directory); override with `-DSILIC2_ROOT=/path/to/silic2`. GLM and shared Map / JSON sources come from that tree. On macOS, GLFW can be supplied by Homebrew.

The existing executable entry remains `tools/map_editor/build/map_editor`. It works from any working directory. Pass an absolute or relative map file to open it immediately:

```sh
/path/to/silic2/tools/map_editor/build/map_editor /path/to/map.json
```

**Crystal template** opens the game's standard arena as a new unsaved document. Save As chooses a new path and leaves the template untouched. **Open** offers a folder browser, a path field, Game maps, Parent and Home shortcuts. File selection is not committed until Open or a double-click. A failed load preserves the current document.

## Editing

- Click a visible brush, object or marker in the 3D viewport, or select the same item in the left list. The selected bounds are outlined in cyan, including when occluded. Double-click a list item or press **F** to focus it.
- Hold **right mouse** over the viewport to look and fly with **WASD**, **Q/E** down/up and **Shift** for extra speed. Release right mouse to edit. The UI remains visible. The wheel dollies the camera over the viewport.
- Drag or type **Position**, local **Size** (primitives), **Scale** (models), and **Rotation** in the Inspector. Geometry drags update a lightweight visual preview; release commits one shared validation and one undo command. The status reports validation time. Snap applies on release: world grid for positions/sizes, 0.1 for model scale, configurable degrees for rotation. Arrow keys move in world X/Z; Page Up/Down move in Y. Shift increases nudges tenfold.
- **Cmd/Ctrl+D** duplicates; **Delete/Backspace** deletes. Add creates a solid box, plane, or ramp (local +Z low, -Z high). Planes keep zero local Y size and may rotate. Legacy meshes preserve their original topology and flat axes. Creating ramps/models or rotating primitives explicitly enables schema 3 triangle spatial queries; existing v1/v2 maps otherwise retain their mode.
- The Add menu keeps player start, light entity, enemy spawn, item pickup, trigger, point light, directional light and spot light creation. The Inspector edits geometry appearance, entity transform/basic properties and light color, intensity, range, direction and cones. Anchored light movement changes its relative offset.
- Map settings expose gravity, ambient/background colors, environment preset, transformation time/duration and particle count. Gravity is a compatibility field saved in the map; it does not override the current game player gravity (the player still uses PlayerConfig.gravity). Arena templates preserve required player/rift/sentry slots; rejected edits show the shared validator's field/path error.
- **Cmd/Ctrl+Z** undoes; **Cmd/Ctrl+Shift+Z** or **Cmd/Ctrl+Y** redoes. One drag is one command. History keeps 128 commands, including selection. Saving creates a savepoint, so undo/redo restores the correct dirty state.
- **Cmd/Ctrl+S** saves; **Cmd/Ctrl+Shift+S** opens Save As. An asterisk marks unsaved work. New, Open, template and Quit show **Save / Discard / Cancel** when needed. Save As confirms replacing another existing file. Save writes a temporary sibling and renames it after validation; failure preserves the previous file and current document.

Text fields retain normal typing and their text undo shortcuts. Save remains a document shortcut. Camera controls run only while right mouse navigation is active, and stop on focus loss.

## Hybrid authoring

Use **File → Import static OBJ** or **Cmd/Ctrl+I** to browse `.obj` assets. The shared importer accepts static OBJ positions/UVs/normals and triangle or convex quad faces, including negative indices. Export triangulated, outward-wound, Y-up meshes in meters. Each instance uses one material/color; OBJ materials, textures, animation and arbitrary polygons are not supported. No vertex editing is provided.

Choose **Use visual mesh for collision**, or disable it and supply a separate collision OBJ. An empty collision path means decoration. Curves and irregular openings come from the actual mesh triangles. Imported instances appear in **Static models**; their pivot is the source model origin. **Edit resource paths** replaces visual/proxy references transactionally. A missing asset, malformed mesh or invalid transform leaves the document unchanged and displays the shared loader error.

**Add object → Navigation node** creates a 3D foot position. Set its position in the Inspector, enter a target node ID, then **Connect to target**. Links can be bidirectional; remove links in **Navigation links / diagnostics**. Deleting a node removes its attached links in the same undoable command. Move existing player/rift/sentry markers through the same Inspector. The shared diagnostics identify unsupported nodes, blocked links, disconnected routes and invalid spawns. These problems remain editable, but the game rejects a map that still has spatial diagnostics. Geometry edits do not automatically move navigation nodes.

Save As rewrites visual/proxy references relative to the destination directory via the shared Map saver. Reopening the saved map outside the repository works while the original assets remain at their referenced locations. Assets are **not copied or embedded**; this is not a portable package export. Unknown metadata, regions, authored routes and old exploration settings remain preserved. The template remains unchanged unless you explicitly save over it.

## Preview scope

The preview shows geometry with simple colors, grid, entity/light markers and selection. **View settings → Layer** selects visual triangles, collision triangles, or a wireframe navigation overlay. Collision uses shared compiled proxies, not a replacement bounding box. Navigation node IDs and links retain their distinct Y levels; links are drawn as an inspection overlay through geometry. It does **not** reproduce the game's textures, final lighting, crystal effects or combat. Logical UI sizes and actual framebuffer sizes are handled separately for Retina screens. Rendering resources are destroyed before the GL context.

## Reproducible checks

```sh
ctest --test-dir tools/map_editor/build --output-on-failure
./tools/map_editor/build/map_editor --self-test res/maps/textured_room.json /tmp/editor-legacy-check
./tools/map_editor/build/map_editor --self-test res/maps/starry_crystal_arena.json /tmp/editor-arena-check
./tools/map_editor/build/map_editor --smoke res/maps/textured_room.json /tmp/editor-gl-check
./tools/map_editor/build/map_editor --hybrid-self-test /tmp/editor-hybrid-check
./tools/map_editor/build/map_editor --measure-edits res/maps/starry_crystal_arena.json /tmp/editor-edit-cost
./tools/map_editor/build/map_editor --hybrid-smoke res/maps/starry_crystal_arena.json /tmp/editor-hybrid-gl-check res/models/crystal_combat/ring_deck_30.obj
```

`--self-test` requires no GL context. It exercises actual document commands, legacy flat vertex transforms and resizing, texture/topology preservation, cloning with unique IDs, undo/redo across savepoints, failed load/save, extension metadata and reload through the game's shared loader. The arena variant edits a cover and verifies required-marker rejection and template Save As.

`--smoke` creates the real editor/ImGui/OpenGL window, injects synthetic pointer/key events through the shared UI path, exercises selection, movement, duplicate, undo/redo, the unsaved dialog and resize, then writes `edited.json` and PPM framebuffer screenshots. It is an automated rendering/input check, **not a human usability test**. Run GPU checks sequentially with the game. Test outputs go only to the requested output directory; input fixtures are never overwritten.

`--hybrid-self-test` verifies ramps, local transforms/world bounds, OBJ instancing, invalid-resource transactions, external Save As/reopen/edit and navigation link history. `--measure-edits` records full-map CPU transaction costs (not GUI frame times) in `edit-cost.csv`. `--hybrid-smoke` drives the real file dialogs and Inspector through synthetic ImGui input, verifies import/rotation/scale/duplicate/history/ramp creation/external Save As and Open, then tests deferred position/rotation drags on the full loaded map. It writes screenshots and raw `ui-frames.csv`, including import/commit/screenshot spikes. Neither script substitutes for human usability testing. Run GPU checks only in the allocated serial window.

`--layout-smoke MAP OUTPUT` checks a selected model’s full-width TRS fields and real rotation drag/undo at 1440×900 and 800×600. It requires the same serial GPU scheduling as other smoke modes.
