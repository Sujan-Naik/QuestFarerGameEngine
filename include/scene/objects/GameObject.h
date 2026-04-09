#ifndef QUESTFARERGAMEENGINE_GAMEOBJECT_H
#define QUESTFARERGAMEENGINE_GAMEOBJECT_H

#include "glm/vec3.hpp"
#include "../../rendering/RenderContext.h"
#include "../geometry/Transform.h"
struct GameObject {
    int id;
    Transform transform;
    bool active;
    glm::mat4 modelMatrix;

    GameObject(int id, glm::mat4 modelMatrix, bool active = true)
            : id(id), modelMatrix(modelMatrix), active(active) {}

    virtual void draw(const RenderContext& ctx) = 0;
    virtual ~GameObject() = default;

    void setPosition(const glm::vec3& pos) {
        modelMatrix = glm::translate(glm::mat4(1.0f), pos);
    }
};
#endif //QUESTFARERGAMEENGINE_GAMEOBJECT_H
