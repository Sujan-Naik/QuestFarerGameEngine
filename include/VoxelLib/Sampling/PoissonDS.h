#ifndef POISSON_DS_H
#define POISSON_DS_H

#include <vector>
#include <cmath>
#include <random>
#include "Vector2F.h"

namespace VoxelLib::Sampling {

class PoissonDS
{
private:
    static constexpr int POISSON_NEARBY_THRESHOLD_RADIUS = 4;

public:
    /**
     * https://www.cs.ubc.ca/~rbridson/docs/bridson-siggraph07-poissondisk.pdf
     */
    static std::vector<Vector2F> DoPoissonDS2D(
        float extent,
        float minimumSampleDistance,
        int sampleLimit = 1)
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dis(0.0f, 1.0f);

        float gridCellSize = minimumSampleDistance / std::sqrt(2.0f);
        int gridWidth = static_cast<int>(std::ceil(extent / gridCellSize));
        int gridHeight = static_cast<int>(std::ceil(extent / gridCellSize));

        // 2D grid initialized to -1
        std::vector<std::vector<int>> grid(gridWidth, std::vector<int>(gridHeight, -1));

        std::vector<Vector2F> samples;
        std::vector<int> activeList;

        Vector2F x0(dis(gen) * extent, dis(gen) * extent);
        samples.push_back(x0);
        activeList.push_back(0);
        grid[static_cast<int>(x0.x / gridCellSize)][static_cast<int>(x0.z / gridCellSize)] = 0;

        std::uniform_int_distribution<int> randIndex(0, 0);

        while (!activeList.empty())
        {
            randIndex = std::uniform_int_distribution<int>(0, activeList.size() - 1);
            int randIdx = randIndex(gen);
            int sampleIndex = activeList[randIdx];
            Vector2F randX = samples[sampleIndex];

            bool foundValid = false;

            for (int i = 0; i < sampleLimit; ++i)
            {
                float randomAngle = dis(gen) * 2.0f * static_cast<float>(M_PI);
                float randomRadius = minimumSampleDistance + dis(gen) * minimumSampleDistance;
                Vector2F randomPoint(
                    randX.x + randomRadius * std::cos(randomAngle),
                    randX.z + randomRadius * std::sin(randomAngle)
                );

                bool randomPointDistanceIsSufficient = true;

                int gridX = static_cast<int>(randomPoint.x / gridCellSize);
                int gridZ = static_cast<int>(randomPoint.z / gridCellSize);

                // Check nearby grid cells
                for (int deltaX = -POISSON_NEARBY_THRESHOLD_RADIUS;
                     deltaX <= POISSON_NEARBY_THRESHOLD_RADIUS;
                     ++deltaX)
                {
                    for (int deltaZ = -POISSON_NEARBY_THRESHOLD_RADIUS;
                         deltaZ <= POISSON_NEARBY_THRESHOLD_RADIUS;
                         ++deltaZ)
                    {
                        int gridXPositionToCheck = gridX + deltaX;
                        int gridZPositionToCheck = gridZ + deltaZ;

                        if (gridXPositionToCheck >= 0 && gridXPositionToCheck < gridWidth &&
                            gridZPositionToCheck >= 0 && gridZPositionToCheck < gridHeight)
                        {
                            int sampleToCheckID = grid[gridXPositionToCheck][gridZPositionToCheck];
                            if (sampleToCheckID >= 0)
                            {
                                if (Vector2F::Distance(randomPoint, samples[sampleToCheckID]) 
                                    minimumSampleDistance)
                                {
                                    randomPointDistanceIsSufficient = false;
                                    break;
                                }
                            }
                        }
                    }

                    if (!randomPointDistanceIsSufficient)
                    {
                        break;
                    }
                }

                if (randomPointDistanceIsSufficient)
                {
                    if (gridX >= 0 && gridX < gridWidth &&
                        gridZ >= 0 && gridZ < gridHeight)
                    {
                        samples.push_back(randomPoint);
                        grid[gridX][gridZ] = samples.size() - 1;
                        activeList.push_back(samples.size() - 1);
                        foundValid = true;
                    }

                    break;
                }
            }

            if (!foundValid)
            {
                activeList.erase(activeList.begin() + randIdx);
            }
        }

        return samples;
    }
};

}

#endif // POISSON_DS_H