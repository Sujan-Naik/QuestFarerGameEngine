#ifndef QUESTFARERGAMEENGINE_TRANSFORM_H
#define QUESTFARERGAMEENGINE_TRANSFORM_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

struct Transform{
    glm::quat rotation;
    glm::vec3 position;
    glm::vec3 scale;

    glm::mat4 matrix() const {
        return glm::translate(glm::mat4(1.0f), position)
               * glm::mat4_cast(rotation)
               * glm::scale(glm::mat4(1.0f), scale);
    }
};

#endif //QUESTFARERGAMEENGINE_TRANSFORM_H
