# SAMPLE-AUDIO validation — 2026-10-07

Implementation delivered in `include/audio`, `src/audio`, `res/audio`, `tools/audio`, `third_party/miniaudio`, `tests/audio_system_test.cpp`, and `tests/audio_device_smoke.cpp`. No App, Makefile, gameplay, other specialist or existing user files were edited by ECHO.

Observed on local macOS arm64:

- clang++ C++17 Debug (-O0 -g) standalone offline audio suite: pass.
- clang++ C++17 Release (-O2 -DNDEBUG) standalone offline audio suite: pass.
- Real initialized playback backend callback: active. 50 audio-only reset/pause/resume/stop cycles passed; paused rendered-frame counter stayed unchanged, active voices cleared, shutdown released loaded PCM. Final 100ms observation produced 5280 frames. The repeat probe uses zero cue gain and does not assert audible playback.
- 13 WAVs: mono, 48000 Hz, signed 16-bit PCM, expected duration/frame count and manifest SHA-256 verified; sample magnitudes below integer clipping.
- Generator reproducibility: reran `python3 tools/audio/generate.py`; all 13 WAVs and manifest.json were byte-identical (SHA-256 before/after).
- Official dependency header and license SHA-256 match PROVENANCE.md.
- Offline coverage: asset failure/no-op/status, full preload, visible distinction between offline and live availability, left/right output and distance attenuation, exact waveform continuity after pause, 16 ordinary plus 8 reserved critical voices, decoration cannot evict warnings, bounded finite overlap, cancellation, death completion, mute visibility/silence, 50 audio resets retaining resident assets, idempotent shutdown.

Limitations / required integration checks:

- VECTOR owns game event routing, HUD status, Makefile and common test entry. API and build handoff sent directly to VECTOR.
- These 50 audio state cycles are not 50 complete runtime restarts and are not memory growth/performance acceptance.
- No human headphone listening observations were collected. Direction intelligibility, threat priority under fire, comfort, clicks/distortion and pause/death/restart perception remain pending. Muted or unavailable fallback cannot pass this gate.
- Windows/Linux build structure requirements supplied; compilation/runtime not tested on those OSes.
