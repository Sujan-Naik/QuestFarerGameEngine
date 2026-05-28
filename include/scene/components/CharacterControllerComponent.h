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

        // Root motion tracking
        glm::vec3 m_previousRootPos;
        glm::vec3 m_lastFrameClipStart;
        glm::vec3 m_lastFrameClipEnd;

    public:
        explicit CharacterControllerComponent(int entityId, Transform* transformPtr, std::shared_ptr<AnimationFSM> animFSM);
        CharacterControllerComponent();

        std::shared_ptr<AnimationFSM> GetFSM();
        Transform* getTransform() const;

        void initialize(std::shared_ptr<ModelAnimation> model);
        void setLocomotionSpeed(float speed);
        float getSpeed();

        void update() override;
        void receive(int message) override;

    private:
        void updateRootMotion();
        void applyRootMotion(glm::vec3 delta);
    };
}

#endif //QUESTFARERGAMEENGINE_CHARACTERCONTROLLERCOMPONENT_H