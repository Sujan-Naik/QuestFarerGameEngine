#ifndef QUESTFARERGAMEENGINE_CHARACTERCONTROLLERSYSTEM_H
#define QUESTFARERGAMEENGINE_CHARACTERCONTROLLERSYSTEM_H

#include "System.h"
#include "../components/CharacterControllerComponent.h"

namespace scene::components {

    class CharacterControllerSystem : public System {
    public:
        void update(ECSManager& ecs, float dt) override;
        void setLocomotionInput(CharacterControllerComponent& ctrl, float forward, float strafe, bool isSprinting);
        void triggerJump(ECSManager& ecs, CharacterControllerComponent& ctrl);
        void triggerPunch(ECSManager& ecs, CharacterControllerComponent& ctrl);
        void receiveMessage(CharacterControllerComponent& ctrl, int message);
    };
}

#endif