#include "player/player.h"
#include "game/run_stats.h"
#include "engine/game_config.h"
#include "test_support.h"
using namespace silic2;
using namespace silic2::test;
void runPlayerDamageTests() {
    auto& config=GameConfig::getInstance().player;
    const bool oldGod=config.enableGodMode;config.enableGodMode=false;
    Player player({0,0,0});RunStats stats;stats.beginRun(42,1);stats.beginRoom("arena");
    auto damage=[&](float amount,DamageSource source){float actual=player.takeDamage(amount);stats.recordDamage(actual,source);if(player.isDead())stats.recordFatalSource(source);return actual;};
    requireNear(damage(15,DamageSource::EnemyProjectile),15,.0001f,"projectile removes actual HP");
    requireNear(damage(200,DamageSource::EnemyContact),85,.0001f,"fatal damage clamps to remaining HP");
    require(player.isDead(),"fatal hit ends player health");
    requireNear(stats.snapshot().damageTaken,100,.0001f,"stats count HP loss instead of requested damage");
    require(stats.snapshot().fatalSource==DamageSource::EnemyContact,"fatal source comes from final real injury");
    player.respawn({0,0,13});requireNear(player.getHp(),100,.0001f,"respawn restores health");
    require(glm::length(player.getVelocity())==0,"respawn clears velocity");
    config.enableGodMode=true;requireNear(player.takeDamage(20),0,.0001f,"god mode reports zero actual HP loss");
    config.enableGodMode=oldGod;
}
