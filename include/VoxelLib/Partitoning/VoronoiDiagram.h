#ifndef VORONOI_DIAGRAM_H
#define VORONOI_DIAGRAM_H

#include <unordered_map>
#include <vector>
#include <unordered_set>
#include <algorithm>
#include <random>
#include <memory>
#include "Vector2F.h"
#include "Vector2I.h"

namespace VoxelLib::Partitioning {

/**
 * https://pmc.ncbi.nlm.nih.gov/articles/PMC7840081/
 */
class VoronoiDiagram
{
private:
    // Custom hash for Vector2F to use as unordered_map key
    struct Vector2FHash
    {
        std::size_t operator()(const Vector2F& v) const
        {
            auto h1 = std::hash<float>{}(v.x);
            auto h2 = std::hash<float>{}(v.z);
            return h1 ^ (h2 << 1);
        }
    };

    struct Vector2FEqual
    {
        bool operator()(const Vector2F& a, const Vector2F& b) const
        {
            return a.x == b.x && a.z == b.z;
        }
    };

    using VoronoiMap = std::unordered_map<Vector2F, Vector2F, Vector2FHash, Vector2FEqual>;
    using VoronoiRegions = std::unordered_map<Vector2F, std::vector<Vector2F>, Vector2FHash, Vector2FEqual>;

    VoronoiRegions allVoronoi;
    std::vector<Vector2F> seeds;
    VoronoiMap voronoiGrid;

public:
    int maxX, maxZ;

    VoronoiDiagram(
        int maxX,
        int maxZ,
        const std::vector<Vector2F>& seeds,
        float squareSize)
        : maxX(maxX), maxZ(maxZ), seeds(seeds)
    {
        voronoiGrid = CalculatePoints(0, 0, squareSize);

        for (const auto& [key, value] : voronoiGrid)
        {
            if (allVoronoi.find(value) != allVoronoi.end())
            {
                allVoronoi[value].push_back(key);
            }
            else
            {
                allVoronoi[value] = std::vector<Vector2F>{ key };
            }
        }
    }

    virtual ~VoronoiDiagram() = default;

    std::vector<std::vector<Vector2F>> GetVoronoiByRegion() const
    {
        std::vector<std::vector<Vector2F>> result;
        for (const auto& [key, value] : allVoronoi)
        {
            result.push_back(value);
        }
        return result;
    }

    std::vector<std::unordered_set<Vector2I>> SelectArbitraryGridAlignedVoronoiByAmount(
        int amount,
        float xGridMultiple,
        float zGridMultiple)
    {
        auto voronoi = GetVoronoiByRegion();

        std::vector<std::unordered_set<Vector2I>> gridAlignedVoronoi;
        for (const auto& region : voronoi)
        {
            std::unordered_set<Vector2I> alignedPoints;
            for (const auto& point : region)
            {
                // Check if point is grid-aligned (within epsilon tolerance)
                if (std::fmod(point.x, xGridMultiple) <= 0.01f &&
                    std::fmod(point.z, zGridMultiple) <= 0.01f)
                {
                    alignedPoints.insert(Vector2I(static_cast<int>(point.x), static_cast<int>(point.z)));
                }
            }
            if (!alignedPoints.empty())
            {
                gridAlignedVoronoi.push_back(alignedPoints);
            }
        }

        // Shuffle and take first 'amount' elements
        std::random_device rd;
        std::mt19937 gen(rd());
        std::shuffle(gridAlignedVoronoi.begin(), gridAlignedVoronoi.end(), gen);

        if (gridAlignedVoronoi.size() > static_cast<size_t>(amount))
        {
            gridAlignedVoronoi.resize(amount);
        }

        return gridAlignedVoronoi;
    }

private:
    VoronoiMap CalculatePoints(float xStart, float zStart, float squareSize)
    {
        VoronoiMap cornerPoints;

        for (float xCorner = xStart; xCorner < maxX; xCorner += squareSize)
        {
            for (float zCorner = zStart; zCorner < maxZ; zCorner += squareSize)
            {
                Vector2F corner(xCorner, zCorner);
                Vector2F closest = seeds[0];
                float closestDistance = Vector2F::DistanceSquared(corner, closest);

                for (const auto& seed : seeds)
                {
                    float distance = Vector2F::DistanceSquared(seed, corner);
                    if (distance < closestDistance)
                    {
                        closest = seed;
                        closestDistance = distance;
                    }
                }

                cornerPoints[corner] = closest;
            }
        }

        return cornerPoints;
        // return Subdivide(cornerPoints, xStart, zStart, squareSize);
    }

    VoronoiMap MergeDictionary(
        const VoronoiMap& baseDict,
        const VoronoiMap& overridingDict)
    {
        VoronoiMap result = baseDict;
        for (const auto& [key, value] : overridingDict)
        {
            result[key] = value;
        }
        return result;
    }

    VoronoiMap Subdivide(
        const VoronoiMap& gridDict,
        float xStart,
        float zStart,
        float squareSize)
    {
        auto currentDictionary = gridDict;
        if (squareSize <= 8.0f)
        {
            return currentDictionary;
        }

        for (float xCorner = xStart; xCorner < maxX - squareSize; xCorner += squareSize)
        {
            for (float zCorner = zStart; zCorner < maxZ - squareSize; zCorner += squareSize)
            {
                Vector2F corner(xCorner, zCorner); // Bottom Left
                Vector2F corner1(xCorner + squareSize, zCorner); // Bottom Right
                Vector2F corner2(xCorner, zCorner + squareSize); // Top Left
                Vector2F corner3(xCorner + squareSize, zCorner + squareSize); // Top Right

                auto it0 = gridDict.find(corner);
                auto it1 = gridDict.find(corner1);
                auto it2 = gridDict.find(corner2);
                auto it3 = gridDict.find(corner3);

                if (it0 != gridDict.end() && it1 != gridDict.end() &&
                    it2 != gridDict.end() && it3 != gridDict.end())
                {
                    if (!(it0->second == it1->second) ||
                        !(it0->second == it2->second) ||
                        !(it0->second == it3->second))
                    {
                        currentDictionary = MergeDictionary(currentDictionary,
                            CalculatePoints(corner.x, corner.z, squareSize / 2.0f));
                    }
                }
            }
        }

        return currentDictionary;
    }
};

}

#endif // VORONOI_DIAGRAM_H