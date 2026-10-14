#pragma once
#include <glm/glm.hpp>
#include <cstdint>
#include <string>
#include <vector>
namespace silic2 {
struct StaticMesh {
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> texCoords;
    std::vector<uint32_t> faces;
};
// Static Wavefront OBJ: v/vt/vn, triangles or convex quads, positive/negative indices.
// Material assignment belongs to the map instance. No external MTL dependency.
StaticMesh loadStaticObj(const std::string& path);
glm::mat4 staticTransform(glm::vec3 position,glm::vec3 rotationDegrees,glm::vec3 scale);
}
