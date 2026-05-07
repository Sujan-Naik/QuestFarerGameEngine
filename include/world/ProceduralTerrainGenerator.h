#ifndef QUESTFARERGAMEENGINE_PROCEDURAL_TERRAIN_GENERATOR_H
#define QUESTFARERGAMEENGINE_PROCEDURAL_TERRAIN_GENERATOR_H

#include <memory>
#include <random>
#include "../voxel/Grid.h"
#include "../include/globals.h"
#include "FastNoiseLite/FastNoiseLite.h"

namespace world {
    class ProceduralTerrainGenerator {
    private:
        std::unique_ptr<FastNoiseLite> noise;
        float heightScale = 128.0f;
        float baseHeight = 20.0f;

    public:
        ProceduralTerrainGenerator() {
            noise = std::make_unique<FastNoiseLite>();

            std::random_device rd;
            noise->SetSeed(static_cast<int>(rd()));

            noise->SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
            noise->SetFrequency(0.002f);

            noise->SetFractalType(FastNoiseLite::FractalType_FBm);
            noise->SetFractalOctaves(5);
            noise->SetFractalLacunarity(2.0f);
            noise->SetFractalGain(0.5f);
        }

        void generateChunk(std::shared_ptr<voxel::Grid> grid, int xChunk, int zChunk) {
            for (int x = 0; x < 16; ++x) {
                for (int z = 0; z < 16; ++z) {
                    float worldX = static_cast<float>((xChunk * 16) + x);
                    float worldZ = static_cast<float>((zChunk * 16) + z);

                    float n = noise->GetNoise(worldX, worldZ);
                    float normalized = (n + 1.0f) * 0.5f;

                    int surfaceHeight = static_cast<int>(baseHeight + (normalized * heightScale));

//                    if (surfaceHeight >= Y_CHUNK_SIZE) surfaceHeight = Y_CHUNK_SIZE - 1;

//                    if (surfaceHeight < 1) surfaceHeight = 1;

                    for (int y = 0; y < surfaceHeight; ++y) {
                        voxel::VoxelType type = voxel::VoxelType::STONE;

                        if (y >= surfaceHeight - 1) {
                            type = voxel::VoxelType::GRASS;
                        } else if (y >= surfaceHeight - 4) {
                            type = voxel::VoxelType::DIRT;
                        }

                        grid->SetVoxel(static_cast<int>(worldX), y, static_cast<int>(worldZ), type);
                    }
                }
            }
        }
    };
}

#endif