#ifndef QUESTFARERGAMEENGINE_VOXELRENDERER_H
#define QUESTFARERGAMEENGINE_VOXELRENDERER_H

#include "MeshRenderer.h"
#include "../voxel/Grid.h"

class VoxelRenderer: public MeshRenderer{

private:

    std::shared_ptr<Grid> grid;
    unsigned int textureId = 0; // GL texture handle passed in from main
public:

    VoxelRenderer(std::shared_ptr<Logger> logger, std::unique_ptr<Shader> shader,
                  std::shared_ptr<Grid> grid);


    void setTexture(unsigned int glTextureId);

    void setup() override;

    void draw(const RenderContext& ctx, glm::mat4 modelMatrix) override;
};

#endif //QUESTFARERGAMEENGINE_VOXELRENDERER_H