#ifndef QUESTFARERGAMEENGINE_TERRAINRENDERER_H
#define QUESTFARERGAMEENGINE_TERRAINRENDERER_H


#include "Shader.h"
#include "../generator/Generator.h"
#include "../mesh/Mesh.h"
#include "MeshRenderer.h"

/**
 * @class TerrainRenderer
 * An implementation of logic used for the setup and
 */
class TerrainRenderer : public MeshRenderer {

private:

    std::shared_ptr<Generator> generator;

public:

    TerrainRenderer(std::shared_ptr<Logger> logger, std::unique_ptr<Shader> terrainShader);

    void setup() override;

    void draw(glm::vec3 cameraPosition, glm::mat4 projection, glm::mat4 view, glm::vec3 offset) override;

    void initTerrainData(std::shared_ptr<Generator> newGenerator);

};


#endif //QUESTFARERGAMEENGINE_TERRAINRENDERER_H
