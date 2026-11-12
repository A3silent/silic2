#pragma once
#include "character/weapon_asset.h"
namespace silic2 {
// Pure CPU deterministic geometry; no files, textures, network, RNG or GL calls.
// Coordinates are camera-root local, +Y up, muzzle -Z. The optic is included.
WeaponAsset buildProceduralRifle();
// Runtime/test seam: default, missing, disabled and invalid configs all resolve
// to the procedural rifle. Only source="imported" requests the legacy adapter.
WeaponAsset resolveWeaponPresentation(const std::string& configPath, std::string& status);
}
