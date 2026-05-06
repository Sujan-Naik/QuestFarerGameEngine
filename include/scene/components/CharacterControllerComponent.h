#ifndef QUESTFARERGAMEENGINE_CHARACTERCONTROLLERCOMPONENT_H
#define QUESTFARERGAMEENGINE_CHARACTERCONTROLLERCOMPONENT_H

#include "Component.h"
#include "../../rendering/model/ModelAnimation.h"
#include "../../scene/components/fsm/AnimationState.h"
#include "../../scene/components/fsm/AnimationFSM.h"
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <iostream>

using namespace rendering::mesh;
using namespace rendering::model;
using namespace scene::components::fsm;

namespace scene::components {

    class CharacterControllerComponent : public Component {
    private:
        std::shared_ptr<AnimationFSM> fsm;
        std::shared_ptr<ModelAnimation> skeleton;
        Transform* transform;

        // Root motion tracking
        glm::vec3 m_previousRootPos = glm::vec3(0.0f);
        glm::vec3 m_lastFrameClipStart = glm::vec3(0.0f);
        glm::vec3 m_lastFrameClipEnd = glm::vec3(0.0f);

    public:
        explicit CharacterControllerComponent(int entityId, Transform* transformPtr, std::shared_ptr<AnimationFSM> animFSM)
                : Component(entityId), transform(transformPtr), fsm(animFSM) {}

        CharacterControllerComponent() : Component(-1), transform(nullptr), fsm(std::make_shared<AnimationFSM>()) {}

        std::shared_ptr<AnimationFSM> GetFSM() {
            return fsm;
        }

        Transform* getTransform() const {
            return transform;
        }

        void initialize(std::shared_ptr<ModelAnimation> model) {
            skeleton = model;
        }

        void setLocomotionSpeed(float speed) {
            if (speed < 0.01f) {
                fsm->TransitionTo("Idle");
            } else {
                fsm->TransitionTo("Locomotion");

                auto state = fsm->GetCurrentState<LocomotionBlendState>();
                if (state) {
                    state->SetSpeed(speed);
                }
            }
        }

        float getSpeed(){
            if (auto simpleState = fsm->GetCurrentState<SimpleAnimationState>()) {
                return 0;
            } else if (auto locomotionState = fsm->GetCurrentState<LocomotionBlendState>()) {
                return locomotionState->getSpeed();
            }
        }

        void update() override {
            fsm->Update(FIXED_TIMESTEP);
            updateRootMotion();
        }

        void receive(int message) override {
            switch (message) {
                case 0: setLocomotionSpeed(0.0f);   break;
                case 1: setLocomotionSpeed(0.5f);   break;
                case 2: setLocomotionSpeed(1.0f);   break;
            }
        }

    private:
        void updateRootMotion() {
            if (!fsm) return;

            auto output = fsm->GetOutput();
            glm::vec3 currentRootPos = output.m_rootBonePosition;
            glm::vec3 currentClipStart = fsm->GetClipStartRootPos();
            glm::vec3 currentClipEnd = fsm->GetClipEndRootPos();

            glm::vec3 deltaRootPos(0.0f);

            if (fsm->WasAnimationSwitched()) {
                m_previousRootPos = currentRootPos;
            } else if (fsm->DidLoopThisFrame()) {
                glm::vec3 distanceToEnd = m_lastFrameClipEnd - m_previousRootPos;

                if (distanceToEnd.z < 0.0f) distanceToEnd.z = 0.0f;
                if (distanceToEnd.x < 0.0f) distanceToEnd.x = 0.0f;

                glm::vec3 distanceFromStart = currentRootPos - currentClipStart;

                if (distanceFromStart.z < 0.0f) distanceFromStart.z = 0.0f;

                deltaRootPos = distanceToEnd + distanceFromStart;

                std::cout << "--- LOOP LOG ---" << std::endl;
                std::cout << "DistToEnd: " << glm::to_string(distanceToEnd) << std::endl;
                std::cout << "DistFromStart: " << glm::to_string(distanceFromStart) << std::endl;
                std::cout << "Final Delta: " << glm::to_string(deltaRootPos) << std::endl;
                std::cout << "m_lastFrameClipEnd: " << glm::to_string(m_lastFrameClipEnd) << std::endl;
                std::cout << "m_previousRootPos: " << glm::to_string(m_previousRootPos) << std::endl;
            } else {
                deltaRootPos = currentRootPos - m_previousRootPos;
            }

            applyRootMotion(deltaRootPos);

            m_previousRootPos = currentRootPos;
            m_lastFrameClipStart = currentClipStart;
            m_lastFrameClipEnd = currentClipEnd;
        }

        void applyRootMotion(glm::vec3 delta) {
            glm::vec3 worldDelta = transform->getForward() * delta.z +
                                   transform->getRight()   * delta.x +
                                   transform->getUp()      * delta.y;
            transform->position += worldDelta;
        }
    };
}

#endif //QUESTFARERGAMEENGINE_CHARACTERCONTROLLERCOMPONENT_H