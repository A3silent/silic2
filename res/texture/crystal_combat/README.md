# Crystal combat surface textures

Four original images generated on 2026-10-13 with the built-in `image_gen.imagegen`
tool, one call per material, opaque background and no reference image. Complete
submitted prompts are in `prompts/`; unmodified 1254×1254 tool outputs are retained
in `originals/`. `generation-sources.json` records the original tool output paths.
`manifest.json` records dimensions, SHA-256 hashes and runtime profiles. These are
AI-generated assets, not hand-painted pixel art or procedural replacements.

Runtime PNGs are RGB, 64×64, made only with Pillow's BOX resize:

```python
Image.open(original).convert('RGB').resize((64, 64), Image.Resampling.BOX).save(runtime)
```

No recoloring, seam repair, compositing or invented texture detail was performed.
The generated sources are not strictly seamless: panel joints in particular need
not match at opposite edges. Repeated 64×64 previews and actual 320×200 GL captures
were inspected; the low-contrast repeats are accepted at the current world scale.

| Runtime file | Material mapping | Repeat size | Modulation strength |
| --- | --- | --- | --- |
| metal_deck-64.png | combat_foundation, combat_deck | 6 m | 0.80 |
| mineral_ceramic-64.png | combat_ivory | 5 m | 0.55 |
| dark_shell-64.png | combat_brown | 8 m | 0.65 |
| crystal_cleavage-64.png | combat_crystal | 6 m | 0.48 |

MapRenderer selects these defaults only for `crystal_combat` surfaces without an
explicit texture. Existing OBJ geometry has no UVs; the Crystal shader uses three
world-space projections blended by normal weights. Sample RGB is divided by the
asset's mean RGB, clamped to 0.55–1.45 and blended by the strength above. Regional
palette, lights, transformation and warm warning signals remain shader-driven.
Old procedural bands are softened when a texture is active. Missing assets retain
the previous procedural appearance and do not prevent loading the map.

Existing TextureManager owns four shared cached textures, loaded at map load,
with REPEAT, NEAREST and NEAREST_MIPMAP_NEAREST filtering. Cache reuse prevents
per-brush duplicate uploads. Four RGB 64×64 images plus complete mip chains require
65,532 bytes of logical texel data; driver allocation/padding is not measured.
There are no additional draw calls, geometry, VBOs or per-frame file reads.

Validation, limitations and reproducible probe commands:
[`TEXTURES.md`](../../../src/environment/validation/TEXTURES.md).
