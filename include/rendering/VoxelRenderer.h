#ifndef QUESTFARERGAMEENGINE_VOXELRENDERER_H
#define QUESTFARERGAMEENGINE_VOXELRENDERER_H

#include "MeshRenderer.h"
#include "../voxel/Grid.h"
#include <unordered_map>
using namespace logger;
using namespace voxel;

namespace rendering {
    class VoxelRenderer : public MeshRenderer {

    private:
        std::shared_ptr<Grid> grid;
        unsigned int textureId = 0;

        // One mesh per chunk
        std::unordered_map<glm::ivec2, std::unique_ptr<mesh::Mesh>> chunkMeshes;

        void rebuildChunkMesh(int xChunk, int zChunk);

        void updateDirtyChunks();

    public:
        VoxelRenderer(std::shared_ptr<Logger> logger, std::unique_ptr<Shader> shader,
                      std::shared_ptr<Grid> grid);

        void setTexture(unsigned int glTextureId);

        void setup() override;

        void draw(const RenderContext &ctx, glm::mat4 modelMatrix) override;
    };
}

#endif //QUESTFARERGAMEENGINE_VOXELRENDERER_H