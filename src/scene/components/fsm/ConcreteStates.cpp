
#include "../../../../include/scene/components/fsm/ConcreteStates.h"

namespace scene::components::fsm {

    BaseAnimState::BaseAnimState(CharacterControllerComponent* ctrl, std::shared_ptr<AnimationState> legacyState)
            : m_controller(ctrl), m_legacyAnimState(legacyState) {}

    void BaseAnimState::Enter() {
        if (m_legacyAnimState) {
            m_legacyAnimState->entry();
        }
    }

    void BaseAnimState::Exit() {
        if (m_legacyAnimState) {
            m_legacyAnimState->exit();
        }
    }

    void BaseAnimState::Update(float deltaTime, const BlendParameterContext& ctx) {
        if (m_legacyAnimState) {
            m_legacyAnimState->update(deltaTime, ctx);
        }
    }

    AnimationStateOutput BaseAnimState::GetOutput() const {
        return m_legacyAnimState ? m_legacyAnimState->output : AnimationStateOutput();
    }

    glm::vec3 BaseAnimState::GetRootDeltaThisFrame() const {
        return m_legacyAnimState ? m_legacyAnimState->GetRootDeltaThisFrame() : glm::vec3(0.0f);
    }

    bool BaseAnimState::IsComplete() const {
        return false;
    }

    bool JumpState::IsComplete() const {
        return m_legacyAnimState ? m_legacyAnimState->GetLoopedThisFrame() : false;
    }
}