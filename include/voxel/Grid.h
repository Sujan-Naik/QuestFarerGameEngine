#ifndef QUESTFARERGAMEENGINE_GRID_H
#define QUESTFARERGAMEENGINE_GRID_H

#include <unordered_map>
#include <memory>
#include <glm/glm.hpp>
#include "Chunk.h"
#include "../../include/globals.h"

namespace voxel {

    struct Vector2IHash {
        // Optimized hash for grid coordinates using bit-packing
        std::size_t operator()(const glm::ivec2& v) const {
            return std::hash<long long>{}((static_cast<long long>(v.x) << 32) | (static_cast<unsigned int>(v.y)));
        }
    };

    class Grid {
    public:
        // Unique ownership. Map is still great for "infinite" sparse grids.
        std::unordered_map<glm::ivec2, std::unique_ptr<Chunk>, Vector2IHash> chunks;

        Grid() = default;
        virtual ~Grid() = default;

        // Automatically creates a chunk if it doesn't exist
        void SetVoxel(int worldX, int worldY, int worldZ, VoxelType type) {
            int chunkX = worldX >> 4; // Fast divide by 16
            int chunkZ = worldZ >> 4;

            glm::ivec2 pos(chunkX, chunkZ);
            auto it = chunks.find(pos);

            if (it == chunks.end()) {
                chunks[pos] = std::make_unique<Chunk>();
                it = chunks.find(pos);
            }

            int localX = worldX & 15; // Fast modulo 16
            int localZ = worldZ & 15;

            it->second->SetVoxel(localX, worldY, localZ, type);
        }

        VoxelType GetVoxel(int worldX, int worldY, int worldZ) const {
            // Early exit for height limits
            if (worldY < 0 || worldY >= Y_CHUNK_SIZE) return VoxelType::AIR;

            int chunkX = worldX >> 4;
            int chunkZ = worldZ >> 4;

            auto it = chunks.find({chunkX, chunkZ});
            if (it != chunks.end()) {
                return it->second->GetVoxel(worldX & 15, worldY, worldZ & 15);
            }

            return VoxelType::AIR;
        }

        bool IsSolid(int worldX, int worldY, int worldZ) const {
            return GetVoxel(worldX, worldY, worldZ) != VoxelType::AIR;
        }

        // Used by the renderer to clean up or prioritize meshing
        void UnloadFarChunks(const glm::vec3& playerPos, float distance) {
            float distSq = distance * distance;
            for (auto it = chunks.begin(); it != chunks.end(); ) {
                float dx = (it->first.x * 16) - playerPos.x;
                float dz = (it->first.y * 16) - playerPos.z;
                if ((dx * dx + dz * dz) > distSq) {
                    it = chunks.erase(it);
                } else {
                    ++it;
                }
            }
        }
    };
}

#endif