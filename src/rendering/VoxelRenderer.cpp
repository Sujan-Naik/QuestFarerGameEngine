#include <utility>
#include "../../include/globals.h"
#include "../../include/rendering/VoxelRenderer.h"
#include <glm/gtc/matrix_transform.hpp>
using namespace logger;
using namespace rendering;
using namespace rendering::mesh;

VoxelRenderer::VoxelRenderer(std::shared_ptr<Logger> logger, std::unique_ptr<Shader> shader,
                             std::shared_ptr<Grid> grid)
        : MeshRenderer(std::move(logger), std::move(shader)), grid(std::move(grid)) {
}

void VoxelRenderer::setTexture(unsigned int glTextureId) {
    textureId = glTextureId;
}

void VoxelRenderer::setup() {
    // Build mesh for each chunk
    for (int xChunk = 0; xChunk < X_CHUNKS; xChunk++) {
        for (int zChunk = 0; zChunk < Z_CHUNKS; zChunk++) {
            rebuildChunkMesh(xChunk, zChunk);
        }
    }
}

void addVoxelGeometry(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices,
                      int x, int y, int z, std::shared_ptr<Chunk> chunk) {

    glm::vec3 pos(x, y, z);
    unsigned int baseIndex = vertices.size();

    // Helper to check if neighbor is solid
    auto isSolid = [chunk](int nx, int ny, int nz) -> bool {
        if (nx < 0 || nx >= X_CHUNK_SIZE || ny < 0 || ny >= Y_CHUNK_SIZE || nz < 0 || nz >= Z_CHUNK_SIZE)
            return false;
        return chunk->voxels[nx][ny][nz] != VoxelType::AIR;
    };

    // +X face (right)
    if (!isSolid(x+1, y, z)){
        vertices.push_back({{pos.x+1, pos.y+0, pos.z+0}, {1,0,0}, {0,0}});
        vertices.push_back({{pos.x+1, pos.y+1, pos.z+0}, {1,0,0}, {0,1}});
        vertices.push_back({{pos.x+1, pos.y+1, pos.z+1}, {1,0,0}, {1,1}});
        vertices.push_back({{pos.x+1, pos.y+0, pos.z+1}, {1,0,0}, {1,0}});
        indices.insert(indices.end(), {baseIndex+0, baseIndex+1, baseIndex+2, baseIndex+0, baseIndex+2, baseIndex+3});
        baseIndex += 4;
    }

    // -X face (left)
    if (!isSolid(x-1, y, z)){
        vertices.push_back({{pos.x+0, pos.y+0, pos.z+1}, {-1,0,0}, {0,0}});
        vertices.push_back({{pos.x+0, pos.y+1, pos.z+1}, {-1,0,0}, {0,1}});
        vertices.push_back({{pos.x+0, pos.y+1, pos.z+0}, {-1,0,0}, {1,1}});
        vertices.push_back({{pos.x+0, pos.y+0, pos.z+0}, {-1,0,0}, {1,0}});
        indices.insert(indices.end(), {baseIndex+0, baseIndex+1, baseIndex+2, baseIndex+0, baseIndex+2, baseIndex+3});
        baseIndex += 4;
    }

    // +Y face (top)
    if (!isSolid(x, y+1, z)){
        vertices.push_back({{pos.x+0, pos.y+1, pos.z+0}, {0,1,0}, {0,0}});
        vertices.push_back({{pos.x+1, pos.y+1, pos.z+0}, {0,1,0}, {1,0}});
        vertices.push_back({{pos.x+1, pos.y+1, pos.z+1}, {0,1,0}, {1,1}});
        vertices.push_back({{pos.x+0, pos.y+1, pos.z+1}, {0,1,0}, {0,1}});
        indices.insert(indices.end(), {baseIndex+0, baseIndex+1, baseIndex+2, baseIndex+0, baseIndex+2, baseIndex+3});
        baseIndex += 4;
    }

    // -Y face (bottom)
    if (!isSolid(x, y-1, z)){
        vertices.push_back({{pos.x+0, pos.y+0, pos.z+1}, {0,-1,0}, {0,0}});
        vertices.push_back({{pos.x+1, pos.y+0, pos.z+1}, {0,-1,0}, {1,0}});
        vertices.push_back({{pos.x+1, pos.y+0, pos.z+0}, {0,-1,0}, {1,1}});
        vertices.push_back({{pos.x+0, pos.y+0, pos.z+0}, {0,-1,0}, {0,1}});
        indices.insert(indices.end(), {baseIndex+0, baseIndex+1, baseIndex+2, baseIndex+0, baseIndex+2, baseIndex+3});
        baseIndex += 4;
    }

    // +Z face (front)
    if (!isSolid(x, y, z+1)){
        vertices.push_back({{pos.x+1, pos.y+0, pos.z+1}, {0,0,1}, {0,0}});
        vertices.push_back({{pos.x+1, pos.y+1, pos.z+1}, {0,0,1}, {0,1}});
        vertices.push_back({{pos.x+0, pos.y+1, pos.z+1}, {0,0,1}, {1,1}});
        vertices.push_back({{pos.x+0, pos.y+0, pos.z+1}, {0,0,1}, {1,0}});
        indices.insert(indices.end(), {baseIndex+0, baseIndex+1, baseIndex+2, baseIndex+0, baseIndex+2, baseIndex+3});
        baseIndex += 4;
    }

    // -Z face (back)
    if (!isSolid(x, y, z-1)){
        vertices.push_back({{pos.x+0, pos.y+0, pos.z+0}, {0,0,-1}, {0,0}});
        vertices.push_back({{pos.x+0, pos.y+1, pos.z+0}, {0,0,-1}, {0,1}});
        vertices.push_back({{pos.x+1, pos.y+1, pos.z+0}, {0,0,-1}, {1,1}});
        vertices.push_back({{pos.x+1, pos.y+0, pos.z+0}, {0,0,-1}, {1,0}});
        indices.insert(indices.end(), {baseIndex+0, baseIndex+1, baseIndex+2, baseIndex+0, baseIndex+2, baseIndex+3});
        baseIndex += 4;
    }
}

