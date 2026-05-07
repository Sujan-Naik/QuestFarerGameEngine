#ifndef TERRAIN_H
#define TERRAIN_H

#include <memory>
#include <unordered_map>
#include <vector>
#include <bitset>
#include <functional>
#include <stdexcept>
#include <cmath>
#include "Vector2I.h"
#include "Vector3F.h"
#include "Vector2F.h"
#include "VoxelGrid.h"
#include "FastNoiseLite.h"
#include "FeatureGenerator.h"
#include "SettlementGenerator.h"
#include "PoissonDS.h"
#include "Globals.h"
#include "IManager.h"
#include "VoxelRandom.h"

namespace VoxelLib {

class Terrain
{
private:
    struct Vector2IHash
    {
        std::size_t operator()(const Vector2I& v) const
        {
            auto h1 = std::hash<int>{}(v.x);
            auto h2 = std::hash<int>{}(v.z);
            return h1 ^ (h2 << 1);
        }
    };

    struct Vector2IEqual
    {
        bool operator()(const Vector2I& a, const Vector2I& b) const
        {
            return a.x == b.x && a.z == b.z;
        }
    };

    using ChunkGrid = std::unordered_map<Vector2I, std::shared_ptr<VoxelGrid>, Vector2IHash, Vector2IEqual>;

    std::shared_ptr<FeatureGenerator> featureGenerator;
    ChunkGrid chunkGrids;
    std::shared_ptr<FastNoiseLite> fastNoiseLite;
    std::shared_ptr<SettlementGenerator> settlementGenerator;

public:
    static std::shared_ptr<Terrain> instance;

    Terrain()
    {
        if (instance != nullptr)
        {
            throw std::runtime_error("Only 1 Terrain instance should exist at any time.");
        }

        featureGenerator = std::make_shared<FeatureGenerator>();
        settlementGenerator = std::make_shared<SettlementGenerator>();
        fastNoiseLite = std::make_shared<FastNoiseLite>();

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dis(0, 9999);
        
        fastNoiseLite->SetSeed(dis(gen));
        fastNoiseLite->SetNoiseType(FastNoiseLite::NoiseType::Perlin);
        fastNoiseLite->SetFractalType(FastNoiseLite::FractalType::FBm);

        instance = std::make_shared<Terrain>(*this);
    }

    virtual ~Terrain() = default;

    std::vector<Line> GetLines(const std::string& name) const
    {
        return featureGenerator->GetLines(name);
    }

    float GetGroundHeight(float x, float z) const
    {
        float noise = (fastNoiseLite->GetNoise(x, z) + 1.0f) / 2.0f * Globals::TERRAIN_HEIGHT_RANGE;
        return noise;
    }

    void SpawnTerrain(const Vector3F& position)
    {
        Vector2I gridCentreChunkCoordinate(
            static_cast<int>(position.x) / Globals::X_CHUNK_SIZE,
            static_cast<int>(position.z) / Globals::Z_CHUNK_SIZE
        );

        std::unordered_set<Vector2I, Vector2IHash, Vector2IEqual> activeChunks;

        for (int gridXChunkCoordinateOffset = -Globals::CHUNK_RADIUS;
             gridXChunkCoordinateOffset <= Globals::CHUNK_RADIUS;
             ++gridXChunkCoordinateOffset)
        {
            int gridXChunkCoordinate = gridCentreChunkCoordinate.x + gridXChunkCoordinateOffset;
            if (gridXChunkCoordinate < 0 || gridXChunkCoordinate >= Globals::MAX_X_CHUNK)
            {
                continue;
            }

            for (int gridZChunkCoordinateOffset = -Globals::CHUNK_RADIUS;
                 gridZChunkCoordinateOffset <= Globals::CHUNK_RADIUS;
                 ++gridZChunkCoordinateOffset)
            {
                int gridZChunkCoordinate = gridCentreChunkCoordinate.z + gridZChunkCoordinateOffset;
                if (gridZChunkCoordinate < 0 || gridZChunkCoordinate >= Globals::MAX_Z_CHUNK)
                {
                    continue;
                }

                Vector2I currentGridPosVec(gridXChunkCoordinate, gridZChunkCoordinate);
                activeChunks.insert(currentGridPosVec);

                if (chunkGrids.find(currentGridPosVec) != chunkGrids.end())
                {
                    continue;
                }

                int chunkXOrigin = gridXChunkCoordinate * Globals::X_CHUNK_SIZE;
                int chunkZOrigin = gridZChunkCoordinate * Globals::Z_CHUNK_SIZE;

                CreateChunkGreedyMeshed(currentGridPosVec, chunkXOrigin, chunkZOrigin);
                SpawnLSystems(chunkXOrigin, chunkZOrigin);
                SpawnSettlements(chunkXOrigin, chunkZOrigin);
            }
        }
    }

