# Editor dependency provenance

## Dear ImGui

- Upstream: https://github.com/ocornut/imgui
- Local checkout commit: `d4f722d5b289d4d75b775acfb0a4b41e7df472a7`.
- Version declared by the imported `imgui.h`: `1.92.4 WIP` (`IMGUI_VERSION_NUM=19231`).
- Imported from the existing editor checkout on 2026-10-07. Its Git working tree was clean.
- License: MIT; full original notice is in [imgui/LICENSE.txt](imgui/LICENSE.txt).
- Only four core translation units, their required headers, GLFW/OpenGL3 backends and the backend GL loader are vendored. No Git history, examples, demos, build products or optional FreeType implementation is included.
- The imported bytes are unchanged from that local checkout. SHA-256 values follow.

| Imported file | SHA-256 |
| --- | --- |
| `imgui/imgui.cpp` | `7cc2ebe5babfa68283e6c5f8a478545742a56dd3e1b271965e3b93e205d8bdd6` |
| `imgui/imgui_draw.cpp` | `4abc55ee4407dadcfea3504d06c40b5c2c251ff64948aa5226c8cd70635ba573` |
| `imgui/imgui_tables.cpp` | `1fa2ea427b67dfbfd714a1e0fbafc232f52151e1b31afec895bdd3f2c38c8432` |
| `imgui/imgui_widgets.cpp` | `11bb28adf740bd4a396f58bdbcc00e96a15cd66e838f335435c444cba4fe9382` |
| `imgui/imgui.h` | `9fb23267beb6bf8093c71e84e75d2e8c2c9aaaf94fa8b178d33c1082c388d299` |
| `imgui/imgui_internal.h` | `ff49cadac39787af3ee5bc39e8b663c2445a72a84f976bd7c4f74a38ad8fe13d` |
| `imgui/imconfig.h` | `a92534d918030048328fffcf0a50236b8c05f79558355b37e34b664e13c71091` |
| `imgui/imstb_rectpack.h` | `2efa3d5f7d003c19743b15155dad9f46d9f9fc783a18893d703b431d3c990972` |
| `imgui/imstb_textedit.h` | `61942af90f12b49e1f4db3ff3bde842937883caba0b340172927b5157b3fbdc6` |
| `imgui/imstb_truetype.h` | `88f0a25e27f5eefd6ec50c42fc5fe3026aa8170603efa08aa18d622db8660923` |
| `imgui/backends/imgui_impl_glfw.cpp` | `8a7fc6e71b655f4d3c6da2b79b6820295e0f64abb89835a5caaa543ad2f4ff06` |
| `imgui/backends/imgui_impl_glfw.h` | `112483b1ad9b41ac4b715f21dd313fdff177935b152ad7392d6d944011236bc8` |
| `imgui/backends/imgui_impl_opengl3.cpp` | `50f73906f253bd2c4f1bfbd57e6e6b8be5207b82aac67be237e382f93f7e2ba8` |
| `imgui/backends/imgui_impl_opengl3.h` | `d70bf0cfe8254b06574ec20ae2353bd7517c2650a366248f73b71b1c1a5f925f` |
| `imgui/backends/imgui_impl_opengl3_loader.h` | `78c116410afd80062dc81f04926b56274527f1f1ea785b649b113e09dcbc014d` |
| `imgui/LICENSE.txt` | `c80c5789748d955c4a650562baa0d750494e2c7128c2ca66aaebe2e3c4e198bf` |

## Shared game module and system libraries

- Map and JSON compile directly from `SILIC2_ROOT/src/engine/map.cpp` and `simple_json.cpp`, with headers under `SILIC2_ROOT/include`. They are not duplicated.
- GLM and, on non-Apple platforms, GLAD come from the same game source tree.
- GLFW and OpenGL are external build dependencies resolved by CMake. No external editor framework or runtime download is required.
- `SILIC2_ROOT` defaults to the root of this game repository and remains a configurable CMake cache path.

## Editor source import

The accepted editor source was copied from the adjacent `silic2_map_editor` workspace. Only CMake default paths and the build/invocation documentation were adapted for this repository layout; interaction code and tests are byte-identical. The adjacent workspace and its executable were retained.
