#ifndef QUESTFARERGAMEENGINE_CUSTOMMESHOBJECT_H
#define QUESTFARERGAMEENGINE_CUSTOMMESHOBJECT_H

#include "../../rendering/MeshRenderer.h"
#include "../../rendering/RenderContext.h"
#include "GameObject.h"

struct CustomMeshObject : GameObject {
    std::unique_ptr<MeshRenderer> renderer;

    explicit CustomMeshObject(int id, std::unique_ptr<MeshRenderer> renderer, glm::mat4 modelMatrix, bool active = true)
            : GameObject(id, modelMatrix, active), renderer(std::move(renderer)) {}



    void drawCustom(const RenderContext& ctx, glm::mat4 modelMatrix) {
        renderer->draw(ctx, modelMatrix);
    }

    void draw(const RenderContext& ctx) override {
        drawCustom(ctx, modelMatrix);
    }
};
#endif //QUESTFARERGAMEENGINE_CUSTOMMESHOBJECT_H
