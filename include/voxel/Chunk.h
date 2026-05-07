#ifndef QUESTFARERGAMEENGINE_CHUNK_H
#define QUESTFARERGAMEENGINE_CHUNK_H

#include <memory>
#include <vector>
#include <algorithm>
#include "../../include/globals.h"

namespace voxel {
    // Explicitly use uint8_t to save 3 bytes per voxel vs default int-enum
    enum class VoxelType : uint8_t { AIR = 0, GRASS, DIRT, STONE };

    class Chunk {
    private:
        std::unique_ptr<VoxelType[]> voxels;
        bool dirty;

    public:
        Chunk() : dirty(true) {
            size_t totalSize = X_CHUNK_SIZE * Y_CHUNK_SIZE * Z_CHUNK_SIZE;
            voxels = std::make_unique<VoxelType[]>(totalSize);
            // Default to AIR
            std::fill(voxels.get(), voxels.get() + totalSize, VoxelType::AIR);
        }

        // Y-axis as most significant bit is better for vertical column access
        // and standard greedy meshing iterations.
        inline int GetIndex(int x, int y, int z) const {
            return (y * X_CHUNK_SIZE * Z_CHUNK_SIZE) + (z * X_CHUNK_SIZE) + x;
        }

        void SetVoxel(int x, int y, int z, VoxelType type) {
            // Safety check for bounds
            if (x & ~15 || y < 0 || y >= Y_CHUNK_SIZE || z & ~15) return;

            int idx = GetIndex(x, y, z);
            if (voxels[idx] != type) {
                voxels[idx] = type;
                dirty = true;
            }
        }

        VoxelType GetVoxel(int x, int y, int z) const {
            if (x & ~15 || y < 0 || y >= Y_CHUNK_SIZE || z & ~15) return VoxelType::AIR;
            return voxels[GetIndex(x, y, z)];
        }

        bool isDirty() const { return dirty; }
        void clearDirty() { dirty = false; }
        void markDirty() { dirty = true; }

        // Raw access for the renderer's greedy mesher
        VoxelType* getData() { return voxels.get(); }
    };
}
#endif