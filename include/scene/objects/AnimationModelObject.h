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
            // Use extrapolated velocity to bridge the loop gap
            frameDeltaPos = lastFrameDeltaPos;
            frameDeltaRot = lastFrameDeltaRot;
        } else {
            frameDeltaPos = currentRootPos - previousFrameRootPos;
            frameDeltaRot = currentRootRot * glm::inverse(previousFrameRootRot);
        }

        // Cache deltas for the next frame
        lastFrameDeltaPos = frameDeltaPos;
        lastFrameDeltaRot = frameDeltaRot;

        // Apply movement: only affect X and Z to keep character on ground
        // Note: transform.rotation is the GameObject's world orientation
        glm::vec3 worldDeltaPos = transform.rotation * frameDeltaPos;
        transform.position.x += worldDeltaPos.x;
        transform.position.z += worldDeltaPos.z;

        // Optional: If you want the animation to turn the GameObject
        // transform.rotation = transform.rotation * frameDeltaRot;

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

        // Calculate model matrix from GameObject transform (Position, Rotation, Scale)
        glm::mat4 modelMat = getModelMatrix();

        if (animator) {
            glm::vec3 rootPos = animator->GetRootBonePosition();
            // Counter-act the animation's visual forward translation so the mesh stays at GameObject origin
            // We only subtract X and Z because we are applying those to the GameObject transform
            modelMat = glm::translate(modelMat, glm::vec3(-rootPos.x, 0.0f, -rootPos.z));
        }

        shader->setMat4("model", modelMat);
        model->Draw(*shader);
    }
};

#endif