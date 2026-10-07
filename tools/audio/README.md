# SAMPLE-AUDIO integration and validation

All thirteen WAVs are original mathematical/noise synthesis from `generate.py`, generated offline with Python's standard library and fixed seeds. There are no external recordings, samples, trained models or runtime Python dependencies. Source and assets are supplied as part of Silic2. `res/audio/manifest.json` records the PCM format, durations, sample counts, peaks, RMS and SHA-256 hashes. Regenerate from repository root with `python3 tools/audio/generate.py`.

Dependency: unmodified miniaudio 0.11.25 with elected MIT-0 license; see `third_party/miniaudio/PROVENANCE.md`. No network at build/runtime. Compile `src/audio/audio_system.cpp` and `src/audio/miniaudio_impl.cpp` (the only implementation TU). macOS links CoreAudio and AudioToolbox; Linux links dl, pthread and m; Windows uses miniaudio's dynamically loaded system APIs. Windows/Linux runtime checks are pending.

## Game-thread contract

Include `audio/audio_system.h`. Call `initialize("res/audio")` before READY/combat. It decodes all assets to resident float mono PCM and opens a 48kHz stereo playback device. A failed asset/device leaves the system safe to call. Show `statusText()` in the English HUD whenever nonempty; null backends are rejected. `available()` confirms a real initialized backend, not perceptual audibility. Output device interruption/stopping produces AUDIO UNAVAILABLE. A failed device can be retried through initialize outside combat. AUDIO MUTED, AUDIO UNAVAILABLE and AUDIO OFFLINE TEST never count as passing listening acceptance.

`setListener(camera.getPosition(), camera.getFront(), camera.getUp())` each frame. Stereo equal-power pan uses the listener's right vector and inverse-distance attenuation (2-unit near range). No HRTF or front/back/elevation localization promise.

Call `play(cue, worldPosition, sourceId, spatial, gain)` once per event, not once per frame. Use spatial=false for first-person PlayerShot, Hit/Kill confirmation, PlayerHurt/PlayerDeath, Slide, Transform, Ambience, UiConfirm and Victory. Use spatial=true for RiftWarning, SentryCharge and SentryFire. Distinct nonzero source IDs make cues cancellable; `stopSource(id)` stops all voices belonging to that source. Reserve separate source ID namespaces for rifts and sentries. Replaying the same cue/source restarts the cue, so never poll play for a continuously active warning. Source 0 is anonymous and cannot be cancelled separately.

- RiftWarning runs 1.2s; start at warning onset. Stop its source on cancellation/materialization. Director retries must not emit duplicate warning-start events.
- SentryCharge runs 1s with rising pitch/pulse rate; start at charge onset, cancel if interrupted/killed, stop its source before SentryFire.
- Transform runs 4s, start once at combat time 45s.
- Ambience loops (8s seamless periodic bed), play once after reset. The loop is deduplicated. It is expendable under severe voice pressure.
- On pause/focus loss: setPaused(true); resume: setPaused(false). Paused cues cannot be added; active cursors remain frozen and output is silent. Do not play UiConfirm while paused if expecting it to be audible.
- On death/victory: stopAll(), then play PlayerDeath/Victory nonspatial. No stale attacks survive the transition.
- On restart: reset(), then play Ambience nonspatial. reset clears voices, statistics, sequence and pause; keeps resident PCM and the user's mute choice. No combat-time file I/O.
- On exit: shutdown(), or destruction. Device callback is joined before PCM is freed. Repeated shutdown is safe.

The mixer has exactly 24 slots. Eight are reserved for priority>=4 (rift, charge/fire, hurt/death). Other sounds use only the remaining 16. Higher/equal priority can replace an older voice; decoration cannot evict a warning. Critical overflow can replace ordinary slots. Ordinary volume ducks to 40% while any critical voice plays, with bounded soft-limited output. The callback performs no file access, decoding or allocation; a short mutex protects fixed voice/listener state. diagnostics() exposes active/critical counts, played/dropped/stolen, renderedFrames and loadedFrames.

## Repeatable checks

From repository root:

```
clang++ -std=c++17 -Wall -Wextra -O0 -g -Iinclude -Itests -DSILIC2_AUDIO_TEST_MAIN src/audio/audio_system.cpp src/audio/miniaudio_impl.cpp tests/audio_system_test.cpp -framework CoreAudio -framework AudioToolbox -o /tmp/silic2-audio-test
/tmp/silic2-audio-test
clang++ -std=c++17 -O2 -DNDEBUG -Iinclude -DSILIC2_AUDIO_DEVICE_SMOKE_MAIN src/audio/audio_system.cpp src/audio/miniaudio_impl.cpp tests/audio_device_smoke.cpp -framework CoreAudio -framework AudioToolbox -o /tmp/silic2-audio-device-test
/tmp/silic2-audio-device-test
```

For the common test executable, call runAudioTests() from test_main and link audio sources/platform libraries. The standalone main exists only when its explicit test macro is defined. The device smoke uses zero cue gain and proves callback/lifecycle behavior only. Offline tests exercise direction, distance, voice limits, overlap bounds, source cancellation, pause continuity, death, mute, missing assets and 50 audio-only resets. These are **not** 50 complete game restarts, memory-growth acceptance, or a human headphone listening test.

Human headphone checks still required: left/right threats while turning; clarity of overlapping Sentry/rift warnings under rapid fire; relative gun/hit/hurt distinction; full 45s transformation while under fire; pause/focus resume; no stale cues after death; 50 full runtime restarts; comfort and absence of audible clicks/distortion. Record observations, device, volume and participant rather than inferring a pass from rendered waveforms.
