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

    unsigned int prototypeTexID = 0;
    bool hasLoadedPrototype = false;

    AnimationModelObject(int id, std::shared_ptr<ModelAnimation> model, std::unique_ptr<Shader> shader,
                         Transform transform, std::shared_ptr<AnimationFSM> fsm, bool active = true)
            : GameObject(id, transform, active), model(std::move(model)), shader(std::move(shader)),
              animationFSM(std::move(fsm)) {}

    void draw(const RenderContext& ctx) override {
        shader->use();

        if (!hasLoadedPrototype) {
            prototypeTexID = model->TextureFromFile("prototype.png", "resources/images", false);
            hasLoadedPrototype = (prototypeTexID != 0);
        }

        if (hasLoadedPrototype) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, prototypeTexID);
            shader->setInt("texture_diffuse1", 0);
        }

        if (animationFSM) {
            // Fetch the bone matrices (which already contain the IK adjustments applied in PhysicsSystem)
            auto bonesToRender = model->GetAdjustedBoneMatrices();

            for (int i = 0; i < bonesToRender.size(); ++i) {
                shader->setMat4("finalBonesMatrices[" + std::to_string(i) + "]", bonesToRender[i]);
            }
        }

        shader->setMat4("model", getModelMatrix());
        shader->setMat4("view", ctx.view);
        shader->setMat4("projection", ctx.projection);
        model->Draw(*shader);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
};

#endif