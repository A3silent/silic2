#include "game/seed_service.h"
#include "test_support.h"

#include <stdexcept>

using silic2::SeedService;
using silic2::test::require;

void runSeedServiceTests() {
    require(SeedService::deriveSeed(42, "encounter") == 8738628067895197900ULL,
            "encounter seed derivation changed");
    require(SeedService::deriveSeed(42, "rewards") == 6264610358136742345ULL,
            "rewards seed derivation changed");
    require(SeedService::deriveSeed(42, "room_graph") == 15271608144001022160ULL,
            "room graph seed derivation changed");
    require(SeedService::deriveSeed(42, "cosmetic") == 17313518848883080277ULL,
            "cosmetic seed derivation changed");

    SeedService first(42);
    SeedService second(42);
    for (int i = 0; i < 1000; ++i) {
        require(first.nextUInt64("encounter") == second.nextUInt64("encounter"),
                "same seed and stream produced different sequences");
    }

    SeedService baseline(91);
    const uint64_t expectedReward = baseline.nextUInt64("rewards");
    SeedService withCosmetics(91);
    for (int i = 0; i < 100; ++i) {
        (void)withCosmetics.nextUInt64("cosmetic");
    }
    require(withCosmetics.nextUInt64("rewards") == expectedReward,
            "cosmetic stream changed rewards stream");

    SeedService resettable(777);
    const uint64_t firstValue = resettable.nextUInt64("room_graph");
    (void)resettable.nextUInt64("room_graph");
    resettable.reset(777);
    require(resettable.nextUInt64("room_graph") == firstValue,
            "reset did not restore stream sequence");

    bool invalidRangeRejected = false;
    try {
        (void)first.nextInt("encounter", 5, 4);
    } catch (const std::invalid_argument&) {
        invalidRangeRejected = true;
    }
    require(invalidRangeRejected, "invalid integer range was accepted");

    bool emptyIndexRejected = false;
    try {
        (void)first.nextIndex("encounter", 0);
    } catch (const std::invalid_argument&) {
        emptyIndexRejected = true;
    }
    require(emptyIndexRejected, "empty index range was accepted");
}
