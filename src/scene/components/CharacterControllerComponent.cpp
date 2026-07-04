#include "../../../include/scene/components/CharacterControllerComponent.h"
#include <utility>

namespace scene::components {

    CharacterControllerComponent::CharacterControllerComponent(int entityId, Transform* transformPtr, std::shared_ptr<AnimationFSM> animFSM)
            : Component(entityId), transform(transformPtr), fsm(std::move(animFSM)),
              m_currentSpeedFactor(0.0f), m_currentDirection(0.0f), m_wantsToJump(false), m_wantsToPunch(false), m_activePunchIntent(BoxingPunch::None) {}

    CharacterControllerComponent::CharacterControllerComponent()
            : Component(-1), transform(nullptr), fsm(std::make_shared<AnimationFSM>()),
              m_currentSpeedFactor(0.0f), m_currentDirection(0.0f), m_wantsToJump(false), m_wantsToPunch(false), m_activePunchIntent(BoxingPunch::None) {}

    bool CharacterControllerComponent::getWantsToPunch() const {
        return m_wantsToPunch;
    }
}