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

using namespace rendering::mesh;
using namespace rendering::model;
using namespace scene::components::fsm;

namespace scene::components {

    class CharacterControllerComponent : public Component {
    private:
        std::shared_ptr<AnimationFSM> fsm;
        std::shared_ptr<ModelAnimation> skeleton;
        Transform* transform;

        float m_currentSpeedFactor;
        glm::vec2 m_currentDirection;
        float m_movementScale = 0.12f;

    public:
        explicit CharacterControllerComponent(int entityId, Transform* transformPtr, std::shared_ptr<AnimationFSM> animFSM);
        CharacterControllerComponent();

        std::shared_ptr<AnimationFSM> GetFSM();
        Transform* getTransform() const;

        void initialize(std::shared_ptr<ModelAnimation> model);
        void setLocomotionInput(float forward, float strafe, bool isSprinting);
        void triggerJump();
        float getSpeed();

        void update() override;
        void receive(int message) override;

    private:
        void updateRootMotion();
        void applyRootMotion(glm::vec3 delta);
    };
}

#endif