#pragma once

#include "particle_system.h"
#include <memory>

namespace silic2 {

// Factory function to create enhanced ground particle system with fire effects
// This creates a particle system that:
// - Spawns particles randomly across entire floor surfaces
// - Creates large, glowing particles like bullets
// - Uses orange/yellow/red fire colors
// - Particles live for exactly 2.5 seconds
// - Particles float upward from the ground
std::unique_ptr<GroundParticleSystem> createEnhancedGroundParticleSystem(size_t maxParticles, GroundParticleSystem::GParticleMode mode);

} // namespace silic2