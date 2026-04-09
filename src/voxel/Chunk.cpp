#include <cstring>
#include "../../include/voxel/Chunk.h"
#include "../../include/globals.h"


Chunk::Chunk() {
    memset(voxels, 0, sizeof(voxels));
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