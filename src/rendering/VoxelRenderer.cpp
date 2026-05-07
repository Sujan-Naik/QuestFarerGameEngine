#include "stb/stb_image.h"
#include <glm/gtc/matrix_transform.hpp>
#include "../../include/rendering/VoxelRenderer.h"
#include <iostream>

using namespace rendering;
using namespace rendering::mesh;
using namespace voxel;

const float TILE_SIZE = 1.0f / 16.0f;

VoxelRenderer::VoxelRenderer(std::shared_ptr<logger::Logger> logger, std::unique_ptr<Shader> shader,
                             std::shared_ptr<Grid> grid)
        : MeshRenderer(std::move(logger), std::move(shader)), grid(std::move(grid)) {
    // Pre-reserve memory to prevent reallocations during "insane" loading
    vertexBuffer.reserve(5000);
    indexBuffer.reserve(7500);
}

void VoxelRenderer::unloadChunkMesh(int xChunk, int zChunk) {
    auto it = chunkMeshes.find({xChunk, zChunk});
    if (it != chunkMeshes.end()) {
        chunkMeshes.erase(it);
    }
}

void VoxelRenderer::loadTextureAtlas(const std::string& path) {
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);

    if (data) {
        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        stbi_image_free(data);
    }
}

glm::vec2 getTileOffset(VoxelType type) {
    switch (type) {
        case VoxelType::GRASS: return {0.0f, 15.0f};
        case VoxelType::DIRT:  return {1.0f, 15.0f};
        case VoxelType::STONE: return {2.0f, 15.0f};
        default:               return {15.0f, 0.0f};
    }
}

void VoxelRenderer::appendQuad(
        std::vector<mesh::Vertex>& vertices, std::vector<unsigned int>& indices,
        const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3,
        const glm::vec3& normal, voxel::VoxelType type, int w, int h, bool isBackFace) {

    unsigned int baseIndex = static_cast<unsigned int>(vertices.size());
    glm::vec2 offset = getTileOffset(type);

    float uMin = offset.x * TILE_SIZE;
    float vMin = offset.y * TILE_SIZE;
    float uMax = uMin + (TILE_SIZE * w);
    float vMax = vMin + (TILE_SIZE * h);

    glm::vec2 uvs[4] = {{uMin, vMax}, {uMax, vMax}, {uMax, vMin}, {uMin, vMin}};

    vertices.push_back({v0, normal, uvs[0], glm::vec3(0), glm::vec3(0)});
    vertices.push_back({v1, normal, uvs[1], glm::vec3(0), glm::vec3(0)});
    vertices.push_back({v2, normal, uvs[2], glm::vec3(0), glm::vec3(0)});
    vertices.push_back({v3, normal, uvs[3], glm::vec3(0), glm::vec3(0)});

    if (!isBackFace) {
        indices.insert(indices.end(), {baseIndex+0, baseIndex+1, baseIndex+2, baseIndex+2, baseIndex+3, baseIndex+0});
    } else {
        indices.insert(indices.end(), {baseIndex+0, baseIndex+3, baseIndex+2, baseIndex+2, baseIndex+1, baseIndex+0});
    }
}

