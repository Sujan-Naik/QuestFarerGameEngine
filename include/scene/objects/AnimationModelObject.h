#ifndef QUESTFARERGAMEENGINE_ANIMATIONMODELOBJECT_H
#define QUESTFARERGAMEENGINE_ANIMATIONMODELOBJECT_H

#include <memory>
#include <iostream>
#include "../../rendering/RenderContext.h"
#include "GameObject.h"
#include "../../animation/Animator.h"

struct AnimationModelObject : GameObject {
    std::shared_ptr<ModelAnimation> model;
    std::unique_ptr<Shader> shader;
    std::shared_ptr<animation::Animator> animator;

    glm::vec3 previousFrameRootPos = glm::vec3(0.0f);
    glm::quat previousFrameRootRot = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

    glm::vec3 lastFrameDeltaPos = glm::vec3(0.0f);
    glm::quat lastFrameDeltaRot = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

    bool isFirstFrame = true;

    AnimationModelObject(int id, std::shared_ptr<ModelAnimation> model, std::unique_ptr<Shader> shader,
                         Transform transform, std::shared_ptr<animation::Animator> animator, bool active = true)
            : GameObject(id, transform, active), model(std::move(model)), shader(std::move(shader)),
              animator(std::move(animator)) {}

    void resetRootMotionBaseline() {
        // Called whenever an animation switch fires. Captures the new animation's
        // starting root position as the baseline so the first delta is always zero,
        // preventing the rubber band jump between animations.
        previousFrameRootPos = animator->GetRootBonePosition();
        previousFrameRootRot = animator->GetRootBoneRotation();
        lastFrameDeltaPos = glm::vec3(0.0f);
        lastFrameDeltaRot = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        isFirstFrame = false;
    }

    void applyRootMotion() {
        if (!animator) return;

        // If a new animation just started this tick, reset baseline instead of
        // computing a delta — that delta would be garbage (old end vs new start)
        if (animator->WasAnimationSwitched()) {
            resetRootMotionBaseline();
            return;
        }

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
            // Animation looped but didn't switch — extrapolate using last frame's
            // velocity to bridge the seam cleanly
            frameDeltaPos = lastFrameDeltaPos;
            frameDeltaRot = lastFrameDeltaRot;
        } else {
            frameDeltaPos = currentRootPos - previousFrameRootPos;
            frameDeltaRot = currentRootRot * glm::inverse(previousFrameRootRot);
        }

        lastFrameDeltaPos = frameDeltaPos;
        lastFrameDeltaRot = frameDeltaRot;

        glm::vec3 worldDeltaPos = transform.rotation * frameDeltaPos;
        transform.position.x += worldDeltaPos.x;
        transform.position.z += worldDeltaPos.z;

        previousFrameRootPos = currentRootPos;
        previousFrameRootRot = currentRootRot;
    }

    void draw(const RenderContext& ctx) override {
        if (animator) {
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
            modelMat = glm::translate(modelMat, glm::vec3(-rootPos.x, 0.0f, -rootPos.z));
        }

        shader->setMat4("model", modelMat);
        model->Draw(*shader);
    }
};

#endif //QUESTFARERGAMEENGINE_ANIMATIONMODELOBJECT_H