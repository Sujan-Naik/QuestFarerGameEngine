#include "../../include/voxel/Grid.h"
#include "../../include/globals.h"

Grid::Grid(){


    for (int xChunk = 0; xChunk < X_CHUNKS ; xChunk++ ){
        for (int zChunk = 0; zChunk < Z_CHUNKS ; zChunk++ ) {
            chunks[{xChunk, zChunk}] = std::make_shared<Chunk>();
        }
    }
}

// Grid.cpp
void Grid::setVoxel(int worldX, int worldY, int worldZ, VoxelType type) {
    int chunkX = worldX / X_CHUNK_SIZE;
    int chunkZ = worldZ / Z_CHUNK_SIZE;
    int localX = worldX % X_CHUNK_SIZE;
    int localZ = worldZ % Z_CHUNK_SIZE;

    if (chunks.find({chunkX, chunkZ}) != chunks.end()) {
        chunks[{chunkX, chunkZ}]->setVoxel(localX, worldY, localZ, type);

        // Mark neighbors dirty if on boundary
        if (localX == 0 && chunks.find({chunkX - 1, chunkZ}) != chunks.end())
            chunks[{chunkX - 1, chunkZ}]->dirty = true;
        if (localX == X_CHUNK_SIZE - 1 && chunks.find({chunkX + 1, chunkZ}) != chunks.end())
            chunks[{chunkX + 1, chunkZ}]->dirty = true;
        if (localZ == 0 && chunks.find({chunkX, chunkZ - 1}) != chunks.end())
            chunks[{chunkX, chunkZ - 1}]->dirty = true;
        if (localZ == Z_CHUNK_SIZE - 1 && chunks.find({chunkX, chunkZ + 1}) != chunks.end())
            chunks[{chunkX, chunkZ + 1}]->dirty = true;
    }
}

bool Grid::isSolid(int worldX, int worldY, int worldZ) const {

    int chunkX = std::floor(static_cast<float>(worldX) / X_CHUNK_SIZE);
    int chunkZ = std::floor(static_cast<float>(worldZ) / Z_CHUNK_SIZE);

    // 2. Correct Local Index (Handles negative wrapping)
    int localX = ((worldX % X_CHUNK_SIZE) + X_CHUNK_SIZE) % X_CHUNK_SIZE;
    int localZ = ((worldZ % Z_CHUNK_SIZE) + Z_CHUNK_SIZE) % Z_CHUNK_SIZE;

    // 3. Find the chunk
    auto it = chunks.find({chunkX, chunkZ});
    if (it != chunks.end()) {
        return it->second->isSolid(localX, worldY, localZ);
    }

    return false; // Not in a loaded chunk
}