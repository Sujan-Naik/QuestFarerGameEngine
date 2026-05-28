#ifndef QUESTFARERGAMEENGINE_VOXELRENDERER_H
#define QUESTFARERGAMEENGINE_VOXELRENDERER_H

#include "MeshRenderer.h"
#include "../voxel/Grid.h"
#include <unordered_map>
#include <memory>
#include <vector>
#include <string>

namespace rendering {
    class VoxelRenderer : public MeshRenderer {

    private:
        std::shared_ptr<voxel::Grid> grid;
        unsigned int textureId = 0;

        std::unordered_map<glm::ivec2,
                std::unique_ptr<mesh::Mesh>,
                voxel::Vector2IHash> chunkMeshes;

        void appendQuad(
                std::vector<mesh::Vertex>& vertices,
                std::vector<unsigned int>& indices,
                const glm::vec3& v0, const glm::vec3& v1,
                const glm::vec3& v2, const glm::vec3& v3,
                const glm::vec3& normal,
                voxel::VoxelType type,
                int width, int height,
                bool isBackFace);

    public:
        VoxelRenderer(std::shared_ptr<logger::Logger> logger,
                      std::unique_ptr<Shader> shader,
                      std::shared_ptr<voxel::Grid> grid);

        void loadTextureAtlas(const std::string& path);
        void setTexture(unsigned int glTextureId);

        void setup() override;

        void draw(const RenderContext &ctx, glm::mat4 modelMatrix) override;

        void unloadChunkMesh(int xChunk, int zChunk);

        void greedyMeshSpecificChunk(
                voxel::Chunk* chunk,
                std::vector<mesh::Vertex>& vertices,
                std::vector<unsigned int>& indices);

        void uploadManualMesh(
                int cx,
                int cz,
                std::vector<mesh::Vertex>& vertices,
                std::vector<unsigned int>& indices);
    };
}

#endif