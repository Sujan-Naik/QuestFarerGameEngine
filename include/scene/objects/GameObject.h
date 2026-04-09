#ifndef QUESTFARERGAMEENGINE_GAMEOBJECT_H
#define QUESTFARERGAMEENGINE_GAMEOBJECT_H

#include "glm/vec3.hpp"
#include "../../rendering/RenderContext.h"
#include "../geometry/Transform.h"
struct GameObject {
    int id;
    Transform transform;
    bool active;

    GameObject(int id,  Transform transform, bool active = true)
            : id(id), transform(transform), active(active) {}

    virtual void draw(const RenderContext& ctx) = 0;
    virtual ~GameObject() = default;

    void setPosition(const glm::vec3& pos) {
        transform.position = pos;
    }

    void setRotation(const glm::quat& rot) {
        transform.rotation = rot;
    }

    [[nodiscard]] glm::vec3 getPosition() const {
        return transform.position;
    }

    [[nodiscard]] glm::quat getRotation() const {
        return transform.rotation;
    }

    [[nodiscard]] glm::mat4 getModelMatrix() const{
        return transform.matrix();
    }
};
#endif //QUESTFARERGAMEENGINE_GAMEOBJECT_H