void VoxelRenderer::rebuildChunkMesh(int xChunk, int zChunk) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    vertices.reserve(X_CHUNK_SIZE * Y_CHUNK_SIZE * Z_CHUNK_SIZE * 24);
    indices.reserve(X_CHUNK_SIZE * Y_CHUNK_SIZE * Z_CHUNK_SIZE * 36);

    std::shared_ptr<Chunk> chunk = grid->chunks[{xChunk, zChunk}];

    for (int x = 0; x < X_CHUNK_SIZE; x++) {
        for (int y = 0; y < Y_CHUNK_SIZE; y++) {
            for (int z = 0; z < Z_CHUNK_SIZE; z++) {
                if (chunk->voxels[x][y][z] != VoxelType::AIR) {
                    addVoxelGeometry(vertices, indices, x, y, z, chunk);
                }
            }
        }
    }

    Texture tex;
    tex.id = textureId;
    tex.type = "texture_diffuse";

    // Store the mesh for this chunk
    chunkMeshes[{xChunk, zChunk}] = std::make_unique<Mesh>(vertices, indices, std::vector<Texture>{tex});
}



void VoxelRenderer::updateDirtyChunks() {
    for (int xChunk = 0; xChunk < X_CHUNKS; xChunk++) {
        for (int zChunk = 0; zChunk < Z_CHUNKS; zChunk++) {
            auto chunk = grid->chunks[{xChunk, zChunk}];
            if (chunk->isDirty()) {
                logger->log(DEBUG, "Rebuilding chunk: " + std::to_string(xChunk) + ", " + std::to_string(zChunk));
                rebuildChunkMesh(xChunk, zChunk);
                chunk->clearDirty();
            }
        }
    }
}

void VoxelRenderer::draw(const RenderContext &ctx, glm::mat4 modelMatrix) {
    updateDirtyChunks();

    shader->use();
    shader->setMat4("projection", ctx.projection);
    shader->setMat4("view", ctx.view);

    for (int xChunk = 0; xChunk < X_CHUNKS; xChunk++) {
        for (int zChunk = 0; zChunk < Z_CHUNKS; zChunk++) {
            auto it = chunkMeshes.find({xChunk, zChunk});
            if (it == chunkMeshes.end()) continue;

            glm::mat4 chunkMatrix = modelMatrix * glm::translate(glm::mat4(1.0f),
                                                                 glm::vec3(xChunk * X_CHUNK_SIZE, 0.0f, zChunk * Z_CHUNK_SIZE));

            shader->setMat4("model", chunkMatrix);
            it->second->Draw(*shader);
        }
    }
}