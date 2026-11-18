#include "engine/fisheye_settings.h"
#include "engine/simple_json.h"
#include <algorithm>
#include <cerrno>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

#ifdef _WIN32
#include <process.h>
#include <windows.h>
#else
#include <unistd.h>
#ifdef __APPLE__
#include <stdio.h>
#elif defined(__linux__)
#include <fcntl.h>
#include <linux/fs.h>
#include <sys/syscall.h>
#endif
#endif

namespace silic2 {
namespace {
namespace fs = std::filesystem;

SimpleJson readObject(const fs::path& path) {
    if (!fs::is_regular_file(path)) throw std::runtime_error("Expected a configuration file");
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("Cannot open configuration file");
    std::ostringstream data;
    data << file.rdbuf();
    auto json = SimpleJson::parse(data.str());
    if (!json.isObject()) throw std::runtime_error("Expected a JSON object");
    return json;
}

FisheyeSettings readSection(const SimpleJson& config) {
    FisheyeSettings settings;
    if (!config.hasKey("fisheye")) return settings;
    const auto& section = config["fisheye"];
    if (!section.isObject()) throw std::runtime_error("Expected a fisheye settings object");
    settings.enabled = section.getBool("enabled", settings.enabled);
    settings.hipStrength = std::clamp(section.getNumber("hipStrength", settings.hipStrength),
                                      0.0, FisheyeSettings::MaxHipStrength);
    return settings;
}

// Prefer an exclusive rename so simultaneous launches can agree on a winner.
// These APIs also work on volumes that do not support hard links.
bool renameNew(const fs::path& temporary, const fs::path& path) {
#ifdef _WIN32
    if (MoveFileW(temporary.c_str(), path.c_str())) return true;
    const std::error_code error(GetLastError(), std::system_category());
#elif defined(__APPLE__)
    if (renamex_np(temporary.c_str(), path.c_str(), RENAME_EXCL) == 0) return true;
    const std::error_code error(errno, std::generic_category());
#elif defined(__linux__) && defined(SYS_renameat2) && defined(RENAME_NOREPLACE)
    if (syscall(SYS_renameat2, AT_FDCWD, temporary.c_str(), AT_FDCWD, path.c_str(), RENAME_NOREPLACE) == 0) return true;
    const std::error_code error(errno, std::generic_category());
#else
    const auto error = std::make_error_code(std::errc::operation_not_supported);
#endif
    if (error == std::errc::file_exists) return false;
    if (error != std::errc::operation_not_supported && error != std::errc::function_not_supported &&
        error != std::errc::invalid_argument)
        throw fs::filesystem_error("Cannot publish new configuration", path, error);
    // Older kernels/volumes lack the exclusive operation. Recheck before a normal atomic rename.
    if (fs::exists(fs::symlink_status(path))) return false;
    fs::rename(temporary, path);
    return true;
}

bool publish(const fs::path& path, const SimpleJson& config, bool replace) {
#ifdef _WIN32
    const auto processId = _getpid();
#else
    const auto processId = getpid();
#endif
    auto temporary = path;
    temporary += ".tmp-" + std::to_string(processId);
    try {
        std::ofstream file;
        file.exceptions(std::ios::badbit | std::ios::failbit);
        file.open(temporary, std::ios::binary | std::ios::trunc);
        file << config.toPrettyString() << '\n';
        file.close();
        if (replace) {
            fs::rename(temporary, path);
        } else if (!renameNew(temporary, path)) {
            fs::remove(temporary);
            return false;
        }
    } catch (...) {
        std::error_code ignored;
        fs::remove(temporary, ignored);
        throw;
    }
    return true;
}
} // namespace

FisheyeSettings FisheyeSettings::load(const fs::path& path) {
    FisheyeSettings settings;
    try {
        if (fs::exists(fs::symlink_status(path))) return readSection(readObject(path));
        const auto legacy = path.parent_path() / "fisheye.local.json";
        if (fs::exists(legacy)) {
            try {
                auto config = SimpleJson::object();
                config["fisheye"] = readObject(legacy);
                settings = readSection(config);
                if (publish(path, config, false)) {
                    fs::remove(legacy);
                    return settings;
                }
            } catch (const std::exception&) {
                // Another launcher may have finished migration while we opened the old file.
                if (!fs::exists(path)) throw;
            }
        }
        if (fs::exists(path)) {
            settings = {};
            return readSection(readObject(path));
        }
    } catch (const std::exception& e) {
        std::cerr << "Could not load or migrate settings for " << path << ": " << e.what() << '\n';
    }
    return settings;
}

bool FisheyeSettings::adjustHip(bool increase) {
    // Work in hundredths for the hotkey step, preserving fractions set in JSON.
    const double next = std::clamp((hipStrength * 100.0 + (increase ? 5.0 : -5.0)) / 100.0,
                                   0.0, MaxHipStrength);
    if (next == hipStrength) return false;
    hipStrength = next;
    return true;
}

void FisheyeSettings::save(const fs::path& path) const {
    // Read at each save so settings added or edited since startup survive the merge.
    for (;;) {
        const bool exists = fs::exists(fs::symlink_status(path));
        auto config = exists ? readObject(path) : SimpleJson::object();
        if (!config.hasKey("fisheye")) config["fisheye"] = SimpleJson::object();
        auto& section = config["fisheye"];
        if (!section.isObject()) throw std::runtime_error("Expected a fisheye settings object");
        section["enabled"] = SimpleJson(enabled);
        section["hipStrength"] = SimpleJson(hipStrength);
        if (publish(path, config, exists)) return;
    }
}

} // namespace silic2
