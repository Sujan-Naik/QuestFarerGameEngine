#ifndef QUESTFARERGAMEENGINE_MODELOBJECT_H
#define QUESTFARERGAMEENGINE_MODELOBJECT_H

#include <memory>
#include "GameObject.h"
#include "../../rendering/model/StaticModel.h"

using namespace rendering::model;
using namespace rendering;

struct ModelObject : GameObject {
    std::unique_ptr<StaticModel> model;
    std::unique_ptr<Shader> shader;


    explicit ModelObject(int id, std::unique_ptr<StaticModel> model,std::unique_ptr<Shader> shader, Transform transform, bool active = true)
            : GameObject(id,transform, active), model(std::move(model)), shader(std::move(shader)) {}

    void draw(const RenderContext& ctx) override {

        shader->use();

        shader->setMat4("view", ctx.view);
        shader->setMat4("projection", ctx.projection);
        shader->setMat4("model", getModelMatrix());
        model->Draw(*shader);
    }


};


#endif //QUESTFARERGAMEENGINE_MODELOBJECT_H
