#include "../../include/voxel/Grid.h"
#include "../../include/globals.h"

Grid::Grid(){

    for (int xChunk = 0; xChunk < X_CHUNKS ; xChunk++ ){
        for (int zChunk = 0; zChunk < Z_CHUNKS ; zChunk++ ) {
            chunks[{xChunk, zChunk}] = std::make_shared<Chunk>();
        }
    }
}