    void CreateChunk(const Vector2I& chunk, int xOrigin, int zOrigin)
    {
        auto voxelGrid = std::make_shared<VoxelGrid>(
            Globals::X_CHUNK_SIZE,
            Globals::TERRAIN_HEIGHT_RANGE,
            Globals::Z_CHUNK_SIZE
        );

        std::vector<bool> voxels(voxelGrid->GetRequiredStorageSize());

        for (int x = 0; x < Globals::X_CHUNK_SIZE; ++x)
        {
            for (int z = 0; z < Globals::Z_CHUNK_SIZE; ++z)
            {
                float noise = GetGroundHeight(xOrigin + x, zOrigin + z);
                for (int y = 0; y < Globals::TERRAIN_HEIGHT_RANGE; ++y)
                {
                    voxels[voxelGrid->Flatten(x, y, z)] = (y <= noise);
                }
            }
        }

        voxelGrid->SetVoxels(voxels);
        chunkGrids[chunk] = voxelGrid;

        std::vector<Vector3F> vertices;
        std::vector<Vector2F> uvs;
        std::vector<int> triangles;

        for (int x = 0; x < Globals::X_CHUNK_SIZE; ++x)
        {
            for (int z = 0; z < Globals::Z_CHUNK_SIZE; ++z)
            {
                for (int y = 0; y < Globals::TERRAIN_HEIGHT_RANGE; ++y)
                {
                    if (voxelGrid->GetVoxel(x, y, z))
                    {
                        bool isTopVoxel = !voxelGrid->GetVoxel(x, y + 1, z);
                        bool isBackVoxel = !voxelGrid->GetVoxel(x, y, z - 1);
                        bool isLeftVoxel = !voxelGrid->GetVoxel(x - 1, y, z);
                        bool isFrontVoxel = !voxelGrid->GetVoxel(x, y, z + 1);
                        bool isRightVoxel = !voxelGrid->GetVoxel(x + 1, y, z);
                        bool isBelowVoxel = !voxelGrid->GetVoxel(x, y - 1, z);

                        VoxelMesh mesh(isTopVoxel, isBackVoxel, isLeftVoxel, isFrontVoxel,
                                      isRightVoxel, isBelowVoxel);

                        MergeVoxelMesh(mesh, vertices, uvs, triangles, Vector3F(x, y, z));
                    }
                }
            }
        }

        IManager::GetInstance()->GetAbstractHook()->CreateVoxelChunkMesh(
            chunk, vertices, uvs, triangles);
    }

