#include "particle_system.h"

namespace silic2 {

// Factory function to create enhanced ground particle system  
std::unique_ptr<GroundParticleSystem> createEnhancedGroundParticleSystem(size_t maxParticles, GroundParticleSystem::particleMode mode) {
    // Create the standard ground particle system with enhanced settings
    auto system = std::make_unique<GroundParticleSystem>(maxParticles);
    // NEW: Set the particle mode based on the parameter
    system->setParticleMode(mode);

    // NEW: Configure settings based on the selected mode
    if (mode == GroundParticleSystem::particleMode::FIRE) {
        system->setEmissionRate(50.0f);  // Particles per second for fire
        system->setFireIntensity(3.0f);  // Much brighter, more intense fire
    } else if (mode == GroundParticleSystem::particleMode::DUST) {
        system->setEmissionRate(25.0f);  // A different rate for dust
    }

    system->setEnabled(true);
    
    return system;
}

} // namespace silic2