#ifndef QUESTFARERGAMEENGINE_CUSTOMMESHOBJECT_H
#define QUESTFARERGAMEENGINE_CUSTOMMESHOBJECT_H

#include "../../rendering/MeshRenderer.h"
#include "../../rendering/RenderContext.h"
#include "GameObject.h"

struct CustomMeshObject : GameObject {
    std::unique_ptr<MeshRenderer> renderer;

    explicit CustomMeshObject(int id, std::unique_ptr<MeshRenderer> renderer, bool active = true)
            : GameObject(id, active), renderer(std::move(renderer)) {}

    void draw(const RenderContext& ctx) override {
        renderer->draw(ctx);
    }
};
#endif //QUESTFARERGAMEENGINE_CUSTOMMESHOBJECT_H
