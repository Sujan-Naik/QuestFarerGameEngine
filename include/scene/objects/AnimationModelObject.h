#ifndef QUESTFARERGAMEENGINE_ANIMATIONMODELOBJECT_H
#define QUESTFARERGAMEENGINE_ANIMATIONMODELOBJECT_H

#include <memory>
#include <iostream>
#include "../../rendering/RenderContext.h"
#include "GameObject.h"
#include "../../animation/Animator.h"

struct AnimationModelObject: GameObject {
    std::shared_ptr<ModelAnimation> model;
    std::unique_ptr<Shader> shader;
    std::shared_ptr<animation::Animator> animator;

    glm::vec3 previousFrameRootPos = glm::vec3(0.0f);
    glm::quat previousFrameRootRot = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

    // NEW: Trackers to maintain velocity during the loop-wrap frame
    glm::vec3 lastFrameDeltaPos = glm::vec3(0.0f);
    glm::quat lastFrameDeltaRot = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

    bool isFirstFrame = true;

    AnimationModelObject(int id, std::shared_ptr<ModelAnimation> model, std::unique_ptr<Shader> shader,
                         Transform transform, std::shared_ptr<animation::Animator> animator, bool active = true)
            : GameObject(id, transform, active), model(std::move(model)), shader(std::move(shader)),
              animator(std::move(animator)) {
    }

    void applyRootMotion() {
        if (!animator) return;

        glm::vec3 currentRootPos = animator->GetRootBonePosition();
        glm::quat currentRootRot = animator->GetRootBoneRotation();

        if (isFirstFrame) {
            previousFrameRootPos = currentRootPos;
            previousFrameRootRot = currentRootRot;
            isFirstFrame = false;
            return;
        }

        glm::vec3 frameDeltaPos;
        glm::quat frameDeltaRot;

        if (animator->DidLoopThisFrame()) {
            // THE FIX: Extrapolate this single frame's motion using the previous frame's velocity.
            // This perfectly bridges the gap between the end of the clip and the start of the next,
            // preventing the 1-frame stall that causes the visual "pop back".
            frameDeltaPos = lastFrameDeltaPos;
            frameDeltaRot = lastFrameDeltaRot;
        } else {
            frameDeltaPos = currentRootPos - previousFrameRootPos;
            frameDeltaRot = currentRootRot * glm::inverse(previousFrameRootRot);
        }

        // Cache the delta for the next frame (in case the next frame is a loop)
        lastFrameDeltaPos = frameDeltaPos;
        lastFrameDeltaRot = frameDeltaRot;

        // Transform local animation motion into World Space
        glm::vec3 worldDeltaPos = transform.rotation * frameDeltaPos;

        // Apply continuously to GameObject transform
        transform.position.x += worldDeltaPos.x;
        transform.position.z += worldDeltaPos.z;

        previousFrameRootPos = currentRootPos;
        previousFrameRootRot = currentRootRot;
    }

    void draw(const RenderContext& ctx) override {
        if (animator) {
            animator->UpdateAnimation(FIXED_TIMESTEP);
            applyRootMotion();
        }

        shader->use();
        auto transforms = animator ? animator->GetFinalBoneMatrices() : std::vector<glm::mat4>();
        for (int i = 0; i < transforms.size(); ++i) {
            shader->setMat4("finalBonesMatrices[" + std::to_string(i) + "]", transforms[i]);
        }

        shader->setMat4("view", ctx.view);
        shader->setMat4("projection", ctx.projection);

        glm::mat4 modelMat = getModelMatrix();

        if (animator) {
            glm::vec3 rootPos = animator->GetRootBonePosition();
            // Counter-act the animation's visual forward translation
            modelMat = glm::translate(modelMat, glm::vec3(-rootPos.x, 0.0f, -rootPos.z));
        }

        shader->setMat4("model", modelMat);
        model->Draw(*shader);
    }
};

#endif //QUESTFARERGAMEENGINE_ANIMATIONMODELOBJECT_H