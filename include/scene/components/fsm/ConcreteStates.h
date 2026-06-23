#ifndef QUESTFARERGAMEENGINE_CONCRETE_STATES_H
#define QUESTFARERGAMEENGINE_CONCRETE_STATES_H

#include "State.h"
#include <memory>

namespace scene::components {
    class CharacterControllerComponent;
}

namespace scene::components::fsm {

    class BaseAnimState : public State {
    protected:
        CharacterControllerComponent* m_controller;
        std::shared_ptr<AnimationState> m_legacyAnimState;

    public:
        BaseAnimState(CharacterControllerComponent* ctrl, std::shared_ptr<AnimationState> legacyState);

        void Enter() override;
        void Exit() override;
        void Update(float deltaTime, const BlendParameterContext& ctx) override;

        AnimationStateOutput GetOutput() const override;
        glm::vec3 GetRootDeltaThisFrame() const override;
        bool IsComplete() const override;
    };

    class IdleState : public BaseAnimState {
    public:
        using BaseAnimState::BaseAnimState;
    };

    class LocomotionState : public BaseAnimState {
    public:
        using BaseAnimState::BaseAnimState;
    };

    class JumpState : public BaseAnimState {
    public:
        using BaseAnimState::BaseAnimState;
        bool IsComplete() const override;
    };

    class PunchState : public BaseAnimState {
    public:
        using BaseAnimState::BaseAnimState;
        bool IsComplete() const override;
    };
}

#endif