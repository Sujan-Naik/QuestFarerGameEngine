#ifndef QUESTFARERGAMEENGINE_CONCRETE_TRANSITIONS_H
#define QUESTFARERGAMEENGINE_CONCRETE_TRANSITIONS_H

#include "Transition.h"
#include "BoxingPunch.h"
#include <memory>
#include "../ECSManager.h"



namespace scene::components {
    class CharacterControllerComponent;
}

namespace scene::components::fsm {

    class State;

    class IdleToLocomotionTransition : public Transition {
    private:
        CharacterControllerComponent* m_controller;
    public:
        explicit IdleToLocomotionTransition(CharacterControllerComponent* ctrl);
        bool ShouldTransition() override;
    };

    class LocomotionToIdleTransition : public Transition {
    private:
        CharacterControllerComponent* m_controller;
    public:
        explicit LocomotionToIdleTransition(CharacterControllerComponent* ctrl);
        bool ShouldTransition() override;
    };

    class AnyToJumpTransition : public Transition {
    private:
        CharacterControllerComponent* m_controller;
        ECSManager& m_ecs;
    public:
        AnyToJumpTransition(CharacterControllerComponent* ctrl, ECSManager& ecs);
        bool ShouldTransition() override;
    };

    class JumpToFallbackTransition : public Transition {
    private:
        CharacterControllerComponent* m_controller;
        ECSManager& m_ecs;
        std::shared_ptr<State> m_jumpState;
    public:
        JumpToFallbackTransition( CharacterControllerComponent* ctrl, ECSManager& m_ecs, std::shared_ptr<State> jumpState);
        bool ShouldTransition() override;
    };

    class AnyToPunchTransition : public Transition {
    private:
        CharacterControllerComponent* m_controller;
        BoxingPunch m_targetPunch;
    public:
        AnyToPunchTransition(CharacterControllerComponent* ctrl, BoxingPunch targetPunch);
        bool ShouldTransition() override;
    };

    class PunchToFallbackTransition : public Transition {
    private:
        CharacterControllerComponent* m_controller;
        std::shared_ptr<State> m_punchState;
    public:
        PunchToFallbackTransition(CharacterControllerComponent* ctrl, std::shared_ptr<State> punchState);
        bool ShouldTransition() override;
    };
}

#endif