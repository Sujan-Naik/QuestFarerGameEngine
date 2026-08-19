#include "stb/stb_image.h"
#include <glm/gtc/matrix_transform.hpp>
#include "../../include/rendering/VoxelRenderer.h"
#include <iostream>
#include <array>
#include <algorithm>

using namespace rendering;
using namespace rendering::mesh;
using namespace voxel;

// Atlas:
// 1024 x 1024 pixels
// 64 x 64 pixels per tile
// 16 x 16 tiles
constexpr float ATLAS_TILE_SIZE = 1.0f / 16.0f;

// Source atlas tile coordinates, converted for the fact that the image
// is vertically flipped by stb_image.
//
// Source atlas:
// Dirt       = (0, 0)
// Grass side = (1, 0)
// Grass top  = (2, 0)
// Stone      = (3, 0)
//
// After stbi_set_flip_vertically_on_load(true):
// Dirt       = (0, 15)
// Grass side = (1, 15)
// Grass top  = (2, 15)
// Stone      = (3, 15)
constexpr glm::vec2 DIRT_TILE       = {0.0f, 15.0f};
constexpr glm::vec2 GRASS_SIDE_TILE = {1.0f, 15.0f};
constexpr glm::vec2 GRASS_TOP_TILE  = {2.0f, 15.0f};
constexpr glm::vec2 STONE_TILE      = {3.0f, 15.0f};

namespace {

glm::vec2 getAtlasTile(
        VoxelType type,
        const glm::vec3& normal) {

    switch (type) {
        case VoxelType::DIRT:
            return DIRT_TILE;

        case VoxelType::GRASS:
            // +Y is grass top.
            if (normal.y > 0.5f) {
                return GRASS_TOP_TILE;
            }

            // -Y uses dirt on the grass block.
            if (normal.y < -0.5f) {
                return DIRT_TILE;
            }

            // Horizontal grass faces use the grass-side tile.
            return GRASS_SIDE_TILE;

        case VoxelType::STONE:
            return STONE_TILE;

        case VoxelType::AIR:
        default:
            // AIR should never produce a face.
            return DIRT_TILE;
    }
}

}

VoxelRenderer::VoxelRenderer(
        std::shared_ptr<logger::Logger> logger,
        std::unique_ptr<Shader> shader,
        std::shared_ptr<Grid> grid)
        : MeshRenderer(
                std::move(logger),
                std::move(shader)
          ),
          grid(std::move(grid)) {
}

void VoxelRenderer::unloadChunkMesh(int xChunk, int zChunk) {
    auto it = chunkMeshes.find({xChunk, zChunk});

    if (it != chunkMeshes.end()) {
        chunkMeshes.erase(it);
    }
}

void VoxelRenderer::loadTextureAtlas(const std::string& path) {
    int width = 0;
    int height = 0;
    int nrChannels = 0;

    stbi_set_flip_vertically_on_load(true);

    unsigned char* data =
            stbi_load(
                    path.c_str(),
                    &width,
                    &height,
                    &nrChannels,
                    0
            );

    if (!data) {
        std::cerr
                << "Failed to load voxel texture atlas: "
                << path
                << "\nReason: "
                << stbi_failure_reason()
                << std::endl;

        return;
    }

    GLenum format;

    if (nrChannels == 4) {
        format = GL_RGBA;
    }
    else if (nrChannels == 3) {
        format = GL_RGB;
    }
    else {
        std::cerr
                << "Unsupported texture atlas channel count: "
                << nrChannels
                << std::endl;

        stbi_image_free(data);
        return;
    }

    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);

    glTexImage2D(
            GL_TEXTURE_2D,
            0,
            format,
            width,
            height,
            0,
            format,
            GL_UNSIGNED_BYTE,
            data
    );

    // Do not generate mipmaps for the atlas.
    // They can bleed neighbouring tiles together.
    glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_MIN_FILTER,
            GL_NEAREST
    );

    glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_MAG_FILTER,
            GL_NEAREST
    );

    glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_WRAP_S,
            GL_CLAMP_TO_EDGE
    );

    glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_WRAP_T,
            GL_CLAMP_TO_EDGE
    );

    stbi_image_free(data);

    std::cout
            << "Loaded voxel texture atlas: "
            << path
            << " ("
            << width
            << "x"
            << height
            << ", "
            << nrChannels
            << " channels)"
            << std::endl;
}

