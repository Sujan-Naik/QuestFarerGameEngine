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
        float heightScale = 14.0f;
        float baseHeight = 1.0f;

    public:
        ProceduralTerrainGenerator() {
            noise = std::make_unique<FastNoiseLite>();
            std::random_device rd;
            noise->SetSeed(static_cast<int>(rd()));
            noise->SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
            noise->SetFrequency(0.015f); // Increased frequency for better detail
            noise->SetFractalType(FastNoiseLite::FractalType_FBm);
            noise->SetFractalOctaves(3);
        }

        void generateChunkData(voxel::Chunk* chunk, int xChunk, int zChunk) {
            float heightMap[16][16];
            for (int x = 0; x < 16; ++x) {
                for (int z = 0; z < 16; ++z) {
                    float worldX = static_cast<float>((xChunk << 4) + x);
                    float worldZ = static_cast<float>((zChunk << 4) + z);
                    heightMap[x][z] = (noise->GetNoise(worldX, worldZ) + 1.0f) * 0.5f;
                }
            }

            voxel::VoxelType* data = chunk->getData();
            for (int x = 0; x < 16; ++x) {
                for (int z = 0; z < 16; ++z) {
                    int surfaceHeight = static_cast<int>(baseHeight + (heightMap[x][z] * heightScale));
                    for (int y = 0; y < surfaceHeight; ++y) {
                        voxel::VoxelType type = voxel::VoxelType::STONE;
                        if (y >= surfaceHeight - 1) type = voxel::VoxelType::GRASS;
                        else if (y >= surfaceHeight - 4) type = voxel::VoxelType::DIRT;
                        data[(y << 8) | (z << 4) | x] = type;
                    }
                }
            }
        }
    };
}
#endif