void VoxelRenderer::greedyMeshChunk(int cx, int cz, std::vector<mesh::Vertex>& vertices, std::vector<unsigned int>& indices) {
    auto it = grid->chunks.find({cx, cz});
    if (it == grid->chunks.end()) return;

    Chunk* chunk = it->second.get();

    for (int d = 0; d < 3; ++d) {
        int u = (d + 1) % 3, v = (d + 2) % 3;
        std::array<int, 3> x = {0, 0, 0}, q = {0, 0, 0};
        std::vector<int> mask(16 * 256); // Optimized size for 16x256x16
        std::vector<VoxelType> typeMask(16 * 256);

        q[d] = 1;
        for (x[d] = -1; x[d] < (d == 1 ? 256 : 16);) {
            int n = 0;
            for (x[v] = 0; x[v] < (v == 1 ? 256 : 16); ++x[v]) {
                for (x[u] = 0; x[u] < (u == 1 ? 256 : 16); ++x[u]) {
                    // Use internal bit-shifting for GetVoxel lookups
                    VoxelType a = chunk->GetVoxel(x[0], x[1], x[2]);
                    VoxelType b = chunk->GetVoxel(x[0] + q[0], x[1] + q[1], x[2] + q[2]);

                    if ((a != VoxelType::AIR) == (b != VoxelType::AIR)) {
                        mask[n] = 0;
                    } else if (a != VoxelType::AIR) {
                        mask[n] = 1; typeMask[n] = a;
                    } else {
                        mask[n] = -1; typeMask[n] = b;
                    }
                    n++;
                }
            }

            x[d]++; n = 0;
            int outerLimit = (v == 1 ? 256 : 16);
            int innerLimit = (u == 1 ? 256 : 16);

            for (int j = 0; j < outerLimit; ++j) {
                for (int i = 0; i < innerLimit;) {
                    if (mask[n] != 0) {
                        int currentMask = mask[n];
                        VoxelType currentType = typeMask[n];
                        int w, h;
                        for (w = 1; i + w < innerLimit && mask[n + w] == currentMask && typeMask[n + w] == currentType; ++w);

                        bool done = false;
                        for (h = 1; j + h < outerLimit; ++h) {
                            for (int k = 0; k < w; ++k) {
                                if (mask[n + k + h * innerLimit] != currentMask || typeMask[n + k + h * innerLimit] != currentType) {
                                    done = true; break;
                                }
                            }
                            if (done) break;
                        }

                        x[u] = i; x[v] = j;
                        std::array<int, 3> du = {0, 0, 0}; du[u] = w;
                        std::array<int, 3> dv = {0, 0, 0}; dv[v] = h;
                        glm::vec3 v0(x[0], x[1], x[2]);
                        glm::vec3 v1 = v0 + glm::vec3(du[0], du[1], du[2]);
                        glm::vec3 v2 = v0 + glm::vec3(du[0] + dv[0], du[1] + dv[1], du[2] + dv[2]);
                        glm::vec3 v3 = v0 + glm::vec3(dv[0], dv[1], dv[2]);

                        glm::vec3 norm(0); norm[d] = (float)currentMask;
                        appendQuad(vertices, indices, v0, v1, v2, v3, norm, currentType, w, h, currentMask < 0);

                        for (int l = 0; l < h; ++l)
                            for (int k = 0; k < w; ++k) mask[n + k + l * innerLimit] = 0;
                        i += w; n += w;
                    } else { i++; n++; }
                }
            }
        }
    }
}

void VoxelRenderer::rebuildChunkMesh(int cx, int cz) {
    vertexBuffer.clear();
    indexBuffer.clear();

    greedyMeshChunk(cx, cz, vertexBuffer, indexBuffer);

    if (vertexBuffer.empty()) {
        chunkMeshes.erase({cx, cz});
        return;
    }

    mesh::Texture tex;
    tex.id = textureId;
    tex.type = "texture_diffuse";

    // Efficiently replace or create the mesh
    chunkMeshes[{cx, cz}] = std::make_unique<mesh::Mesh>(
            vertexBuffer, indexBuffer, std::vector<mesh::Texture>{tex}
    );
}

void VoxelRenderer::updateDirtyChunks() {
    int count = 0;
    for (auto const& [pos, chunk] : grid->chunks) {
        if (chunk->isDirty()) {
            rebuildChunkMesh(pos.x, pos.y);
            chunk->clearDirty();
            if (++count > 4) break; // Limit rebuilds per frame to keep FPS smooth
        }
    }
}

void VoxelRenderer::setup() {
    for (auto const& [pos, chunk] : grid->chunks) {
        rebuildChunkMesh(pos.x, pos.y);
    }
}

void VoxelRenderer::draw(const RenderContext &ctx, glm::mat4 modelMatrix) {
    updateDirtyChunks();
    shader->use();
    shader->setMat4("projection", ctx.projection);
    shader->setMat4("view", ctx.view);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);

    for (auto const& [pos, mesh] : chunkMeshes) {
        // Only draw if within a reasonable distance (Basic Culling)
        glm::mat4 chunkMatrix = glm::translate(modelMatrix,
                                               glm::vec3(pos.x << 4, 0.0f, pos.y << 4)); // Shift instead of multiply
        shader->setMat4("model", chunkMatrix);
        mesh->Draw(*shader);
    }
}