void VoxelRenderer::appendQuad(
        std::vector<mesh::Vertex>& vertices,
        std::vector<unsigned int>& indices,
        const glm::vec3& v0,
        const glm::vec3& v1,
        const glm::vec3& v2,
        const glm::vec3& v3,
        const glm::vec3& normal,
        voxel::VoxelType type,
        int w,
        int h,
        bool isBackFace) {

    const unsigned int baseIndex =
            static_cast<unsigned int>(vertices.size());

    // IMPORTANT:
    //
    // TexCoords are LOCAL coordinates and may span multiple blocks:
    //
    //     0 -> w
    //     0 -> h
    //
    // The atlas tile is stored separately in AtlasTile.
    //
    // This avoids interpolating atlas tile coordinates across a greedy quad.
    const glm::vec2 atlasTile =
            getAtlasTile(type, normal);

    const glm::vec2 uvs[4] = {
            {0.0f, static_cast<float>(h)},
            {static_cast<float>(w), static_cast<float>(h)},
            {static_cast<float>(w), 0.0f},
            {0.0f, 0.0f}
    };

    vertices.push_back({
            v0,
            normal,
            uvs[0],
            glm::vec3(0.0f),
            glm::vec3(0.0f),
            atlasTile
    });

    vertices.push_back({
            v1,
            normal,
            uvs[1],
            glm::vec3(0.0f),
            glm::vec3(0.0f),
            atlasTile
    });

    vertices.push_back({
            v2,
            normal,
            uvs[2],
            glm::vec3(0.0f),
            glm::vec3(0.0f),
            atlasTile
    });

    vertices.push_back({
            v3,
            normal,
            uvs[3],
            glm::vec3(0.0f),
            glm::vec3(0.0f),
            atlasTile
    });

    if (!isBackFace) {
        indices.insert(
                indices.end(),
                {
                        baseIndex + 0,
                        baseIndex + 1,
                        baseIndex + 2,

                        baseIndex + 2,
                        baseIndex + 3,
                        baseIndex + 0
                }
        );
    }
    else {
        indices.insert(
                indices.end(),
                {
                        baseIndex + 0,
                        baseIndex + 3,
                        baseIndex + 2,

                        baseIndex + 2,
                        baseIndex + 1,
                        baseIndex + 0
                }
        );
    }
}

