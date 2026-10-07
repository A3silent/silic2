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

**Arena template** opens the game's standard arena as a new unsaved document. Save As chooses a new path and leaves the template untouched. **Open** offers a folder browser, a path field, Game maps, Parent and Home shortcuts. File selection is not committed until Open or a double-click. A failed load preserves the current document.

## Editing

- Click a visible brush, object or marker in the 3D viewport, or select the same item in the left list. The selected bounds are outlined in cyan, including when occluded. Double-click a list item or press **F** to focus it.
- Hold **right mouse** over the viewport to look and fly with **WASD**, **Q/E** down/up and **Shift** for extra speed. Release right mouse to edit. The UI remains visible. The wheel dollies the camera over the viewport.
- Drag or type **Position** and **Size** in the Inspector. With Snap enabled, values snap when the edit ends. **Snap to grid** snaps immediately. Arrow keys move in world X/Z; Page Up/Down move in Y. Shift increases the nudge by ten grid steps.
- **Cmd/Ctrl+D** duplicates; **Delete/Backspace** deletes. New geometry is a true solid box or a horizontal plane. Planes have zero Y size; legacy flat mesh axes remain flat. Geometry rotation is intentionally unsupported.
- The Add menu keeps player start, light entity, enemy spawn, item pickup, trigger, point light, directional light and spot light creation. The Inspector edits geometry appearance, entity transform/basic properties and light color, intensity, range, direction and cones. Anchored light movement changes its relative offset.
- Map settings expose gravity, ambient/background colors, environment preset, transformation time/duration and particle count. Gravity is a compatibility field saved in the map; it does not override the current game player gravity (the player still uses PlayerConfig.gravity). Arena templates preserve required player/rift/sentry slots; rejected edits show the shared validator's field/path error.
- **Cmd/Ctrl+Z** undoes; **Cmd/Ctrl+Shift+Z** or **Cmd/Ctrl+Y** redoes. One drag is one command. History keeps 128 commands, including selection. Saving creates a savepoint, so undo/redo restores the correct dirty state.
- **Cmd/Ctrl+S** saves; **Cmd/Ctrl+Shift+S** opens Save As. An asterisk marks unsaved work. New, Open, template and Quit show **Save / Discard / Cancel** when needed. Save As confirms replacing another existing file. Save writes a temporary sibling and renames it after validation; failure preserves the previous file and current document.

Text fields retain normal typing and their text undo shortcuts. Save remains a document shortcut. Camera controls run only while right mouse navigation is active, and stop on focus loss.

## Preview scope

The preview shows geometry with simple colors, grid, entity/light markers and selection. It does **not** reproduce the game's textures, final lighting, crystal effects or combat. Logical UI sizes and actual framebuffer sizes are handled separately for Retina screens. Rendering resources are destroyed before the GL context.

## Reproducible checks

```sh
ctest --test-dir tools/map_editor/build --output-on-failure
./tools/map_editor/build/map_editor --self-test res/maps/textured_room.json /tmp/editor-legacy-check
./tools/map_editor/build/map_editor --self-test res/maps/starry_crystal_arena.json /tmp/editor-arena-check
./tools/map_editor/build/map_editor --smoke res/maps/textured_room.json /tmp/editor-gl-check
./tools/map_editor/build/map_editor --smoke res/maps/starry_crystal_arena.json /tmp/editor-arena-gl-check
```

`--self-test` requires no GL context. It exercises actual document commands, legacy flat vertex transforms and resizing, texture/topology preservation, cloning with unique IDs, undo/redo across savepoints, failed load/save, extension metadata and reload through the game's shared loader. The arena variant edits a cover and verifies required-marker rejection and template Save As.

`--smoke` creates the real editor/ImGui/OpenGL window, injects synthetic pointer/key events through the shared UI path, exercises selection, movement, duplicate, undo/redo, the unsaved dialog and resize, then writes `edited.json` and PPM framebuffer screenshots. It is an automated rendering/input check, **not a human usability test**. Run GPU checks sequentially with the game. Test outputs go only to the requested output directory; input fixtures are never overwritten.
