#include "hud/hud_view_model.h"
#include <algorithm>
#include <cmath>
#include <cctype>
#include <iomanip>
#include <sstream>

namespace silic2 {
std::string hudPhaseLabel(const HudViewModel& m) {
    if (!m.arenaSurvival) return m.livingEnemies>0 ? "CLEAR" : "EXPLORE";
    if (m.cleanup) return "CLEANUP";
    if (m.hasTransformation && m.combatSeconds>=m.transformationStart) {
        return m.combatSeconds>=m.transformationStart+m.transformationDuration ? "AFTERSHIFT" : "SHIFTING";
    }
    return "SURVIVE";
}
std::string hudObjectiveLabel(const HudViewModel& m) {
    if (!m.arenaSurvival && m.livingEnemies==0) return "EXPLORE THE MAP";
    if (!m.arenaSurvival || m.cleanup) return std::to_string(m.livingEnemies)+" HOSTILES LEFT";
    return "HOLD UNTIL 01:30";
}
std::string hudReadyObjective(const HudViewModel& m) {
    return m.arenaSurvival ? "SURVIVE 90 SECONDS. CLEAR THE ARENA." : "CLEAR EXISTING HOSTILES. EXPLORE.";
}

bool hudEnemyVisible(const HudEnemy& e) {
    return e.alive && e.inView && e.unobstructed;
}
bool hudEnemyBarVisible(const HudEnemy& e) {
    return hudEnemyVisible(e) && std::isfinite(e.secondsSinceDamage) &&
           e.secondsSinceDamage >= 0.0f && e.secondsSinceDamage <= 2.0f &&
           e.healthAnchor.x >= 0.0f && e.healthAnchor.x <= 1.0f &&
           e.healthAnchor.y >= 0.0f && e.healthAnchor.y <= 1.0f;
}
std::string hudClock(float seconds) {
    const int value = std::isfinite(seconds) ? static_cast<int>(std::clamp(seconds, 0.0f, 59999.0f)) : 0;
    std::ostringstream out;
    out << std::setfill('0') << std::setw(2) << value / 60 << ':' << std::setw(2) << value % 60;
    return out.str();
}
std::string hudAccuracy(int hits, int shots) {
    if (shots <= 0) return "--";
    return std::to_string(static_cast<int>(std::lround(100.0 * std::clamp(hits, 0, shots) / shots))) + "%";
}
std::string hudHpLoss(float amount) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(1) << (std::isfinite(amount) ? std::max(amount,0.0f):0.0f);
    return out.str();
}
std::string hudFatalSource(const std::string& source) {
    std::string label=source;
    for(char& c:label) c=static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    if(label=="ENEMY_PROJECTILE") return "SENTRY PROJECTILE";
    if(label=="ENEMY_CONTACT") return "CHASER CONTACT";
    if(label=="SELF_DAMAGE") return "SELF DAMAGE";
    for(char& c:label) if(c=='_') c=' ';
    return label.empty()?"UNKNOWN":label;
}
} // namespace silic2
