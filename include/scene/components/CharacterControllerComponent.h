#ifndef QUESTFARERGAMEENGINE_CHARACTERCONTROLLERCOMPONENT_H
#define QUESTFARERGAMEENGINE_CHARACTERCONTROLLERCOMPONENT_H

#include "Component.h"
#include "../../rendering/model/ModelAnimation.h"
#include "../../animation/Animator.h"

#include <memory>
#include <map>

using namespace rendering::mesh;
using namespace rendering::model;
using namespace animation;

namespace scene::components {
    class CharacterControllerComponent : public Component {

    public:
        enum class AnimationState {
            IDLE,
            WALK,
            RUN,
            JUMP,
            ATTACK
        };

    private:
        std::shared_ptr<ModelAnimation> skeleton;
        std::shared_ptr<Animator> animator;
        std::map<AnimationState, std::shared_ptr<Animation>> animations;
        AnimationState currentState = AnimationState::IDLE;
        bool stateInitialized = false;

        Transform* transform;

    public:
        explicit CharacterControllerComponent(int entityId, Transform *transform)
                : Component(entityId), transform(transform) {}

        CharacterControllerComponent() : Component(-1) {}

        void initialize(std::shared_ptr<ModelAnimation> model, std::shared_ptr<Animator> anim) {
            skeleton = model;
            animator = anim;
        }

        void registerAnimation(AnimationState state, const std::string &animationPath) {
            if (!skeleton) {
                std::cerr << "Error: Skeleton not initialized\n";
                return;
            }

            if (animationPath.empty()) {
                std::cerr << "Error: Animation path is empty\n";
                return;
            }

            std::cerr << "Registering animation: " << animationPath << std::endl;

            try {
                auto newAnimation = std::make_shared<Animation>(animationPath, skeleton.get());
                animations[state] = newAnimation;
                std::cerr << "Animation registered successfully\n";
            } catch (const std::exception& e) {
                std::cerr << "Exception during animation creation: " << e.what() << "\n";
            }
        }

        void switchAnimation(AnimationState newState) {
            // Don't re-queue the same state that's already pending
            if (stateInitialized && newState == currentState) {
                return;
            }

            if (animations.find(newState) == animations.end()) {
                std::cerr << "Error: Animation state not registered\n";
                return;
            }

            if (animator) {

                stateInitialized = true;



                switch (currentState){
                    case AnimationState::IDLE:
                        animator->ForceAnimation(animations[newState]);
                        break;
                    case AnimationState::WALK:
                        animator->PlayAnimation(animations[newState]);
                        break;
                    case AnimationState::RUN:
                        animator->PlayAnimation(animations[newState]);
                        break;

                }
                currentState = newState;

            }
        }

        AnimationState getCurrentState() const {
            return currentState;
        }

        void receive(int message) override {
            switch (message) {
                case 0: switchAnimation(AnimationState::IDLE);   break;
                case 1: switchAnimation(AnimationState::WALK);   break;
                case 2: switchAnimation(AnimationState::RUN);    break;
                case 3: switchAnimation(AnimationState::JUMP);   break;
            }
        }

        void update() override {
            if (animator) {
                animator->UpdateAnimation(FIXED_TIMESTEP);
            }
        }

        Transform* getTransform() {
            return transform;
        }
    };
}

#endif //QUESTFARERGAMEENGINE_CHARACTERCONTROLLERCOMPONENT_H