#ifndef QUESTFARERGAMEENGINE_CHARACTERCONTROLLERCOMPONENT_H
#define QUESTFARERGAMEENGINE_CHARACTERCONTROLLERCOMPONENT_H

#include "Component.h"
#include "../../rendering/model/ModelAnimation.h"
#include "../../scene/components/fsm/AnimationState.h"
#include "../../scene/components/fsm/AnimationFSM.h"
#include "../geometry/Transform.h"
#include "fsm/BoxingPunch.h"
#include <memory>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

using namespace rendering::mesh;
using namespace rendering::model;
using namespace scene::components::fsm;

namespace scene::components {

    struct DynamicBoneState {
        glm::quat dynamicRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        glm::vec3 angularVelocity = glm::vec3(0.0f);
    };

    class CharacterControllerComponent : public Component {
    private:
        bool m_wantsToJump = false;
        bool m_wantsToPunch = false;
        bool updateAnimations = true;
    public:
        std::shared_ptr<AnimationFSM> fsm;
        std::shared_ptr<ModelAnimation> skeleton;
        Transform* transform = nullptr;

        float m_currentSpeedFactor = 0.0f;
        glm::vec2 m_currentDirection{0.0f};

        BoxingPunch m_activePunchIntent = BoxingPunch::None;

        std::unordered_map<int, DynamicBoneState> dynamicBones;

        explicit CharacterControllerComponent(int entityId, Transform* transformPtr, std::shared_ptr<AnimationFSM> animFSM);
        CharacterControllerComponent();

        bool shouldUpdateAnimations() const;
        void setUpdateAnimations(bool updateAnimations);

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