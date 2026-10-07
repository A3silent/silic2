#include "audio/audio_system.h"
#include "../../third_party/miniaudio/miniaudio.h"
#include <glm/geometric.hpp>
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <mutex>
#include <vector>

namespace silic2 {
namespace {
struct CueSpec { const char* file; float volume; int priority; bool loop; };
constexpr std::array<CueSpec, static_cast<size_t>(AudioCue::Count)> Specs{{
    {"player_shot.wav", .50f, 2, false}, {"hit.wav", .48f, 2, false},
    {"kill.wav", .48f, 2, false}, {"player_hurt.wav", .65f, 4, false},
    {"player_death.wav", .7f, 5, false}, {"rift_warning.wav", .80f, 4, false},
    {"sentry_charge.wav", .90f, 5, false}, {"sentry_fire.wav", .85f, 5, false},
    {"slide.wav", .38f, 1, false}, {"transform.wav", .40f, 1, false},
    {"ambience.wav", .20f, 0, true}, {"ui_confirm.wav", .45f, 2, false},
    {"victory.wav", .60f, 3, false}
}};
bool finite(glm::vec3 v) { return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z); }
}
struct AudioSystem::Impl {
    struct Voice {
        bool active = false, spatial = false;
        size_t cue = 0, cursor = 0;
        uint32_t source = 0;
        uint64_t order = 0;
        glm::vec3 position{};
        float gain = 1;
    };
    std::array<std::vector<float>, Specs.size()> clips;
    std::array<Voice, VoiceCount> voices{};
    mutable std::mutex mutex;
    ma_device device{};
    bool deviceInitialized = false, loaded = false, offline = false;
    bool paused = false, mute = false;
    std::atomic<bool> lost{false};
    glm::vec3 listener{}, right{1,0,0};
    AudioDiagnostics stats{};
    uint64_t sequence = 0;
    static void callback(ma_device* d, void* output, const void*, ma_uint32 frames) {
        static_cast<Impl*>(d->pUserData)->mix(static_cast<float*>(output), frames);
    }
    static void notification(const ma_device_notification* n) {
        auto* self = static_cast<Impl*>(n->pDevice->pUserData);
        if (n->type == ma_device_notification_type_stopped || n->type == ma_device_notification_type_interruption_began)
            self->lost.store(true);
        if (n->type == ma_device_notification_type_started || n->type == ma_device_notification_type_interruption_ended)
            self->lost.store(false);
    }
    void mix(float* output, size_t frames) {
        std::fill(output, output + frames * 2, 0.0f);
        std::lock_guard<std::mutex> guard(mutex);
        if (!loaded || paused) return;
        bool warning = false;
        for (const auto& v : voices) if (v.active && Specs[v.cue].priority >= 4) warning = true;
        for (auto& v : voices) {
            if (!v.active) continue;
            const auto& spec = Specs[v.cue];
            const auto& clip = clips[v.cue];
            float pan = 0, attenuation = 1;
            if (v.spatial) {
                const auto delta = v.position - listener;
                const float distance = glm::length(delta);
                if (distance > .001f) pan = std::clamp(glm::dot(delta / distance, right), -1.0f, 1.0f);
                attenuation = 1.0f / (1.0f + .12f * std::max(0.0f, distance - 2.0f));
            }
            const float volume = v.gain * spec.volume * attenuation *
                ((warning && spec.priority < 4) ? .40f : 1.0f);
            const float left = std::sqrt((1-pan)*.5f)*volume;
            const float rightGain = std::sqrt((1+pan)*.5f)*volume;
            for (size_t frame = 0; frame < frames; ++frame) {
                if (v.cursor == clip.size()) {
                    if (spec.loop) v.cursor = 0;
                    else { v.active = false; break; }
                }
                const float sample = clip[v.cursor++];
                if (!mute) { output[frame*2] += sample*left; output[frame*2+1] += sample*rightGain; }
            }
        }
        // Soft limit peaks from simultaneous threats, without callback allocations.
        for (size_t i=0; i<frames*2; ++i) output[i] = output[i] / (1.0f + std::abs(output[i]));
        stats.renderedFrames += frames;
    }
};
AudioSystem::AudioSystem() : impl(std::make_unique<Impl>()) {}
AudioSystem::~AudioSystem() { shutdown(); }
bool AudioSystem::initialize(const std::string& directory, bool openDevice) {
    shutdown();
    auto& p = *impl;
    for (size_t i=0; i<Specs.size(); ++i) {
        ma_decoder_config cfg = ma_decoder_config_init(ma_format_f32, 1, SampleRate);
        ma_uint64 frames = 0;
        void* data = nullptr;
        const auto path = directory + "/" + Specs[i].file;
        if (ma_decode_file(path.c_str(), &cfg, &frames, &data) != MA_SUCCESS || !data || frames == 0) {
            if (data) ma_free(data, nullptr);
            shutdown();
            return false;
        }
        const auto* samples = static_cast<float*>(data);
        p.clips[i].assign(samples, samples+frames);
        ma_free(data, nullptr);
        p.stats.loadedFrames += static_cast<size_t>(frames);
    }
    p.loaded = true;
    p.offline = !openDevice;
    p.lost.store(false);
    if (!openDevice) return true;
    auto cfg = ma_device_config_init(ma_device_type_playback);
    cfg.playback.format = ma_format_f32;
    cfg.playback.channels = 2;
    cfg.sampleRate = SampleRate;
    cfg.dataCallback = Impl::callback;
    cfg.notificationCallback = Impl::notification;
    cfg.pUserData = &p;
    if (ma_device_init(nullptr, &cfg, &p.device) != MA_SUCCESS) { shutdown(); return false; }
    p.deviceInitialized = true;
    // A null backend discards audio and must never masquerade as audible success.
    if (p.device.pContext->backend == ma_backend_null) { shutdown(); return false; }
    if (ma_device_start(&p.device) != MA_SUCCESS) { shutdown(); return false; }
    return true;
}
void AudioSystem::shutdown() {
    auto& p = *impl;
    // Stop/join callback before destroying the PCM it reads. Do not hold its lock.
    if (p.deviceInitialized) { ma_device_uninit(&p.device); p.deviceInitialized = false; }
    std::lock_guard<std::mutex> guard(p.mutex);
    p.voices = {};
    for (auto& clip : p.clips) std::vector<float>().swap(clip);
    p.loaded = p.offline = p.paused = false;
    p.stats = {}; p.sequence = 0;
}
bool AudioSystem::play(AudioCue cue, glm::vec3 position, uint32_t source, bool spatial, float gain) {
    auto& p = *impl;
    const size_t index = static_cast<size_t>(cue);
    if (index >= Specs.size() || !finite(position) || !std::isfinite(gain)) return false;
    std::lock_guard<std::mutex> guard(p.mutex);
    if (!p.loaded || p.paused || p.mute || p.lost.load()) return false;
    const auto& spec = Specs[index];
    // A source identifies a cancellable cue, not an entity's entire sound history.
    // Retriggering the same source/cue replaces it rather than stacking it.
    for (auto& v : p.voices) if (v.active && v.cue == index &&
        ((source && v.source == source) || spec.loop)) {
        v.position = position; v.gain = std::clamp(gain,0.0f,2.0f);
        if (!spec.loop) v.cursor = 0;
        return true;
    }
    Impl::Voice* slot = nullptr;
    const bool critical = spec.priority >= 4;
    const size_t begin = critical ? 0 : CriticalVoiceCount;
    for (size_t i=begin; i<VoiceCount; ++i) if (!p.voices[i].active) { slot=&p.voices[i]; break; }
    if (!slot) {
        // Critical overflow may steal ordinary voices; decoration never steals warnings.
        for (size_t i=begin; i<VoiceCount; ++i) {
            auto& v = p.voices[i];
            if (Specs[v.cue].priority > spec.priority) continue;
            if (!slot || Specs[v.cue].priority < Specs[slot->cue].priority ||
                (Specs[v.cue].priority == Specs[slot->cue].priority && v.order < slot->order)) slot=&v;
        }
        if (slot) ++p.stats.stolen;
    }
    if (!slot) { ++p.stats.dropped; return false; }
    *slot = {true, spatial, index, 0, source, ++p.sequence, position, std::clamp(gain,0.0f,2.0f)};
    ++p.stats.played;
    return true;
}
void AudioSystem::stopSource(uint32_t source) {
    if (!source) return;
    std::lock_guard<std::mutex> guard(impl->mutex);
    for (auto& v : impl->voices) if (v.source == source) v.active=false;
}
void AudioSystem::stopAll() { std::lock_guard<std::mutex> guard(impl->mutex); impl->voices={}; }
void AudioSystem::reset() {
    std::lock_guard<std::mutex> guard(impl->mutex);
    impl->voices={}; impl->paused=false; impl->sequence=0;
    const auto loaded=impl->stats.loadedFrames; impl->stats={}; impl->stats.loadedFrames=loaded;
}
void AudioSystem::setPaused(bool value) { std::lock_guard<std::mutex> guard(impl->mutex); impl->paused=value; }
void AudioSystem::setMuted(bool value) { std::lock_guard<std::mutex> guard(impl->mutex); impl->mute=value; }
void AudioSystem::setListener(glm::vec3 position, glm::vec3 forward, glm::vec3 up) {
    if (!finite(position) || !finite(forward) || !finite(up)) return;
    auto r = glm::cross(forward,up);
    std::lock_guard<std::mutex> guard(impl->mutex);
    impl->listener=position;
    if (glm::length(r) > .0001f) impl->right=glm::normalize(r);
}
bool AudioSystem::available() const { return impl->deviceInitialized && !impl->lost.load(); }
bool AudioSystem::muted() const { std::lock_guard<std::mutex> guard(impl->mutex); return impl->mute; }
const char* AudioSystem::statusText() const {
    if (muted()) return "AUDIO MUTED";
    if (impl->offline) return "AUDIO OFFLINE TEST";
    return available() ? "" : "AUDIO UNAVAILABLE";
}
AudioDiagnostics AudioSystem::diagnostics() const {
    std::lock_guard<std::mutex> guard(impl->mutex);
    auto stats=impl->stats;
    for (const auto& v : impl->voices) if (v.active) {
        ++stats.activeVoices; if (Specs[v.cue].priority >= 4) ++stats.criticalVoices;
    }
    return stats;
}
bool AudioSystem::renderOffline(float* output, size_t frames) {
    if (!output || !impl->offline || impl->deviceInitialized) return false;
    impl->mix(output,frames); return true;
}
} // namespace silic2
