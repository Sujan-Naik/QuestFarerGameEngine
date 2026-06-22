#include "../../../include/scene/systems/CharacterControllerSystem.h"
#include "../../../include/scene/components/CharacterControllerComponent.h"
#include "../../../include/scene/components/PhysicsComponent.h"
#include "../../../include/scene/components/ECSManager.h"
#include "../../../include/globals.h"

namespace scene::components {

    void CharacterControllerSystem::update(ECSManager& ecs, float dt) {
        CharacterControllerComponent* controllers = ecs.getCharacterControllerComponentsDense();
        int count = ecs.getCharacterControllerComponentsAmount();

        for (int i = 0; i < count; ++i) {
            CharacterControllerComponent& ctrl = controllers[i];
            if (ctrl.getEntityId() == -1 || !ctrl.fsm) continue;

            PhysicsComponent* physics = ecs.getPhysicsComponent(ctrl.getEntityId());

            // 1. Snapshot the current state pointer before checking transitions
            auto previousState = ctrl.fsm->GetCurrentState();

            ctrl.fsm->UpdateParameters(ctrl.m_currentSpeedFactor, ctrl.m_currentDirection);
            ctrl.fsm->Update(FIXED_TIMESTEP);

            auto currentState = ctrl.fsm->GetCurrentState();
            if (!currentState) continue;

            // 2. CRITICAL FIX: If the state changed this frame, force an immediate data evaluation
            // on the new state so its visual matrices are populated before the engine draws them.
            if (currentState != previousState) {
                fsm::BlendParameterContext ctx;
                ctx.speed = ctrl.m_currentSpeedFactor;
                ctx.direction = ctrl.m_currentDirection;
                currentState->Update(FIXED_TIMESTEP, ctx);
            }

            if (ctrl.m_wantsToJump) {
                ctrl.m_wantsToJump = false;
            }

            // 3. Now it is completely safe to read deltas and bones without 1-frame gaps
            glm::vec3 delta = currentState->GetRootDeltaThisFrame();
            glm::vec3 worldDelta = (ctrl.transform->getForward() * delta.z) +
                                   (ctrl.transform->getRight() * delta.x) +
                                   (ctrl.transform->getUp() * delta.y);

            if (ctrl.skeleton) {
                ctrl.skeleton->SetFinalBoneMatrices(ctrl.fsm->GetOutput().finalBoneMatrices);
            }
            if (physics) {
                physics->addVelocity(worldDelta * ctrl.m_movementScale);
            }
        }
    }

    void CharacterControllerSystem::setLocomotionInput(CharacterControllerComponent& ctrl, float forward, float strafe, bool isSprinting) {
        ctrl.m_currentSpeedFactor = glm::length(glm::vec2(strafe, forward));
        ctrl.m_currentDirection = glm::vec2(strafe, forward);
    }

    void CharacterControllerSystem::triggerJump(ECSManager& ecs, CharacterControllerComponent& ctrl) {
        ctrl.m_wantsToJump = true;
    }

    void CharacterControllerSystem::receiveMessage(CharacterControllerComponent& ctrl, int message) {
        switch (message) {
            case 0: setLocomotionInput(ctrl, 0.0f, 0.0f, false); break;
            case 1: setLocomotionInput(ctrl, 0.5f, 0.0f, false); break;
            case 2: setLocomotionInput(ctrl, 1.0f, 0.0f, true);  break;
        }
    }
}