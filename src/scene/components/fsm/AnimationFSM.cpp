
#include "../../../../include/scene/components/fsm/AnimationFSM.h"

namespace scene::components::fsm {

    void AnimationFSM::Initialize(const TransitionMap& transitions, std::shared_ptr<State> initialState) {
        m_transitionsDictionary = transitions;
        m_currentState = initialState;
        m_currentStateEntered = false;
    }

    void AnimationFSM::UpdateParameters(float speed, glm::vec2 direction) {
        m_context.speed = speed;
        m_context.direction = direction;
    }

    void AnimationFSM::Update(float deltaTime) {
        if (m_currentState) {
            if (!m_currentStateEntered) {
                m_currentState->Enter();
                m_currentStateEntered = true;
            }
            m_currentState->Update(deltaTime, m_context);
        }

        auto it = m_transitionsDictionary.find(m_currentState);
        if (it != m_transitionsDictionary.end()) {
            for (const auto& pair : it->second) {
                if (pair.transition->ShouldTransition()) {
                    m_currentState->Exit();
                    m_currentState = pair.state;
                    m_currentState->Enter();
                    m_currentStateEntered = true;
                    break;
                }
            }
        }
    }

    AnimationStateOutput AnimationFSM::GetOutput() const {
        return m_currentState ? m_currentState->GetOutput() : AnimationStateOutput();
    }

    std::shared_ptr<State> AnimationFSM::GetCurrentState() const {
        return m_currentState;
    }
}