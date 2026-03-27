#ifndef QUESTFARERGAMEENGINE_MODELOBJECT_H
#define QUESTFARERGAMEENGINE_MODELOBJECT_H

#include <memory>
#include "../../model/Model.h"
#include "GameObject.h"

struct ModelObject : GameObject {
    std::unique_ptr<Model> model;
    std::unique_ptr<Shader> shader;


    explicit ModelObject(int id, std::unique_ptr<Model> model,std::unique_ptr<Shader> shader, bool active = true)
            : GameObject(id, active), model(std::move(model)), shader(std::move(shader)) {}

    void draw(const RenderContext& ctx) override {
        shader->setMat4("view", ctx.view);
        model->Draw(*shader);
    }


};


#endif //QUESTFARERGAMEENGINE_MODELOBJECT_H
