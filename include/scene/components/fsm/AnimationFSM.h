#ifndef QUESTFARERGAMEENGINE_ANIMATIONFSM_H
#define QUESTFARERGAMEENGINE_ANIMATIONFSM_H

#include "State.h"
#include "Transition.h"
#include <unordered_map>
#include <vector>
#include <memory>

namespace scene::components::fsm {

    class AnimationFSM {
    public:
        struct TransitionStatePair {
            std::shared_ptr<State> state;
            std::shared_ptr<Transition> transition;

            TransitionStatePair(std::shared_ptr<State> s, std::shared_ptr<Transition> t)
                    : state(s), transition(t) {}
        };

        using TransitionMap = std::unordered_map<std::shared_ptr<State>, std::vector<TransitionStatePair>>;

    private:
        TransitionMap m_transitionsDictionary;
        std::shared_ptr<State> m_currentState = nullptr;
        BlendParameterContext m_context;
        bool m_currentStateEntered = false;

    public:
        AnimationFSM() = default;

        void Initialize(const TransitionMap& transitions, std::shared_ptr<State> initialState);
        void UpdateParameters(float speed, glm::vec2 direction);
        void Update(float deltaTime);

        AnimationStateOutput GetOutput() const;
        std::shared_ptr<State> GetCurrentState() const;
    };
}

#endif