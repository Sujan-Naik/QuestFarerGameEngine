#ifndef QUESTFARERGAMEENGINE_GLOBALS_H
#define QUESTFARERGAMEENGINE_GLOBALS_H

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

#endif //QUESTFARERGAMEENGINE_GLOBALS_H