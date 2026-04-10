#ifndef QUESTFARERGAMEENGINE_CHARACTERANIMATIONCONTROLLER_H
#define QUESTFARERGAMEENGINE_CHARACTERANIMATIONCONTROLLER_H


#ifndef QUESTFARERGAMEENGINE_CHARACTERCONTROLLER_H
#define QUESTFARERGAMEENGINE_CHARACTERCONTROLLER_H

#include <string>
#include <map>
#include <memory>
#include "Animation.h"
#include "Animator.h"
#include "../rendering/mesh/Mesh.h"
#include "../rendering/model/ModelAnimation.h"

using namespace rendering::mesh;
using namespace rendering::model;

namespace animation {
    enum class AnimationState {
        IDLE,
        WALK,
        RUN,
        JUMP,
        ATTACK
    };

    class CharacterController {
    private:
        std::shared_ptr<ModelAnimation> skeleton;
        std::unique_ptr<Animator> animator;
        std::map<AnimationState, std::unique_ptr<Animation>> animations;
        AnimationState currentState = AnimationState::IDLE;

    public:
        CharacterController(std::shared_ptr<ModelAnimation> modelAnimation)
                : skeleton(modelAnimation) {
            animator = std::make_unique<Animator>(nullptr);
        }

        void loadAnimations(const std::map<AnimationState, std::string>& animationPaths) {
            for (const auto& [state, path] : animationPaths) {
                animations[state] = std::make_unique<Animation>(path, skeleton.get());
            }
            // Start with idle
            transitionTo(AnimationState::IDLE);
        }

        void transitionTo(AnimationState newState) {
            if (newState == currentState) return;  // Already playing

            currentState = newState;
            if (animations.find(newState) != animations.end()) {
                animator->PlayAnimation(
                        std::make_unique<Animation>(*animations[newState])
                );
            }
        }

        void update(float dt) {
            animator->UpdateAnimation(dt);
        }

        AnimationState getCurrentState() const {
            return currentState;
        }

//        glm::vec3 getRootMotionDelta() const {
//            return animator->GetRootMotionDelta();
//        }

        std::vector<glm::mat4> getFinalBoneMatrices() const {
            return animator->GetFinalBoneMatrices();
        }
    };
}

#endif


#endif //QUESTFARERGAMEENGINE_CHARACTERANIMATIONCONTROLLER_H
