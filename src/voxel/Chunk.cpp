// Chunk.cpp
#include <cstring>
#include "../../include/voxel/Chunk.h"
#include "../../include/globals.h"

Chunk::Chunk() {
    memset(voxels, 0, sizeof(voxels));
    dirty = true;

    for (int x = 0; x < X_CHUNK_SIZE; x++) {
        for (int y = 0; y < Y_CHUNK_SIZE; y++) {
            for (int z = 0; z < Z_CHUNK_SIZE; z++) {
                if (y == 0){
                    voxels[x][y][z] = VoxelType::DIRT;
                }
            }
        }
    }
}

void Chunk::setVoxel(int x, int y, int z, VoxelType type) {
    if (x < 0 || x >= X_CHUNK_SIZE || y < 0 || y >= Y_CHUNK_SIZE || z < 0 || z >= Z_CHUNK_SIZE)
        return; // Out of bounds

    if (voxels[x][y][z] != type) {
        voxels[x][y][z] = type;
        dirty = true;
    }
}


VoxelType Chunk::getVoxel(int x, int y, int z) const {
    if (x < 0 || x >= X_CHUNK_SIZE || y < 0 || y >= Y_CHUNK_SIZE || z < 0 || z >= Z_CHUNK_SIZE)
        return VoxelType::AIR;
    return voxels[x][y][z];
}

bool Chunk::isSolid(int x, int y, int z) const {
    return getVoxel(x, y, z) != VoxelType::AIR;
}

void Chunk::markDirty() {
    dirty = true;
}

bool Chunk::isDirty() const {
    return dirty;
}

void Chunk::clearDirty() {
    dirty = false;
}