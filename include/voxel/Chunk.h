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

    Chunk();


    VoxelType voxels[X_CHUNK_SIZE][Y_CHUNK_SIZE][Z_CHUNK_SIZE]{};

    bool dirty = true;
};


#endif //QUESTFARERGAMEENGINE_CHUNK_H
