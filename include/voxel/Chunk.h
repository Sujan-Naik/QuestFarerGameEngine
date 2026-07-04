#ifndef QUESTFARERGAMEENGINE_CHUNK_H
#define QUESTFARERGAMEENGINE_CHUNK_H

#include <memory>
#include <vector>
#include <algorithm>
#include "../../include/globals.h"

namespace voxel {
    enum class VoxelType : uint8_t { AIR = 0, GRASS, DIRT, STONE };

    class Chunk {
    private:
        std::unique_ptr<VoxelType[]> voxels;
        bool dirty;

    public:
        std::vector<int> occupyingEntities;

        Chunk() : dirty(true) {
            size_t totalSize = X_CHUNK_SIZE * Y_CHUNK_SIZE * Z_CHUNK_SIZE;
            voxels = std::make_unique<VoxelType[]>(totalSize);
            std::fill(voxels.get(), voxels.get() + totalSize, VoxelType::AIR);
        }

        inline int GetIndex(int x, int y, int z) const {
            return (y * X_CHUNK_SIZE * Z_CHUNK_SIZE) + (z * X_CHUNK_SIZE) + x;
        }

        void SetVoxel(int x, int y, int z, VoxelType type) {
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

        VoxelType* getData() { return voxels.get(); }
    };
}
#endif