void VoxelRenderer::greedyMeshSpecificChunk(
        voxel::Chunk* chunk,
        std::vector<mesh::Vertex>& vertices,
        std::vector<unsigned int>& indices) {

    static thread_local std::vector<int> mask(16 * 256);
    static thread_local std::vector<VoxelType> typeMask(16 * 256);

    for (int d = 0; d < 3; ++d) {
        const int u = (d + 1) % 3;
        const int v = (d + 2) % 3;

        std::array<int, 3> x = {0, 0, 0};
        std::array<int, 3> q = {0, 0, 0};

        std::fill(
                mask.begin(),
                mask.end(),
                0
        );

        q[d] = 1;

        for (x[d] = -1;
             x[d] < (d == 1 ? 256 : 16);) {

            int n = 0;

            for (x[v] = 0;
                 x[v] < (v == 1 ? 256 : 16);
                 ++x[v]) {

                for (x[u] = 0;
                     x[u] < (u == 1 ? 256 : 16);
                     ++x[u]) {

                    const VoxelType a =
                            chunk->GetVoxel(
                                    x[0],
                                    x[1],
                                    x[2]
                            );

                    const VoxelType b =
                            chunk->GetVoxel(
                                    x[0] + q[0],
                                    x[1] + q[1],
                                    x[2] + q[2]
                            );

                    if ((a != VoxelType::AIR) ==
                        (b != VoxelType::AIR)) {

                        mask[n] = 0;
                    }
                    else if (a != VoxelType::AIR) {

                        mask[n] = 1;
                        typeMask[n] = a;
                    }
                    else {

                        mask[n] = -1;
                        typeMask[n] = b;
                    }

                    ++n;
                }
            }

            ++x[d];
            n = 0;

            const int outerLimit =
                    (v == 1 ? 256 : 16);

            const int innerLimit =
                    (u == 1 ? 256 : 16);

            for (int j = 0; j < outerLimit; ++j) {

                for (int i = 0; i < innerLimit;) {

                    if (mask[n] != 0) {

                        const int currentMask = mask[n];
                        const VoxelType currentType = typeMask[n];

                        int w;

                        for (w = 1;
                             i + w < innerLimit &&
                             mask[n + w] == currentMask &&
                             typeMask[n + w] == currentType;
                             ++w) {
                        }

                        int h;
                        bool done = false;

                        for (h = 1;
                             j + h < outerLimit;
                             ++h) {

                            for (int k = 0; k < w; ++k) {

                                if (mask[n + k + h * innerLimit] != currentMask ||
                                    typeMask[n + k + h * innerLimit] != currentType) {

                                    done = true;
                                    break;
                                }
                            }

                            if (done) {
                                break;
                            }
                        }

                        x[u] = i;
                        x[v] = j;

                        std::array<int, 3> du = {
                                0, 0, 0
                        };

                        std::array<int, 3> dv = {
                                0, 0, 0
                        };

                        du[u] = w;
                        dv[v] = h;

                        glm::vec3 v0(
                                x[0],
                                x[1],
                                x[2]
                        );

                        glm::vec3 v1 =
                                v0 +
                                glm::vec3(
                                        du[0],
                                        du[1],
                                        du[2]
                                );

                        glm::vec3 v2 =
                                v0 +
                                glm::vec3(
                                        du[0] + dv[0],
                                        du[1] + dv[1],
                                        du[2] + dv[2]
                                );

                        glm::vec3 v3 =
                                v0 +
                                glm::vec3(
                                        dv[0],
                                        dv[1],
                                        dv[2]
                                );

                        glm::vec3 norm(0.0f);

                        norm[d] =
                                static_cast<float>(currentMask);

                        appendQuad(
                                vertices,
                                indices,
                                v0,
                                v1,
                                v2,
                                v3,
                                norm,
                                currentType,
                                w,
                                h,
                                currentMask < 0
                        );

                        for (int l = 0; l < h; ++l) {
                            for (int k = 0; k < w; ++k) {

                                mask[
                                        n +
                                        k +
                                        l * innerLimit
                                ] = 0;
                            }
                        }

                        i += w;
                        n += w;
                    }
                    else {
                        ++i;
                        ++n;
                    }
                }
            }
        }
    }
}

void VoxelRenderer::uploadManualMesh(
        int cx,
        int cz,
        std::vector<mesh::Vertex>& vertices,
        std::vector<unsigned int>& indices) {

    mesh::Texture tex;

    tex.id = textureId;
    tex.type = "texture_diffuse";

    chunkMeshes[{cx, cz}] =
            std::make_unique<mesh::Mesh>(
                    vertices,
                    indices,
                    std::vector<mesh::Texture>{tex}
            );
}

void VoxelRenderer::setup() {
    for (auto const& [pos, chunk] : grid->chunks) {

        std::vector<mesh::Vertex> vertices;
        std::vector<unsigned int> indices;

        greedyMeshSpecificChunk(
                chunk.get(),
                vertices,
                indices
        );

        uploadManualMesh(
                pos.x,
                pos.y,
                vertices,
                indices
        );
    }
}

void VoxelRenderer::draw(
        const RenderContext& ctx,
        glm::mat4 modelMatrix) {

    shader->use();

    shader->setMat4(
            "projection",
            ctx.projection
    );

    shader->setMat4(
            "view",
            ctx.view
    );

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(
            GL_TEXTURE_2D,
            textureId
    );

    shader->setInt(
            "texture_diffuse1",
            0
    );

    for (auto const& [pos, mesh] : chunkMeshes) {

        glm::mat4 chunkMatrix =
                glm::translate(
                        modelMatrix,
                        glm::vec3(
                                static_cast<float>(pos.x << 4),
                                0.0f,
                                static_cast<float>(pos.y << 4)
                        )
                );

        shader->setMat4(
                "model",
                chunkMatrix
        );

        mesh->Draw(*shader);
    }
}