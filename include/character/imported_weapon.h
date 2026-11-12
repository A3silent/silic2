#pragma once
#include "character/weapon_asset.h"
#include <memory>
namespace silic2 {
// Cached rigid mesh renderer with a private depth attachment. No world depth
// clearing, no sorting/uploading imported triangles in the combat frame loop.
class ImportedWeapon {
public:
    ImportedWeapon();
    ~ImportedWeapon();
    bool init(const WeaponAsset& asset, std::string& status);
    bool render(const glm::mat4& root, const glm::mat4& projection);
    glm::vec3 muzzlePosition(const glm::mat4& root) const;
    glm::vec3 muzzleDirection(const glm::mat4& root) const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
}
