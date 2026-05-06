#ifndef QUESTFARERGAMEENGINE_ANIMATIONFSM_H
#define QUESTFARERGAMEENGINE_ANIMATIONFSM_H

#include <memory>
#include <map>
#include <string>
#include <iostream>
#include "glm/glm.hpp"
#include "../../../animation/Animation.h"
#include "AnimationState.h"

using namespace animation;

namespace scene::components::fsm {

    class AnimationFSM {
    private:
        std::map<std::string, std::shared_ptr<AnimationState>> states;
        std::shared_ptr<AnimationState> currentState;
        std::string currentStateName;
        bool m_LoopDetected = false;
        bool m_AnimationSwitched = false;
        bool m_AnimationSwitchedHadLooped = false;

    public:
        AnimationFSM() : currentState(nullptr) {}

        void RegisterState(const std::string& name, std::shared_ptr<AnimationState> state) {
            states[name] = state;
        }

        void SetState(std::shared_ptr<AnimationState> state) {
            if (currentState) currentState->exit();
            currentState = state;
            currentState->entry();
            m_AnimationSwitched = true;
            m_AnimationSwitchedHadLooped = false;
        }

        void TransitionTo(const std::string& name) {
            auto it = states.find(name);
            if (it != states.end()) {
                if (currentStateName != name) {
                    if (currentState) currentState->exit();
                    currentState = it->second;
                    currentStateName = name;
                    currentState->entry();
                    m_AnimationSwitched = true;
                    m_AnimationSwitchedHadLooped = false;
                }
            } else {
                std::cerr << "ERROR: State '" << name << "' not found in FSM!" << std::endl;
            }
        }

        template<typename T>
        T* GetState(const std::string& name) {
            auto it = states.find(name);
            if (it != states.end()) {
                return dynamic_cast<T*>(it->second.get());
            }
            return nullptr;
        }

        template<typename T>
        T* GetCurrentState() {
            return dynamic_cast<T*>(currentState.get());
        }

        void Update(float deltaTime) {
            if (!currentState) return;

            currentState->update(deltaTime);

            if (auto simpleState = GetCurrentState<SimpleAnimationState>()) {
                m_LoopDetected = simpleState->GetLoopedThisFrame();
            } else if (auto locomotionState = GetCurrentState<LocomotionBlendState>()) {
                m_LoopDetected = locomotionState->GetLoopedThisFrame();
            }

            if (m_AnimationSwitchedHadLooped) {
                PostUpdate();
            }
            m_AnimationSwitchedHadLooped = true;
        }

        void PostUpdate() {
            m_AnimationSwitched = false;
            m_AnimationSwitchedHadLooped = false;
        }

        AnimationStateOutput GetOutput() const {
            return currentState ? currentState->output : AnimationStateOutput();
        }

        glm::vec3 GetClipStartRootPos() const {
            if (auto locomotionState = const_cast<AnimationFSM*>(this)->GetCurrentState<LocomotionBlendState>()) {
                return locomotionState->GetClipStartRootPos();
            }
            if (auto simpleState = const_cast<AnimationFSM*>(this)->GetCurrentState<SimpleAnimationState>()) {
                return simpleState->GetClipStartRootPos();
            }
            return glm::vec3(0.0f);
        }

        glm::vec3 GetClipEndRootPos() const {
            if (auto locomotionState = const_cast<AnimationFSM*>(this)->GetCurrentState<LocomotionBlendState>()) {
                return locomotionState->GetClipEndRootPos();
            }
            if (auto simpleState = const_cast<AnimationFSM*>(this)->GetCurrentState<SimpleAnimationState>()) {
                return simpleState->GetClipEndRootPos();
            }
            return glm::vec3(0.0f);
        }

        bool DidLoopThisFrame() const { return m_LoopDetected; }
        bool WasAnimationSwitched() const { return m_AnimationSwitched; }
        glm::mat4 GetRootGlobalTransform() const { return GetOutput().rootGlobalTransform; }
    };
}
#endif