    void CreateChunkGreedyMeshed(const Vector2I& chunk, int xOrigin, int zOrigin)
    {
        auto voxelGrid = std::make_shared<VoxelGrid>(
            Globals::X_CHUNK_SIZE,
            Globals::TERRAIN_HEIGHT_RANGE,
            Globals::Z_CHUNK_SIZE
        );

        std::vector<bool> voxels(voxelGrid->GetRequiredStorageSize());

        for (int x = 0; x < Globals::X_CHUNK_SIZE; ++x)
        {
            for (int z = 0; z < Globals::Z_CHUNK_SIZE; ++z)
            {
                float noise = GetGroundHeight(xOrigin + x, zOrigin + z);
                for (int y = 0; y < Globals::TERRAIN_HEIGHT_RANGE; ++y)
                {
                    voxels[voxelGrid->Flatten(x, y, z)] = (y <= noise);
                }
            }
        }

        voxelGrid->SetVoxels(voxels);
        chunkGrids[chunk] = voxelGrid;

        std::vector<Vector3F> vertices;
        std::vector<int> triangles;

        GreedyMesh(*voxelGrid, xOrigin, 0, zOrigin, vertices, triangles);

        std::vector<Vector2F> emptyUVs;
        IManager::GetInstance()->GetAbstractHook()->CreateVoxelChunkMesh(
            chunk, vertices, emptyUVs, triangles);
    }

    void GreedyMesh(
        const VoxelGrid& voxelGrid,
        int chunkPosX,
        int chunkPosY,
        int chunkPosZ,
        std::vector<Vector3F>& vertices,
        std::vector<int>& triangles)
    {
        std::array<int, 3> dims = { Globals::X_CHUNK_SIZE, Globals::TERRAIN_HEIGHT_RANGE, Globals::Z_CHUNK_SIZE };

        for (int d = 0; d < 3; ++d)
        {
            int u = (d + 1) % 3;
            int v = (d + 2) % 3;
            std::array<int, 3> x = { 0, 0, 0 };
            std::array<int, 3> q = { 0, 0, 0 };
            std::vector<int> mask(dims[u] * dims[v]);

            q[d] = 1;

            for (x[d] = -1; x[d] < dims[d];)
            {
                int n = 0;
                for (x[v] = 0; x[v] < dims[v]; ++x[v])
                {
                    for (x[u] = 0; x[u] < dims[u]; ++x[u])
                    {
                        bool blockCurrent = x[d] >= 0 && voxelGrid.GetVoxel(x[0], x[1], x[2]);
                        bool blockCompare = x[d] < dims[d] - 1 &&
                                           voxelGrid.GetVoxel(x[0] + q[0], x[1] + q[1], x[2] + q[2]);

                        mask[n++] = (blockCurrent == blockCompare) ? 0 : (blockCurrent ? 1 : -1);
                    }
                }

                x[d]++;
                n = 0;

                for (int j = 0; j < dims[v]; ++j)
                {
                    for (int i = 0; i < dims[u];)
                    {
                        if (mask[n] != 0)
                        {
                            int currentMask = mask[n];
                            int w = 1, h = 1;

                            for (w = 1; i + w < dims[u] && mask[n + w] == currentMask; ++w)
                            {
                            }

                            bool done = false;
                            for (h = 1; j + h < dims[v]; ++h)
                            {
                                for (int k = 0; k < w; ++k)
                                {
                                    if (mask[n + k + h * dims[u]] != currentMask)
                                    {
                                        done = true;
                                        break;
                                    }
                                }
                                if (done) break;
                            }

                            x[u] = i;
                            x[v] = j;
                            std::array<int, 3> du = { 0, 0, 0 };
                            du[u] = w;
                            std::array<int, 3> dv = { 0, 0, 0 };
                            dv[v] = h;

                            Vector3F quadV0(x[0] + du[0], x[1] + du[1], x[2] + du[2]);
                            Vector3F quadV1(x[0] + du[0] + dv[0], x[1] + du[1] + dv[1], x[2] + du[2] + dv[2]);
                            Vector3F quadV2(x[0] + dv[0], x[1] + dv[1], x[2] + dv[2]);
                            Vector3F quadV3(x[0], x[1], x[2]);

                            if (currentMask > 0)
                            {
                                AppendQuad(vertices, triangles, quadV0, quadV1, quadV2, quadV3, false);
                            }
                            else
                            {
                                AppendQuad(vertices, triangles, quadV0, quadV1, quadV2, quadV3, true);
                            }

                            for (int l = 0; l < h; ++l)
                            {
                                for (int k = 0; k < w; ++k)
                                {
                                    mask[n + k + l * dims[u]] = 0;
                                }
                            }

                            i += w;
                            n += w;
                        }
                        else
                        {
                            i++;
                            n++;
                        }
                    }
                }
            }
        }
    }

private:
    void SpawnLSystems(int chunkXOrigin, int chunkZOrigin)
    {
        auto spawnLocations = PoissonDS::DoPoissonDS2D(Globals::X_CHUNK_SIZE, 4.0f);

        for (const auto& spawnLocation : spawnLocations)
        {
            featureGenerator->GenerateLSystemModel(
                Globals::GRASS_NAMES[VoxelRandom::GetInt(Globals::GRASS_NAMES.size())],
                Vector3F(
                    spawnLocation.x + chunkXOrigin,
                    GetGroundHeight(chunkXOrigin + spawnLocation.x, chunkZOrigin + spawnLocation.z),
                    spawnLocation.z + chunkZOrigin
                )
            );
        }

        auto bushSpawnLocations = PoissonDS::DoPoissonDS2D(Globals::X_CHUNK_SIZE, 16.0f);

        for (const auto& spawnLocation : bushSpawnLocations)
        {
            featureGenerator->GenerateLSystemModel(
                Globals::BUSH_NAME,
                Vector3F(
                    spawnLocation.x + chunkXOrigin,
                    GetGroundHeight(chunkXOrigin + spawnLocation.x, chunkZOrigin + spawnLocation.z),
                    spawnLocation.z + chunkZOrigin
                )
            );
            break;
        }
    }

