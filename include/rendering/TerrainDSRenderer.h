#ifndef QUESTFARERGAMEENGINE_TERRAINDSRENDERER_H
#define QUESTFARERGAMEENGINE_TERRAINDSRENDERER_H


#include "Shader.h"
#include "../generator/Generator.h"
#include "../mesh/Mesh.h"
#include "MeshRenderer.h"

/**
 * @class TerrainDSRenderer
 * An implementation of logic used for the setup and
 */
class TerrainDSRenderer : public MeshRenderer {

private:

    std::shared_ptr<Generator> generator;

public:

    TerrainDSRenderer(std::shared_ptr<Logger> logger, std::unique_ptr<Shader> terrainShader);

    void setup() override;

    void draw(glm::vec3 cameraPosition, glm::mat4 projection, glm::mat4 view, glm::vec3 offset) override;

    void initTerrainData(std::shared_ptr<Generator> newGenerator);

};


#endif //QUESTFARERGAMEENGINE_TERRAINDSRENDERER_H
