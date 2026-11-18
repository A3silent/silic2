#pragma once

#include <filesystem>

namespace silic2 {

struct FisheyeSettings {
    bool enabled = true;
    double hipStrength = .33;
    static constexpr double MaxHipStrength = 1.50;

    // The path names the general settings file; legacy siblings migrate on first load.
    static FisheyeSettings load(const std::filesystem::path& path);
    void save(const std::filesystem::path& path) const;
    bool adjustHip(bool increase);
};

} // namespace silic2
