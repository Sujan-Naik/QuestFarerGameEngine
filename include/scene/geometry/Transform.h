#ifndef QUESTFARERGAMEENGINE_TRANSFORM_H
#define QUESTFARERGAMEENGINE_TRANSFORM_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

struct Transform {
    // w=1, x=0, y=0, z=0 (Identity rotation)
    glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

    // (0,0,0)
    glm::vec3 position = glm::vec3(0.0f);

    // (1,1,1) - Crucial, as a 0 scale makes the object invisible
    glm::vec3 scale = glm::vec3(1.0f);

    glm::mat4 matrix() const {
        return glm::translate(glm::mat4(1.0f), position)
               * glm::mat4_cast(rotation)
               * glm::scale(glm::mat4(1.0f), scale);
    }
};


#endif //QUESTFARERGAMEENGINE_TRANSFORM_H
