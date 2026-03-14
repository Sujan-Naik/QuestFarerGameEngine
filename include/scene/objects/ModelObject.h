#ifndef QUESTFARERGAMEENGINE_MODELOBJECT_H
#define QUESTFARERGAMEENGINE_MODELOBJECT_H

#include "../../model/Model.h"
#include "GameObject.h"

struct ModelObject : GameObject {
    Model* model;
    Shader* shader;
    void draw(const RenderContext& ctx) override {
        shader->setMat4("view", ctx.view);
        model->Draw(*shader);
    }
};


#endif //QUESTFARERGAMEENGINE_MODELOBJECT_H
