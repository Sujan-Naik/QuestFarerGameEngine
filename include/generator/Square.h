#ifndef QUESTFARERGAMEENGINE_SQUARE_H
#define QUESTFARERGAMEENGINE_SQUARE_H

#include "glm/vec3.hpp"
#include "glm/fwd.hpp"
#include "Grid.h"
#include "glm/detail/type_vec2.hpp"

/**
 * @struct Represents a square for midpoint displacement
 * Lower Z is bottom, higher Z is top
 * Lower X is left, higher X is right
 */
struct Square{
    glm::ivec2 bottomLeft;
    glm::ivec2 topLeft;
    glm::ivec2 bottomRight;
    glm::ivec2 topRight;

    [[nodiscard]] glm::vec3 getBottomLeft(const Grid& grid) const;
    [[nodiscard]] glm::vec3 getTopLeft(const Grid& grid) const;
    [[nodiscard]] glm::vec3 getBottomRight(const Grid& grid) const;
    [[nodiscard]] glm::vec3 getTopRight(const Grid& grid) const;



};

#endif //QUESTFARERGAMEENGINE_SQUARE_H
