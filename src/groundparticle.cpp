#include "particle_system.h"

namespace silic2 {

// Factory function to create enhanced ground particle system  
std::unique_ptr<GroundParticleSystem> createEnhancedGroundParticleSystem(size_t maxParticles) {
    // Create the standard ground particle system with enhanced settings
    auto system = std::make_unique<GroundParticleSystem>(maxParticles);
    
    // Configure for enhanced fire effect
    system->setEmissionRate(50.0f);  // Particles per second
    system->setFireIntensity(3.0f);   // Much brighter, more intense fire
    system->setEnabled(true);
    
    return system;
}

} // namespace silic2