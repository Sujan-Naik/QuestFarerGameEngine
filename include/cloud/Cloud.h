#ifndef QUESTFARERGAMEENGINE_CLOUD_H
#define QUESTFARERGAMEENGINE_CLOUD_H

#include "glm/vec3.hpp"

/**
 * @struct Cloud
 * @brief Represents a singular cloud particle
 *
 * A Cloud is represented visually as a cube using a transparent circular texture to
 * give a spherical impression.
 */
struct Cloud{
    glm::vec3 position;
    glm::vec3 velocity;
    float size;
};
#endif //QUESTFARERGAMEENGINE_CLOUD_H
