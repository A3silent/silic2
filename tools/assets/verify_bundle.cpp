// CPU-only fixture probe for the offline importer's engine roundtrip.
#include "engine/mesh_asset.h"
#include <iostream>
#include <stdexcept>
int main(int argc,char** argv) {
    try {
        if(argc!=2)throw std::runtime_error("usage: verify-static-bundle model.import.json");
        const auto bundle=silic2::loadStaticModelBundle(argv[1]);
        size_t triangles=0;
        for(const auto& part:bundle.parts) {
            if(part.mesh.normals.size()!=part.mesh.vertices.size() || part.mesh.texCoords.size()!=part.mesh.vertices.size())throw std::runtime_error("inconsistent vertex attributes");
            triangles+=part.mesh.faces.size()/3;
        }
        std::cout<<bundle.parts.size()<<" parts, "<<triangles<<" triangles validated by engine loader\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
