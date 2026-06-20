#include <iostream>
#include "../../../include/scene/components/CharacterControllerComponent.h"
#include "../../../include/globals.h"
#include "../../../include/scene/components/PhysicsComponent.h"
#include "../../../include/scene/components/ECSManager.h"

namespace scene::components {

    CharacterControllerComponent::CharacterControllerComponent(int entityId, Transform* transformPtr, std::shared_ptr<AnimationFSM> animFSM)
            : Component(entityId), transform(transformPtr), fsm(animFSM),
              m_currentSpeedFactor(0.0f), m_currentDirection(0.0f) {}

    CharacterControllerComponent::CharacterControllerComponent()
            : Component(-1), transform(nullptr), fsm(std::make_shared<AnimationFSM>()),
              m_currentSpeedFactor(0.0f), m_currentDirection(0.0f) {}

    std::shared_ptr<AnimationFSM> CharacterControllerComponent::GetFSM() {
        return fsm;
    }

    Transform* CharacterControllerComponent::getTransform() const {
        return transform;
    }

    void CharacterControllerComponent::initialize(std::shared_ptr<ModelAnimation> model) {
        skeleton = model;
    }

    void CharacterControllerComponent::setLocomotionInput(float forward, float strafe, bool isSprinting) {
        m_currentSpeedFactor = glm::length(glm::vec2(strafe, forward));
        m_currentDirection = glm::vec2(strafe, forward);

        if (m_currentSpeedFactor < 0.01f) {
            fsm->TransitionTo("Idle");
        } else {
            fsm->TransitionTo("Locomotion");
        }

        fsm->UpdateParameters(m_currentSpeedFactor, m_currentDirection);
    }

    void CharacterControllerComponent::triggerJump() {
        PhysicsComponent* physics = ecsManager->getPhysicsComponent(entityId);
        if (physics && physics->onGround) {
            fsm->TransitionTo("Jump");
        }
    }

    float CharacterControllerComponent::getSpeed() {
        return m_currentSpeedFactor;
    }

    void CharacterControllerComponent::update() {
        fsm->UpdateParameters(m_currentSpeedFactor, m_currentDirection);
        fsm->Update(FIXED_TIMESTEP);
        updateRootMotion();
    }

    void CharacterControllerComponent::receive(int message) {
        switch (message) {
            case 0: setLocomotionInput(0.0f, 0.0f, false); break;
            case 1: setLocomotionInput(0.5f, 0.0f, false); break;
            case 2: setLocomotionInput(1.0f, 0.0f, true);  break;
        }
    }

    void CharacterControllerComponent::updateRootMotion() {
        if (!fsm) return;

        auto currentState = fsm->GetCurrentState();
        if (!currentState) return;

        glm::vec3 deltaRootPos = currentState->GetRootDeltaThisFrame();
        applyRootMotion(deltaRootPos);
    }

    void CharacterControllerComponent::applyRootMotion(glm::vec3 delta) {
        glm::vec3 worldDelta = (transform->getForward() * delta.z) +
                               (transform->getRight() * delta.x) +
                               (transform->getUp() * delta.y);

        PhysicsComponent* physics = ecsManager->getPhysicsComponent(entityId);

        skeleton->SetFinalBoneMatrices(fsm->GetOutput().finalBoneMatrices);
        if (physics) {
            physics->addVelocity(worldDelta * m_movementScale);
        }
    }
}