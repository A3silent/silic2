#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <glm/vec3.hpp>

namespace silic2 {
enum class AudioCue : uint8_t {
    PlayerShot, Hit, Kill, PlayerHurt, PlayerDeath, RiftWarning, SentryCharge,
    SentryFire, Slide, Transform, Ambience, UiConfirm, Victory, Count
};
struct AudioDiagnostics {
    size_t activeVoices = 0, criticalVoices = 0, loadedFrames = 0;
    uint64_t played = 0, dropped = 0, stolen = 0, renderedFrames = 0;
};
// Control methods are called on the game thread. The backend owns its callback.
// No filesystem access, decoding or dynamic allocation occurs during play/mix.
class AudioSystem {
public:
    static constexpr size_t VoiceCount = 24, CriticalVoiceCount = 8;
    static constexpr unsigned SampleRate = 48000;
    AudioSystem();
    ~AudioSystem();
    AudioSystem(const AudioSystem&) = delete;
    AudioSystem& operator=(const AudioSystem&) = delete;
    bool initialize(const std::string& assetDirectory = "res/audio", bool openDevice = true);
    void shutdown();
    bool play(AudioCue cue, glm::vec3 position = {}, uint32_t sourceId = 0,
              bool spatial = true, float gain = 1.0f);
    void stopSource(uint32_t sourceId); // 0 denotes anonymous and is never matched.
    void stopAll();
    void reset(); // Clears voices/counters/pause; retains preloaded PCM and mute choice.
    void setPaused(bool paused);
    void setMuted(bool muted);
    void setListener(glm::vec3 position, glm::vec3 forward, glm::vec3 up = {0,1,0});
    bool available() const; // Real device only; offline tests do not pass audio acceptance.
    bool muted() const;
    const char* statusText() const;
    AudioDiagnostics diagnostics() const;
    // Deterministic integration test/export hook; rejected while a device is active.
    bool renderOffline(float* interleavedStereo, size_t frameCount);
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
} // namespace silic2
