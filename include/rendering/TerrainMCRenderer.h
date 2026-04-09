#ifndef QUESTFARERGAMEENGINE_TERRAINMCRENDERER_H
#define QUESTFARERGAMEENGINE_TERRAINMCRENDERER_H

#include "Shader.h"
#include "../mesh/Mesh.h"
#include "MeshRenderer.h"
#include "../logger/Logger.h"
#include <memory>
#include <optional>

class TerrainMCRenderer : public MeshRenderer {

private:
    std::optional<Mesh> mesh;   // owns the VAO/VBO/EBO via the MeshAnimation class
    unsigned int textureId = 0; // GL texture handle passed in from main

public:
    TerrainMCRenderer(std::shared_ptr<Logger> logger, std::unique_ptr<Shader> shader);

    // Call before setup() so the mesh can register the grass texture
    void setTexture(unsigned int glTextureId);

    void setup() override;

    void draw(const RenderContext& ctx, glm::mat4 modelMatrix) override;

};

#endif //QUESTFARERGAMEENGINE_TERRAINMCRENDERER_H