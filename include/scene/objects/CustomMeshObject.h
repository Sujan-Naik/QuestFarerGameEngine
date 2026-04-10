#ifndef QUESTFARERGAMEENGINE_CUSTOMMESHOBJECT_H
#define QUESTFARERGAMEENGINE_CUSTOMMESHOBJECT_H

#include "../../rendering/MeshRenderer.h"
#include "../../rendering/RenderContext.h"
#include "GameObject.h"
using namespace rendering;

struct CustomMeshObject : GameObject {
    std::unique_ptr<MeshRenderer> renderer;

    explicit CustomMeshObject(int id, std::unique_ptr<MeshRenderer> renderer, Transform transform, bool active = true)
            : GameObject(id, transform, active), renderer(std::move(renderer)) {}



    void drawCustom(const RenderContext& ctx, glm::mat4 modelMatrix) {
        renderer->draw(ctx, modelMatrix);
    }

    void draw(const RenderContext& ctx) override {
        drawCustom(ctx, getModelMatrix());
    }
};
#endif //QUESTFARERGAMEENGINE_CUSTOMMESHOBJECT_H
