#ifndef QUESTFARERGAMEENGINE_ANIMATIONMODELOBJECT_H
#define QUESTFARERGAMEENGINE_ANIMATIONMODELOBJECT_H


#include <memory>
#include "../../animation/model_animation.h"
#include "../../rendering/RenderContext.h"
#include "GameObject.h"
#include "../../animation/Animator.h"

struct AnimationModelObject: GameObject {
        std::unique_ptr<animation::Model> model;
        std::unique_ptr<Shader> shader;
    std::unique_ptr<animation::Animator> animator;

         AnimationModelObject(int id, std::unique_ptr<animation::Model> model, std::unique_ptr<Shader> shader, Transform transform, std::unique_ptr<animation::Animator> animator , bool active = true)
                : GameObject(id,transform, active), model(std::move(model)), shader(std::move(shader)), animator(std::move(animator)) {}

        void draw(const RenderContext& ctx) override {



            shader->use();
            auto transforms = animator->GetFinalBoneMatrices();
            for (int i = 0; i < transforms.size(); ++i)
                shader->setMat4("finalBonesMatrices[" + std::to_string(i) + "]", transforms[i]);

            shader->setMat4("view", ctx.view);
            shader->setMat4("projection", ctx.projection);
            shader->setMat4("model", getModelMatrix());
            model->Draw(*shader);
            if (animator){
                animator->UpdateAnimation(FIXED_TIMESTEP);

            }

        }

};


#endif //QUESTFARERGAMEENGINE_ANIMATIONMODELOBJECT_H
