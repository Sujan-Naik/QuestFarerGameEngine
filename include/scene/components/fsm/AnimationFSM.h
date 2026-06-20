#ifndef QUESTFARERGAMEENGINE_ANIMATIONFSM_H
#define QUESTFARERGAMEENGINE_ANIMATIONFSM_H

#include "AnimationState.h"
#include <map>
#include <string>
#include <memory>

namespace scene::components::fsm {

    class AnimationFSM {
    private:
        std::map<std::string, std::shared_ptr<AnimationState>> m_states;
        std::shared_ptr<AnimationState> m_currentState = nullptr;
        std::string m_currentStateName = "";
        BlendParameterContext m_context;
        bool m_wasAnimationSwitched = false;

    public:
        AnimationFSM() = default;

        void RegisterState(const std::string& name, std::shared_ptr<AnimationState> state) {
            m_states[name] = state;
        }

        void TransitionTo(const std::string& name) {
            if (m_currentStateName == name) {
                m_wasAnimationSwitched = false;
                return;
            }

            auto it = m_states.find(name);
            if (it != m_states.end()) {
                if (m_currentState) {
                    m_currentState->exit();
                }
                m_currentState = it->second;
                m_currentStateName = name;
                m_currentState->entry();
                m_wasAnimationSwitched = true;
            }
        }

        void UpdateParameters(float speed, glm::vec2 direction) {
            m_context.speed = speed;
            m_context.direction = direction;
        }

        void Update(float deltaTime) {
            if (m_currentState) {
                m_currentState->update(deltaTime, m_context);
            }
        }

        AnimationStateOutput GetOutput() const {
            if (m_currentState) {
                return m_currentState->output;
            }
            return AnimationStateOutput();
        }

        std::shared_ptr<AnimationState> GetCurrentState() const {
            return m_currentState;
        }

        bool WasAnimationSwitched() {
            bool result = m_wasAnimationSwitched;
            m_wasAnimationSwitched = false;
            return result;
        }

        bool DidLoopThisFrame() const {
            return m_currentState ? m_currentState->GetLoopedThisFrame() : false;
        }

        glm::vec3 GetClipStartRootPos() const {
            return m_currentState ? m_currentState->GetClipStartRootPos() : glm::vec3(0.0f);
        }

        glm::vec3 GetClipEndRootPos() const {
            return m_currentState ? m_currentState->GetClipEndRootPos() : glm::vec3(0.0f);
        }
    };
}

#endif