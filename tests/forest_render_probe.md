# Forest renderer diagnostic

Run from the repo root with an exclusive GPU slot:

```sh
make -f Makefile.map BUILD=release forest-render-probe
./bin/macos/release/forest-render-probe artifacts/forest-render res/maps/starry_crystal_arena.json
```

This explicit diagnostic uses the actual `MapRenderer::loadMap`, opaque and
translucent passes, and PixelRenderer targets at 320×200 and 640×400. It freezes
the simulation clock and three cameras. Each camera/resolution has eight warmup
frames and forty recorded frames. It does not simulate combat, HUD, audio or a
full game. The hidden context has VSync disabled; `glFinish` makes completed-frame
time include CPU submission and GPU completion. Short samples can vary with
system scheduling and GPU power state; they are not a gameplay FPS guarantee.

GL entry-point wrappers live only in this diagnostic binary. Indexed draw counts
include ordinary map surfaces, tree batches and the two crystal passes. Buffer
bytes are live requested VBO/EBO/instance-buffer storage (including the small
environment and target screen-quad buffers), not driver VRAM residency; they
exclude textures, framebuffer attachments and CPU-side map data. Textures retain
the existing TextureManager cache. No production instrumentation is enabled.

For a functional check without repeated timing, append `--verify`. This renders
one frame per camera/resolution, asserts draw/buffer counts are bounded by unique
pine mesh/material groups instead of tree placements, and checks GL errors and
complete buffer release. `checks.jsonl` records results. The actual full map
must have local Tripo assets available.

Compare matching frozen frames before and after an optimization:

```sh
python3 tests/compare_forest_frames.py artifacts/before artifacts/after --output artifacts/comparison.json
```

The comparison permits sparse local-space floating-point rasterization changes
(≤0.1% pixels differ and ≤0.02% differ by more than 8/255), not missing trees or
changed materials. Inspect representative images as well. The baseline and
first failing resource checks for the initial optimization are retained in
`artifacts/acceptance/forest-render-opt/` locally; no source assets are modified.
