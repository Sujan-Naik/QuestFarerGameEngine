#ifndef QUESTFARERGAMEENGINE_CHARACTERCONTROLLERCOMPONENT_H
#define QUESTFARERGAMEENGINE_CHARACTERCONTROLLERCOMPONENT_H

#include "Component.h"
#include "../../rendering/model/ModelAnimation.h"
#include "../../scene/components/fsm/AnimationState.h"
#include "../../scene/components/fsm/AnimationFSM.h"
#include "../geometry/Transform.h"
#include "fsm/BoxingPunch.h"
#include <memory>
#include <glm/glm.hpp>

using namespace rendering::mesh;
using namespace rendering::model;
using namespace scene::components::fsm;

namespace scene::components {

    class CharacterControllerComponent : public Component {
    private:
        bool m_wantsToJump = false;
        bool m_wantsToPunch = false;
    public:
        std::shared_ptr<AnimationFSM> fsm;
        std::shared_ptr<ModelAnimation> skeleton;
        Transform* transform = nullptr;

        float m_currentSpeedFactor = 0.0f;
        glm::vec2 m_currentDirection{0.0f};

        BoxingPunch m_activePunchIntent = BoxingPunch::None;

        explicit CharacterControllerComponent(int entityId, Transform* transformPtr, std::shared_ptr<AnimationFSM> animFSM);
        CharacterControllerComponent();

        bool getWantsToJump(){
            return m_wantsToJump;
        }

        bool getWantsToPunch() const;

        void triggerJump() {
            if (!m_wantsToJump){
                std::cout<< "jumping" << std::endl;
                m_wantsToJump = true;
            }
        }

        void clearJump() {
            m_wantsToJump = false;
        }


        void triggerPunch() {
            m_wantsToPunch = true;
        }

        void clearPunch() {
            m_wantsToPunch = false;
        }
    };
}

#endif