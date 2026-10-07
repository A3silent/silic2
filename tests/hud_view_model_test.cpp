#include "hud/bitmap_font_renderer.h"
#include "test_support.h"
#include <cmath>
#include <limits>
#ifdef SILIC2_HUD_TEST_MAIN
#include <iostream>
#endif
using namespace silic2;
using silic2::test::require;

void runHudViewModelTests() {
    HudEnemy enemy;
    enemy.hp=3;enemy.maxHp=6;enemy.secondsSinceDamage=0.1f;
    require(!hudEnemyVisible(enemy),"enemy visible without view/occlusion evidence");
    enemy.inView=true;
    require(!hudEnemyVisible(enemy),"occluded enemy leaked to minimap");
    enemy.unobstructed=true;
    require(hudEnemyVisible(enemy) && hudEnemyBarVisible(enemy),"visible damaged enemy not shown");
    enemy.secondsSinceDamage=2;
    require(hudEnemyBarVisible(enemy),"bar removed before two second boundary");
    enemy.secondsSinceDamage=2.001f;
    require(hudEnemyVisible(enemy) && !hudEnemyBarVisible(enemy),"expired bar remained or contact disappeared");
    enemy.secondsSinceDamage=-1;
    require(!hudEnemyBarVisible(enemy),"future damage produced a bar");
    enemy.secondsSinceDamage=std::numeric_limits<float>::quiet_NaN();
    require(!hudEnemyBarVisible(enemy),"invalid damage age produced a bar");
    enemy.secondsSinceDamage=0;enemy.alive=false;
    require(!hudEnemyVisible(enemy) && !hudEnemyBarVisible(enemy),"dead contact remained");
    require(hudClock(90.9f)=="01:30" && hudClock(-1)=="00:00","combat clock wrong");
    require(hudAccuracy(0,0)=="--" && hudAccuracy(1,3)=="33%","accuracy rounds incorrectly");
    require(hudAccuracy(4,3)=="100%","accuracy outside percentage bounds");
    require(hudHpLoss(19.24f)=="19.2","actual fractional damage lost in result display");
    require(hudFatalSource("enemy_projectile")=="SENTRY PROJECTILE","raw damage enum leaked into result UI");

    HudViewModel labels;
    labels.arenaSurvival=false;labels.livingEnemies=3;labels.combatSeconds=60;
    require(hudPhaseLabel(labels)=="CLEAR" && hudObjectiveLabel(labels)=="3 HOSTILES LEFT", "legacy HUD describes existing hostiles without survival timer");
    require(hudReadyObjective(labels).find("90")==std::string::npos, "legacy READY has no fixed wave duration");
    labels.livingEnemies=0;
    require(hudPhaseLabel(labels)=="EXPLORE" && hudObjectiveLabel(labels)=="EXPLORE THE MAP", "empty legacy map remains exploration");
    labels.arenaSurvival=true;labels.transformationStart=12;labels.transformationDuration=8;
    labels.combatSeconds=11.9f;require(hudPhaseLabel(labels)=="SURVIVE", "authored transformation has not started");
    labels.combatSeconds=12;require(hudPhaseLabel(labels)=="SHIFTING", "authored transformation starts exactly at boundary");
    labels.combatSeconds=19.9f;require(hudPhaseLabel(labels)=="SHIFTING", "authored transformation duration retained");
    labels.combatSeconds=20;require(hudPhaseLabel(labels)=="AFTERSHIFT", "authored transformation ends exactly at boundary");
    labels.hasTransformation=false;require(hudPhaseLabel(labels)=="SURVIVE", "None preset has no fictitious transformation");
    labels.cleanup=true;require(hudPhaseLabel(labels)=="CLEANUP", "cleanup remains the encounter objective");
    HudViewModel model;model.phase=HudPhase::Playing;
    HudBatch empty,hidden,visible;
    buildHudGeometry(empty,model,800,600);
    enemy.alive=true;enemy.unobstructed=false;model.enemies={enemy};
    buildHudGeometry(hidden,model,800,600);
    require(empty.vertices.size()==hidden.vertices.size(),"hidden contact leaked geometry");
    model.enemies[0].unobstructed=true;
    buildHudGeometry(visible,model,800,600);
    require(visible.vertices.size()>hidden.vertices.size(),"visible contact produced no geometry");

    for(auto phase:{HudPhase::Ready,HudPhase::Playing,HudPhase::Paused,HudPhase::Won,HudPhase::Dead}) {
        model.phase=phase;model.audioStatus=HudAudioStatus::Unavailable;model.resultLockRemaining=0.75f;
        model.fatalSource="ENEMY_PROJECTILE";model.kills=32;model.combatSeconds=123;
        model.hitAge=0;model.damageAge=0;model.killAge=0;
        for(auto size:{glm::ivec2(1280,720),glm::ivec2(800,600),glm::ivec2(640,480),glm::ivec2(320,240)}) {
            HudBatch a,b;
            buildHudGeometry(a,model,size.x,size.y);buildHudGeometry(b,model,size.x,size.y);
            require(!a.vertices.empty() && a.vertices.size()==b.vertices.size(),"HUD not deterministic");
            for(size_t i=0;i<a.vertices.size();++i) {
                const auto& v=a.vertices[i];
                require(std::isfinite(v.position.x) && std::isfinite(v.position.y),"invalid geometry");
                require(v.position.x>=0 && v.position.x<=size.x && v.position.y>=0 && v.position.y<=size.y,"HUD outside logical window");
                require(v.position==b.vertices[i].position && v.color==b.vertices[i].color,"identical paused snapshot changed presentation");
            }
        }
    }
    HudBatch zero;buildHudGeometry(zero,model,0,600);
    require(zero.vertices.empty(),"minimized window generated geometry");
    for(char c='A';c<='Z';++c) {
        bool ink=false;for(auto row:BitmapFontRenderer::glyph(c)) { require(row<32,"glyph exceeds five columns");ink|=row!=0; }
        require(ink,"missing uppercase glyph");
    }
}
#ifdef SILIC2_HUD_TEST_MAIN
int main() {
    try { runHudViewModelTests();std::cout<<"HUD snapshot/visibility/layout tests passed\n";return 0; }
    catch(const std::exception& error) { std::cerr<<error.what()<<'\n';return 1; }
}
#endif
