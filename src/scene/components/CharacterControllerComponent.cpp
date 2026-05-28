#include <iostream>
#include "../../../include/scene/components/CharacterControllerComponent.h"
#include "../../../include/globals.h"
#include "../../../include/scene/components/PhysicsComponent.h"
#include "../../../include/scene/components/ECSManager.h"

namespace scene::components {

    CharacterControllerComponent::CharacterControllerComponent(int entityId, Transform* transformPtr, std::shared_ptr<AnimationFSM> animFSM)
            : Component(entityId), transform(transformPtr), fsm(animFSM),
              m_previousRootPos(0.0f), m_lastFrameClipStart(0.0f), m_lastFrameClipEnd(0.0f) {}

    CharacterControllerComponent::CharacterControllerComponent()
            : Component(-1), transform(nullptr), fsm(std::make_shared<AnimationFSM>()),
              m_previousRootPos(0.0f), m_lastFrameClipStart(0.0f), m_lastFrameClipEnd(0.0f) {}

    std::shared_ptr<AnimationFSM> CharacterControllerComponent::GetFSM() {
        return fsm;
    }

    Transform* CharacterControllerComponent::getTransform() const {
        return transform;
    }

    void CharacterControllerComponent::initialize(std::shared_ptr<ModelAnimation> model) {
        skeleton = model;
    }

    void CharacterControllerComponent::setLocomotionSpeed(float speed) {
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

    float CharacterControllerComponent::getSpeed() {
        if (auto simpleState = fsm->GetCurrentState<SimpleAnimationState>()) {
            return 0;
        } else if (auto locomotionState = fsm->GetCurrentState<LocomotionBlendState>()) {
            return locomotionState->getSpeed();
        }
    }

    void CharacterControllerComponent::update() {
        fsm->Update(FIXED_TIMESTEP);
        updateRootMotion();
    }

    void CharacterControllerComponent::receive(int message) {
        switch (message) {
            case 0: setLocomotionSpeed(0.0f);   break;
            case 1: setLocomotionSpeed(0.5f);   break;
            case 2: setLocomotionSpeed(1.0f);   break;
        }
    }

    void CharacterControllerComponent::updateRootMotion() {
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

//            std::cout << "--- LOOP LOG ---" << std::endl;
//            std::cout << "DistToEnd: " << glm::to_string(distanceToEnd) << std::endl;
//            std::cout << "DistFromStart: " << glm::to_string(distanceFromStart) << std::endl;
//            std::cout << "Final Delta: " << glm::to_string(deltaRootPos) << std::endl;
//            std::cout << "m_lastFrameClipEnd: " << glm::to_string(m_lastFrameClipEnd) << std::endl;
//            std::cout << "m_previousRootPos: " << glm::to_string(m_previousRootPos) << std::endl;
        } else {
            deltaRootPos = currentRootPos - m_previousRootPos;
        }

        applyRootMotion(deltaRootPos);

        m_previousRootPos = currentRootPos;
        m_lastFrameClipStart = currentClipStart;
        m_lastFrameClipEnd = currentClipEnd;
    }

    void CharacterControllerComponent::applyRootMotion(glm::vec3 delta) {
        glm::vec3 worldDelta = transform->getForward() * delta.z +
                               transform->getRight()   * delta.x +
                               transform->getUp()      * delta.y;

        PhysicsComponent* physics = ecsManager->getPhysicsComponent(entityId);

        skeleton->SetFinalBoneMatrices(fsm->GetOutput().finalBoneMatrices);
        if (physics) {
            physics->addVelocity(worldDelta * 0.01f);
        }
    }
}