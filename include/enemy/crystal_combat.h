#pragma once
#include "game/gameplay_types.h"
#include <glm/glm.hpp>
#include <array>

namespace silic2::crystalCombat {
constexpr float bodyScale(EnemyType type) {
    return type==EnemyType::VeilRay ? 2.5f : type==EnemyType::HangingCenser ? 3.5f : 1.f;
}
constexpr glm::vec3 HoundSize{1.1f,1.35f,2.8f};
constexpr glm::vec3 RaySize=glm::vec3(3.5f,.65f,1.9f)*bodyScale(EnemyType::VeilRay);
constexpr glm::vec3 CenserSize=glm::vec3(1.6f,2.4f,1.6f)*bodyScale(EnemyType::HangingCenser);
constexpr glm::vec3 CenserMouth{0,.88f,.52f};
constexpr glm::vec3 CenserMouthOffset=CenserMouth*bodyScale(EnemyType::HangingCenser);
// The generated Crystal floor is wider than its low mineral rim (inner face at ~100 m).
constexpr float CrystalFlightRadius=99.f;
constexpr float HeavySpacing=1.5f;
constexpr int HeavyLimit=2;
constexpr int HoundHp=9, RayHp=14, CenserHp=24;
constexpr float HoundMinRange=6, HoundMaxRange=12, RayAltitude=5, CenserAltitude=6;
constexpr float CenserMinRange=15, CenserMaxRange=28, RayCruise=16, CenserCruise=12;
constexpr float HoundApproach=11, HoundWindup=.7f, HoundTracking=.3f;
constexpr float DashSpeed=22, DashLength=14, DashDamage=18, DashRecovery=1;
constexpr float SweepLength=14, SweepWidth=2.4f, SweepTime=1, RayWindup=.85f;
constexpr int SweepSegments=7;
constexpr float SweepDamage=12, RayRecovery=3;
constexpr float CenserWindup=1.2f, CenserRecovery=3, InterruptTime=1.2f;
constexpr float RayLockLead=1.35f, RayStagingDistance=9, RayBankSpeed=5;
constexpr float CenserDrift=5, FlightRise=7;
constexpr float CenserOrbitSpeed=1.35f, CenserLiftSpeed=.75f;
constexpr float CenserBobHeight=.18f, CenserBobRate=1.65f;
constexpr float CenserDropDistance=1, CenserDropSpeed=4;
constexpr int GlandHitLimit=3;
constexpr float VolleyTime=1, BurstRadius=2, OrbDamage=10, VolleyDamageLimit=15;
inline bool isCrystalEnemy(EnemyType t) {
    return t==EnemyType::ForkHound || t==EnemyType::VeilRay || t==EnemyType::HangingCenser;
}
inline std::array<glm::vec3,3> volleyTargets(glm::vec3 center) {
    return {{center+glm::vec3(0,0,1.45f),center+glm::vec3(1.255f,0,-.725f),center+glm::vec3(-1.255f,0,-.725f)}};
}
}
