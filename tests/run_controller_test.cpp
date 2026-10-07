#include "game/run_controller.h"
#include "test_support.h"

using silic2::RunController;
using silic2::test::require;

void runRunControllerTests() {
    RunController controller(1234);
    require(controller.startRun("test_room"), "controller could not start run");
    const uint64_t firstRandom = controller.seeds().nextUInt64("encounter");

    controller.stats().recordShot();
    controller.tick(2.0f, true);
    require(controller.restartRun("test_room"), "controller could not restart run");

    require(controller.state().active, "restarted run is not active");
    require(controller.state().seed == 1234, "restart changed the seed");
    require(controller.state().runIndex == 2, "restart did not advance run index");
    require(controller.stats().snapshot().shotsFired == 0, "restart retained shot count");
    require(controller.stats().snapshot().runTime == 0.0f, "restart retained run time");
    require(controller.seeds().nextUInt64("encounter") == firstRandom,
            "restart did not reset named random streams");

    for (int restart = 0; restart < 50; ++restart) {
        controller.stats().recordShot();
        controller.stats().recordDamage(5.0f, silic2::DamageSource::EnemyContact);
        (void)controller.seeds().nextUInt64("encounter");

        require(controller.restartRun("test_room"),
                "repeated controller restart failed");
        require(controller.stats().snapshot().shotsFired == 0,
                "repeated restart retained shot count");
        require(controller.stats().snapshot().damageTaken == 0.0f,
                "repeated restart retained damage");
        require(controller.seeds().nextUInt64("encounter") == firstRandom,
                "repeated restart retained random stream state");
    }
}
