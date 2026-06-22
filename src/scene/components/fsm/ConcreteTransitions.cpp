

#include "../../../../include/scene/components/CharacterControllerComponent.h"
#include "../../../../include/scene/components/fsm/ConcreteTransitions.h"
#include "../../../../include/scene/components/PhysicsComponent.h"

namespace scene::components::fsm {

    IdleToLocomotionTransition::IdleToLocomotionTransition(CharacterControllerComponent* ctrl)
            : m_controller(ctrl) {}

    bool IdleToLocomotionTransition::ShouldTransition() {
        return m_controller->m_currentSpeedFactor > 0.01f;
    }

    LocomotionToIdleTransition::LocomotionToIdleTransition(CharacterControllerComponent* ctrl)
            : m_controller(ctrl) {}

    bool LocomotionToIdleTransition::ShouldTransition() {
        return m_controller->m_currentSpeedFactor <= 0.01f;
    }

    AnyToJumpTransition::AnyToJumpTransition(CharacterControllerComponent* ctrl, ECSManager& ecs)
            : m_controller(ctrl), m_ecs(ecs) {}

    bool AnyToJumpTransition::ShouldTransition() {
        if (m_controller->m_wantsToJump) {
            PhysicsComponent* physics = m_ecs.getPhysicsComponent(m_controller->getEntityId());
            if (physics && physics->onGround) {
                return true;
            }
        }
        return false;
    }

    JumpToFallbackTransition::JumpToFallbackTransition(CharacterControllerComponent* ctrl, std::shared_ptr<State> jumpState)
            : m_controller(ctrl), m_jumpState(jumpState) {}

    bool JumpToFallbackTransition::ShouldTransition() {
        return m_jumpState->IsComplete();
    }
}