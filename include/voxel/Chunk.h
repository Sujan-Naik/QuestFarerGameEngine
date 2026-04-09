// Chunk.h
#ifndef QUESTFARERGAMEENGINE_CHUNK_H
#define QUESTFARERGAMEENGINE_CHUNK_H

#include <unordered_map>
#include <memory>
#include "glm/fwd.hpp"
#include "VoxelType.h"
#include "../globals.h"

class Chunk {

private:

public:
    VoxelType voxels[X_CHUNK_SIZE][Y_CHUNK_SIZE][Z_CHUNK_SIZE]{};
    bool dirty = true;

    Chunk();

    // Set a voxel and mark chunk dirty
    void setVoxel(int x, int y, int z, VoxelType type);

    // Get a voxel, returns AIR if out of bounds
    VoxelType getVoxel(int x, int y, int z) const;


    // Check if voxel is solid (not air)
    bool isSolid(int x, int y, int z) const;

    // Mark chunk as needing rebuild
    void markDirty();

    // Check if chunk needs rebuild
    bool isDirty() const ;

    // Clear dirty flag (call after mesh rebuild)
    void clearDirty();

};

#endif //QUESTFARERGAMEENGINE_CHUNK_H