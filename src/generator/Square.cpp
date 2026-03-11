#include "../../include/generator/Square.h"

glm::vec3 Square::getBottomLeft(const Grid& grid ) const {
    return grid.heightmap[bottomLeft.x][bottomLeft.y].position;
}

glm::vec3 Square::getTopLeft(const Grid& grid ) const {
    return grid.heightmap[topLeft.x][topLeft.y].position;
}

glm::vec3 Square::getBottomRight(const Grid& grid ) const {
    return grid.heightmap[bottomRight.x][bottomRight.y].position;
}

glm::vec3 Square::getTopRight(const Grid& grid ) const {
    return grid.heightmap[topRight.x][topRight.y].position;
}