#pragma once
#include <glm/glm.hpp>
#include <cstdint>
#include <string>
#include <vector>
namespace silic2 {
struct StaticMesh {
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> texCoords;
    std::vector<glm::vec3> normals; // Per expanded corner; authored vn or face fallback.
    std::vector<uint32_t> faces;
};
// Static Wavefront OBJ: v/vt/vn, triangles or convex quads, positive/negative indices.
// Material assignment belongs to the map instance. No external MTL dependency.
StaticMesh loadStaticObj(const std::string& path);
struct StaticModelPart {
    StaticMesh mesh;
    std::string asset, texture; // Resolved paths, confined to the bundle directory.
    glm::vec4 baseColorFactor{1};
};
struct StaticModelBundle {
    std::vector<StaticModelPart> parts;
    glm::vec3 boundsMin{0}, boundsMax{0};
};
// Offline importer manifest v1; CPU only. Geometry already has baked transforms.
// Throws on invalid bounds, resources, alpha, or an unsupported manifest version.
StaticModelBundle loadStaticModelBundle(const std::string& manifestPath);
glm::mat4 staticTransform(glm::vec3 position,glm::vec3 rotationDegrees,glm::vec3 scale);
}
