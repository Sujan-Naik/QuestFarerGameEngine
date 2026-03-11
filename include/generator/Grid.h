#ifndef QUESTFARERGAMEENGINE_GRID_H
#define QUESTFARERGAMEENGINE_GRID_H


#include "glm/vec3.hpp"

/**
 * @struct GridPoint
 * A structure used to represent the position of a point on the grid can change with time,
 * and specific terrain points will have their own velocity for dynamic movement.
 */
struct GridPoint{
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 basePosition;

    GridPoint(): position(0), velocity(0), basePosition(0) {}
    GridPoint(const glm::vec3& pos ): position(pos), velocity(0), basePosition(pos){}

};

/**
 * @struct Grid
 * A 2D Height Map of the grid the terrain is divided into
 */
struct Grid {

    float heightLowerBound;
    float heightUpperBound;

    std::vector<std::vector<GridPoint>> heightmap;
};


#endif //QUESTFARERGAMEENGINE_GRID_H
