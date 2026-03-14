#ifndef QUESTFARERGAMEENGINE_GAMEOBJECT_H
#define QUESTFARERGAMEENGINE_GAMEOBJECT_H

#include "glm/vec3.hpp"
#include "../../rendering/RenderContext.h"
#include "../geometry/Transform.h"
struct GameObject {
    int id;
    Transform transform;
    bool active;

    GameObject(int id, bool active = true)
            : id(id), active(active) {}

    virtual void draw(const RenderContext& ctx) = 0;
    virtual ~GameObject() = default;
};
#endif //QUESTFARERGAMEENGINE_GAMEOBJECT_H
