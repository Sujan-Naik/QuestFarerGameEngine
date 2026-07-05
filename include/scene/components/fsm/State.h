#ifndef QUESTFARERGAMEENGINE_STATE_H
#define QUESTFARERGAMEENGINE_STATE_H

#include "AnimationState.h"

namespace scene::components::fsm {

    class State {
    public:
        virtual ~State() = default;

        virtual void Enter() = 0;
        virtual void Exit() = 0;
        virtual void Update(float deltaTime, const BlendParameterContext& ctx) = 0;

        virtual AnimationStateOutput GetOutput() const = 0;
        virtual glm::vec3 GetRootDeltaThisFrame() const = 0;
        virtual bool IsComplete() const = 0;
    };
}

#endif