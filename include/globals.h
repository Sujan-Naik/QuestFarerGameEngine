#ifndef QUESTFARERGAMEENGINE_GLOBALS_H
#define QUESTFARERGAMEENGINE_GLOBALS_H

#include <glm/glm.hpp>

const double FIXED_TIMESTEP = 1.0 / 60.0;
const glm::vec3 GRAVITY = glm::vec3(0.0f, -9.81f, 0.0f);



// -----------------------------------------------------------------------
// Diamond Square constants
// -----------------------------------------------------------------------
constexpr int SIZE = 8;
constexpr float HEIGHT_LOWER_BOUND = -10, HEIGHT_UPPER_BOUND = 400;
constexpr int SIDE_LENGTH = (1 << SIZE) + 1;
constexpr int HORIZONTAL_SCALE = 10;
constexpr int EFFECTIVE_SIDE_LENGTH = SIDE_LENGTH * HORIZONTAL_SCALE;

// -----------------------------------------------------------------------
// Marching Cubes constants
// -----------------------------------------------------------------------
constexpr int   MC_GRID_X     = 100;      // scalar field resolution per axis
constexpr int   MC_GRID_Y     = 100;
constexpr int   MC_GRID_Z     = 100;
constexpr float MC_NOISE_FREQ = 0.0201f;  // perlin frequency (larger = smaller blobs)
constexpr float MC_WORLD_SCALE = 5.0f;    // uniform scale applied to the final mesh


constexpr int X_CHUNKS = 2, Z_CHUNKS=2;


constexpr int X_CHUNK_SIZE = 61, Y_CHUNK_SIZE=61, Z_CHUNK_SIZE=61;

#endif //QUESTFARERGAMEENGINE_GLOBALS_H