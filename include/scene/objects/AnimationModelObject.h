#ifndef QUESTFARERGAMEENGINE_ANIMATIONMODELOBJECT_H
#define QUESTFARERGAMEENGINE_ANIMATIONMODELOBJECT_H

#include <memory>
#include <iostream>
#include "../../rendering/RenderContext.h"
#include "GameObject.h"
#include <glm/gtx/string_cast.hpp>

struct AnimationModelObject : GameObject {
    std::shared_ptr<ModelAnimation> model;
    std::unique_ptr<Shader> shader;
    std::shared_ptr<AnimationFSM> animationFSM;

    AnimationModelObject(int id, std::shared_ptr<ModelAnimation> model, std::unique_ptr<Shader> shader,
                         Transform transform, std::shared_ptr<AnimationFSM> fsm, bool active = true)
            : GameObject(id, transform, active), model(std::move(model)), shader(std::move(shader)),
              animationFSM(std::move(fsm)) {}

    void draw(const RenderContext& ctx) override {
        if (!model || !shader) return;

        shader->use();

        // 1. Pass Bones/Animation Uniforms
        if (animationFSM) {
            auto bonesToRender = model->GetAdjustedBoneMatrices();
            for (size_t i = 0; i < bonesToRender.size(); ++i) {
                shader->setMat4("finalBonesMatrices[" + std::to_string(i) + "]", bonesToRender[i]);
            }
        }

        // 2. Pass Transformation Uniforms
        shader->setMat4("model", getModelMatrix());
        shader->setMat4("view", ctx.view);
        shader->setMat4("projection", ctx.projection);

        // 3. Pass Lighting Uniforms
        shader->setVec3("lightPos", glm::vec3(5.0f, 10.0f, 5.0f));
        shader->setVec3("viewPos", glm::vec3(0.0f, 0.0f, 3.0f));
        shader->setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));

        // 4. Draw the model (This automatically binds the textures parsed by Assimp!)
        model->Draw(*shader);
    }
};

#endif