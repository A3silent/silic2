# Crystal slate floor

Generated with built-in imagegen. Selected output: `res/texture/forest_arena/crystal_slate_albedo.png` (1254×1254 RGB).

```text
Use case: stylized-concept
Asset type: seamless tileable game texture, one square opaque RGB albedo / base-color image
Primary request: Make an edge-to-edge texture of compact dark blue-gray slate bedrock for an exposed extraterrestrial crystal arena floor.
Composition: strictly perpendicular orthographic top-down, entire image covered by the same continuous flat natural rock material, no horizon, no scene or objects. Low contrast small-scale mineral mottling and fine-grained flecks, with a few restrained thin irregular shallow natural hairline fractures. Organic fractured mineral grain without pavement, regular joints, or artificial repeating motifs. Even feature scale everywhere; no large cracks or conspicuous landmarks. Edge transitions must seamlessly tile across both axes with the same average tone, color, and grain at opposite edges, no seam.
Color palette: cool charcoal gray and subdued desaturated slate blue, moderate albedo around dark midtones so blue gameplay lighting can reveal detail. Slight quartz fleck variation without any bright or white areas.
Lighting: flat diffuse base color only; no baked lighting, no directionality, no reflections, no cast shadows, no highlights, no vignette.
Avoid: runes, symbols, rings, writing, patterns that look engraved or carved, luminous veins, crystal clusters, trees, plants, roots, foliage, grass, grids, paving tiles, rubble, borders, water, logos, watermarks.
```

The floor samples the tile once per 10 world meters, with repeat wrapping and
linear mipmap filtering. Floor lighting and combat warnings remain separate from
the tile. The PNG is a local game asset under the repository's source-only
policy. If absent, the original procedural floor remains available.
