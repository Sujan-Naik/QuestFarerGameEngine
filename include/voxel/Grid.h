#ifndef QUESTFARERGAMEENGINE_GRID_H
#define QUESTFARERGAMEENGINE_GRID_H

#include <memory>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <glm/glm.hpp>
#include <iostream>
#include "Chunk.h"

namespace voxel {

    struct Vector2IHash {
        std::size_t operator()(const glm::ivec2& k) const {
            return (k.x * 521 + k.y);
        }
    };

    using ChunkHash = Vector2IHash;

    class Grid {
    public:
        std::unordered_map<glm::ivec2, std::shared_ptr<Chunk>, ChunkHash> chunks;

        Grid() = default;

        std::shared_ptr<Chunk> GetChunk(int cx, int cz) {
            auto it = chunks.find(glm::ivec2(cx, cz));
            if (it != chunks.end()) {
                return it->second;
            }
            return nullptr;
        }

        void SetVoxel(int x, int y, int z, VoxelType type) {
            if (y < 0 || y >= Y_CHUNK_SIZE) return;

            int cx = (x >= 0) ? x / 16 : (x - 15) / 16;
            int cz = (z >= 0) ? z / 16 : (z - 15) / 16;

            auto it = chunks.find(glm::ivec2(cx, cz));
            if (it != chunks.end()) {
                int lx = ((x % 16) + 16) % 16;
                int lz = ((z % 16) + 16) % 16;
                it->second->SetVoxel(lx, y, lz, type);
            }
        }

        bool IsSolid(int x, int y, int z) {
            if (y < 0 || y >= Y_CHUNK_SIZE) return false;

            int cx = (x >= 0) ? x / 16 : (x - 15) / 16;
            int cz = (z >= 0) ? z / 16 : (z - 15) / 16;

            auto it = chunks.find(glm::ivec2(cx, cz));
            if (it == chunks.end()) return false;

            int lx = ((x % 16) + 16) % 16;
            int lz = ((z % 16) + 16) % 16;

            return it->second->GetVoxel(lx, y, lz) != VoxelType::AIR;
        }

        void UpdateEntitySpatialPosition(int entityId, glm::vec3 oldPos, glm::vec3 newPos) {
            glm::ivec2 oldChunkCoord = glm::ivec2(static_cast<int>(std::floor(oldPos.x)) >> 4, static_cast<int>(std::floor(oldPos.z)) >> 4);
            glm::ivec2 newChunkCoord = glm::ivec2(static_cast<int>(std::floor(newPos.x)) >> 4, static_cast<int>(std::floor(newPos.z)) >> 4);

            if (oldChunkCoord != newChunkCoord) {
                RemoveEntityFromChunk(entityId, oldChunkCoord);
                AddEntityToChunk(entityId, newChunkCoord);
            }
        }

        void AddEntityToChunk(int entityId, glm::ivec2 chunkCoord) {
            std::cout << "[Grid] Attempting to add entity ID: " << entityId << " to chunk (" << chunkCoord.x << ", " << chunkCoord.y << ")\n";

            auto it = chunks.find(chunkCoord);
            if (it != chunks.end()) {
                auto& entities = it->second->occupyingEntities;
                if (std::find(entities.begin(), entities.end(), entityId) == entities.end()) {
                    entities.push_back(entityId);
                    std::cout << "[Grid] Successfully added entity ID: " << entityId << " to chunk (" << chunkCoord.x << ", " << chunkCoord.y << "). Total entities: " << entities.size() << "\n";
                } else {
                    std::cout << "[Grid] Warning: Entity ID: " << entityId << " already present in chunk (" << chunkCoord.x << ", " << chunkCoord.y << ")\n";
                }
            } else {
                std::cout << "[Grid] Error: Failed to add entity ID: " << entityId << " - Chunk (" << chunkCoord.x << ", " << chunkCoord.y << ") does not exist!\n";
            }
        }

        void RemoveEntityFromChunk(int entityId, glm::ivec2 chunkCoord) {
            auto it = chunks.find(chunkCoord);
            if (it != chunks.end()) {
                auto& entities = it->second->occupyingEntities;
                entities.erase(std::remove(entities.begin(), entities.end(), entityId), entities.end());
            }
        }

        std::vector<int> GetEntitiesNear(glm::vec3 position, float chunkRadius) {
            std::vector<int> foundEntities;
            glm::ivec2 centerChunk = glm::ivec2(static_cast<int>(std::floor(position.x)) >> 4, static_cast<int>(std::floor(position.z)) >> 4);
            int radius = static_cast<int>(std::ceil(chunkRadius));

            for (int cx = -radius; cx <= radius; ++cx) {
                for (int cz = -radius; cz <= radius; ++cz) {
                    auto it = chunks.find(glm::ivec2(centerChunk.x + cx, centerChunk.y + cz));
                    if (it != chunks.end()) {
                        foundEntities.insert(foundEntities.end(), it->second->occupyingEntities.begin(), it->second->occupyingEntities.end());
                    }
                }
            }
            return foundEntities;
        }
    };
}
#endif