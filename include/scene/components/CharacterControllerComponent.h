#ifndef QUESTFARERGAMEENGINE_CHARACTERCONTROLLERCOMPONENT_H
#define QUESTFARERGAMEENGINE_CHARACTERCONTROLLERCOMPONENT_H

#include "Component.h"
#include "../../rendering/model/ModelAnimation.h"
#include "../../scene/components/fsm/AnimationState.h"
#include "../../scene/components/fsm/AnimationFSM.h"
#include "../geometry/Transform.h"
#include <memory>
#include <glm/glm.hpp>

using namespace rendering::mesh;
using namespace rendering::model;
using namespace scene::components::fsm;

namespace scene::components {

    class CharacterControllerComponent : public Component {
    public:
        std::shared_ptr<AnimationFSM> fsm;
        std::shared_ptr<ModelAnimation> skeleton;
        Transform* transform = nullptr;

        float m_currentSpeedFactor = 0.0f;
        glm::vec2 m_currentDirection{0.0f};
        float m_movementScale = 0.12f;
        bool m_wantsToJump = false;
        bool m_wantsToPunch = false;

        explicit CharacterControllerComponent(int entityId, Transform* transformPtr, std::shared_ptr<AnimationFSM> animFSM);
        CharacterControllerComponent();

        void triggerJump() {
            m_wantsToJump = true;
        }

        void triggerPunch() {
            m_wantsToPunch = true;
        }
    };
}

#endif // QUESTFARERGAMEENGINE_CHARACTERCONTROLLERCOMPONENT_H