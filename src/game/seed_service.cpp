#include "game/seed_service.h"

#include <chrono>
#include <limits>
#include <stdexcept>

namespace silic2 {
namespace {

uint64_t splitMix64(uint64_t value) {
    value += 0x9e3779b97f4a7c15ULL;
    value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31U);
}

uint64_t fnv1a64(const std::string& value) {
    uint64_t hash = 14695981039346656037ULL;
    for (unsigned char byte : value) {
        hash ^= static_cast<uint64_t>(byte);
        hash *= 1099511628211ULL;
    }
    return hash;
}

} // namespace

SeedService::SeedService(uint64_t runSeed) : runSeed(runSeed) {
}

uint64_t SeedService::generateRunSeed() {
    std::random_device device;
    const uint64_t randomBits =
        (static_cast<uint64_t>(device()) << 32U) ^ static_cast<uint64_t>(device());
    const uint64_t clockBits = static_cast<uint64_t>(
        std::chrono::high_resolution_clock::now().time_since_epoch().count());
    return splitMix64(randomBits ^ clockBits);
}

uint64_t SeedService::deriveSeed(uint64_t runSeed, const std::string& streamName) {
    if (streamName.empty()) {
        throw std::invalid_argument("Seed stream name must not be empty");
    }
    return splitMix64(runSeed ^ fnv1a64(streamName));
}

void SeedService::reset(uint64_t newRunSeed) {
    runSeed = newRunSeed;
    streams.clear();
}

std::mt19937_64& SeedService::getStream(const std::string& streamName) {
    if (streamName.empty()) {
        throw std::invalid_argument("Seed stream name must not be empty");
    }

    auto found = streams.find(streamName);
    if (found == streams.end()) {
        found = streams.emplace(streamName, std::mt19937_64(deriveSeed(runSeed, streamName))).first;
    }
    return found->second;
}

uint64_t SeedService::nextUInt64(const std::string& streamName) {
    return getStream(streamName)();
}

uint64_t SeedService::nextUInt64(const std::string& streamName, uint64_t min, uint64_t max) {
    if (min > max) {
        throw std::invalid_argument("SeedService integer range has min greater than max");
    }

    auto& engine = getStream(streamName);
    if (min == 0 && max == std::numeric_limits<uint64_t>::max()) {
        return engine();
    }

    const uint64_t range = max - min + 1U;
    const uint64_t threshold = static_cast<uint64_t>(-range) % range;
    uint64_t value = 0;
    do {
        value = engine();
    } while (value < threshold);
    return min + (value % range);
}

int SeedService::nextInt(const std::string& streamName, int min, int max) {
    if (min > max) {
        throw std::invalid_argument("SeedService integer range has min greater than max");
    }

    const int64_t min64 = min;
    const uint64_t span = static_cast<uint64_t>(static_cast<int64_t>(max) - min64);
    return static_cast<int>(min64 + static_cast<int64_t>(nextUInt64(streamName, 0, span)));
}

double SeedService::nextDouble(const std::string& streamName, double min, double max) {
    if (!(min < max)) {
        throw std::invalid_argument("SeedService floating range must satisfy min < max");
    }

    constexpr double inverse53Bits = 1.0 / 9007199254740992.0;
    const double unit = static_cast<double>(nextUInt64(streamName) >> 11U) * inverse53Bits;
    return min + (max - min) * unit;
}

size_t SeedService::nextIndex(const std::string& streamName, size_t size) {
    if (size == 0) {
        throw std::invalid_argument("Cannot select an index from an empty range");
    }
    return static_cast<size_t>(nextUInt64(streamName, 0, static_cast<uint64_t>(size - 1)));
}

} // namespace silic2
