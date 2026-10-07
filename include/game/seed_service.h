#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <random>
#include <string>

namespace silic2 {

class SeedService {
public:
    explicit SeedService(uint64_t runSeed);

    static uint64_t generateRunSeed();
    static uint64_t deriveSeed(uint64_t runSeed, const std::string& streamName);

    void reset(uint64_t runSeed);

    uint64_t getRunSeed() const { return runSeed; }
    uint64_t nextUInt64(const std::string& streamName);
    uint64_t nextUInt64(const std::string& streamName, uint64_t min, uint64_t max);
    int nextInt(const std::string& streamName, int min, int max);
    double nextDouble(const std::string& streamName, double min = 0.0, double max = 1.0);
    size_t nextIndex(const std::string& streamName, size_t size);

private:
    uint64_t runSeed;
    std::map<std::string, std::mt19937_64> streams;

    std::mt19937_64& getStream(const std::string& streamName);
};

} // namespace silic2
