#pragma once
#include <algorithm>
#include <cmath>

namespace silic2 {
// One simulation-owned ADS state. Rendering consumes blend; it never advances it.
class AimState {
public:
    static constexpr float TransitionSeconds = .16f;
    static constexpr float AimedFovDegrees = 32.f;
    void update(bool held, float dt, bool playing = true) {
        if(!playing || !std::isfinite(dt) || dt <= 0) return;
        progress = std::clamp(progress + (held ? dt : -dt) / TransitionSeconds, 0.f, 1.f);
    }
    void reset() { progress = 0; }
    float blend() const { return progress * progress * (3.f - 2.f * progress); }
    float fov(float hipFov) const {
        return hipFov + (std::min(hipFov, AimedFovDegrees) - hipFov) * blend();
    }
    float mouseScale(float hipFov) const {
        constexpr float HalfRadians = .008726646259971648f;
        return std::tan(fov(hipFov) * HalfRadians) / std::tan(hipFov * HalfRadians);
    }
private:
    float progress = 0;
};
}