    void SpawnSettlements(int gridChunkX, int gridChunkZ)
    {
        auto groundHeightFunc = [this](float x, float z) { return GetGroundHeight(x, z); };
        settlementGenerator->SpawnSettlement(Vector2I(gridChunkX, gridChunkZ), groundHeightFunc);
    }

    void AppendQuad(
        std::vector<Vector3F>& vertices,
        std::vector<int>& triangles,
        const Vector3F& v0,
        const Vector3F& v1,
        const Vector3F& v2,
        const Vector3F& v3,
        bool isBackFace)
    {
        int baseIndex = vertices.size();
        vertices.push_back(v0); // Top Right
        vertices.push_back(v1); // Bottom Right
        vertices.push_back(v2); // Bottom Left
        vertices.push_back(v3); // Top Left

        if (!isBackFace)
        {
            triangles.push_back(baseIndex + 0);
            triangles.push_back(baseIndex + 1);
            triangles.push_back(baseIndex + 3);
            triangles.push_back(baseIndex + 1);
            triangles.push_back(baseIndex + 2);
            triangles.push_back(baseIndex + 3);
        }
        else
        {
            triangles.push_back(baseIndex + 3);
            triangles.push_back(baseIndex + 1);
            triangles.push_back(baseIndex + 0);
            triangles.push_back(baseIndex + 3);
            triangles.push_back(baseIndex + 2);
            triangles.push_back(baseIndex + 1);
        }
    }

    void MergeVoxelMesh(
        const VoxelMesh& voxel,
        std::vector<Vector3F>& vertices,
        std::vector<Vector2F>& uvs,
        std::vector<int>& triangles,
        const Vector3F& offset)
    {
        int vertexOffset = vertices.size();

        for (const auto& vertex : voxel.newVertices)
        {
            vertices.push_back(vertex + offset);
        }

        for (const auto& uv : voxel.newUV)
        {
            uvs.push_back(uv);
        }

        for (int index : voxel.newTriangles)
        {
            triangles.push_back(index + vertexOffset);
        }
    }
};

}

#endif // TERRAIN_H