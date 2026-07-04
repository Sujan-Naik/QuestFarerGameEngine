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
        if (m_controller->getWantsToJump()) {
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
        if (m_jumpState->IsComplete()){
            m_controller->clearJump();
        }


        return m_jumpState->IsComplete();
    }

    AnyToPunchTransition::AnyToPunchTransition(CharacterControllerComponent* ctrl, BoxingPunch targetPunch)
            : m_controller(ctrl), m_targetPunch(targetPunch) {}

    bool AnyToPunchTransition::ShouldTransition() {
        if (m_controller && m_controller->getWantsToPunch() && m_controller->m_activePunchIntent == m_targetPunch) {
            m_controller->clearPunch();
            return true;
        }
        return false;
    }

    PunchToFallbackTransition::PunchToFallbackTransition(CharacterControllerComponent* ctrl, std::shared_ptr<State> punchState)
            : m_controller(ctrl), m_punchState(punchState) {}

    bool PunchToFallbackTransition::ShouldTransition() {
        if (m_punchState && m_punchState->IsComplete()) {
            m_controller->m_activePunchIntent = BoxingPunch::None;
            return true;
        }
        return false;
    }
}