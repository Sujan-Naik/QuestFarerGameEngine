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
        // Use unique_ptr for the grid if the renderer owns it,
        // or keep shared_ptr if it's managed by a World class.
        std::shared_ptr<voxel::Grid> grid;
        unsigned int textureId = 0;

        // Matches the optimized Grid hashing for consistency
        std::unordered_map<glm::ivec2,
                std::unique_ptr<mesh::Mesh>,
                voxel::Vector2IHash> chunkMeshes;

        // Optimization: Pre-allocated buffers to prevent frequent heap allocations during meshing
        std::vector<mesh::Vertex> vertexBuffer;
        std::vector<unsigned int> indexBuffer;

        void rebuildChunkMesh(int xChunk, int zChunk);
        void updateDirtyChunks();

        void greedyMeshChunk(
                int xChunk,
                int zChunk,
                std::vector<mesh::Vertex>& vertices,
                std::vector<unsigned int>& indices);

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

        // Initial setup for visible chunks
        void setup() override;

        // Main draw loop
        void draw(const RenderContext &ctx, glm::mat4 modelMatrix) override;

        // Helper to clear resources
        void clear();

        void unloadChunkMesh(int xChunk, int zChunk);
    };
}

#endif