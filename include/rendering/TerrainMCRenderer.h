#ifndef QUESTFARERGAMEENGINE_TERRAINMCRENDERER_H
#define QUESTFARERGAMEENGINE_TERRAINMCRENDERER_H

#include "Shader.h"
#include "../generator/Generator.h"
#include "../mesh/Mesh.h"
#include "MeshRenderer.h"
#include <memory>
#include <optional>

class TerrainMCRenderer : public MeshRenderer {

private:
    std::shared_ptr<Generator> generator;
    std::optional<Mesh> mesh;   // owns the VAO/VBO/EBO via the Mesh class
    unsigned int textureId = 0; // GL texture handle passed in from main

public:
    TerrainMCRenderer(std::shared_ptr<Logger> logger, std::unique_ptr<Shader> shader);

    // Call before setup() so the mesh can register the grass texture
    void setTexture(unsigned int glTextureId);

    void setup() override;

    void draw(glm::vec3 cameraPosition, glm::mat4 projection,
              glm::mat4 view, glm::vec3 offset) override;

    void initTerrainData(std::shared_ptr<Generator> newGenerator);
};

#endif //QUESTFARERGAMEENGINE_TERRAINMCRENDERER_H