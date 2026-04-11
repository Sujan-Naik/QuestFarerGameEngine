#ifndef QUESTFARERGAMEENGINE_ANIMATOR_H
#define QUESTFARERGAMEENGINE_ANIMATOR_H

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "Animation.h"
#include <map>
#include <vector>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>

namespace animation {
    class Animator {
    public:
        Animator(std::unique_ptr<Animation> animation = nullptr) {
            m_CurrentTime = 0.0;
            m_CurrentAnimation = std::move(animation);
            m_LastFrameTime = 0.0;
            m_LoopDetected = false;
            m_AnimationSwitched = false;
            m_RootGlobalTransform = glm::mat4(1.0f);

            m_FinalBoneMatrices.reserve(100);
            for (int i = 0; i < 100; i++)
                m_FinalBoneMatrices.push_back(glm::mat4(1.0f));
        }

        void UpdateAnimation(float dt) {
            m_DeltaTime = dt;
            m_LoopDetected = false;
            m_AnimationSwitched = false;

            if (!m_CurrentAnimation) return;

            float ticksPerSecond = m_CurrentAnimation->GetTicksPerSecond();

            float duration = m_CurrentAnimation->GetDuration(); // this is duration in ticks
            float previousTime = m_CurrentTime;
            m_CurrentTime += ticksPerSecond * dt;

            // Temporary: print every frame to confirm values are sane
            std::cout << "time (ticks wise)=" << m_CurrentTime
                      << " duration (finish time)=" << duration
                      << " tps =" << ticksPerSecond << std::endl;

            m_LoopDetected = (previousTime < duration) && (m_CurrentTime >= duration);
            float clampedTime = fmod(m_CurrentTime, duration);

            if (m_LoopDetected) {
                m_CurrentTime = clampedTime;
                if (m_NextAnimation) {
                    CalculateBoneTransform(&m_CurrentAnimation->GetRootNode(), glm::mat4(1.0f), clampedTime);

                    m_CurrentAnimation = std::move(m_NextAnimation);

                    m_NextAnimation = nullptr;
                    m_CurrentTime = 0.0f;
                    m_LastFrameTime = 0.0f;
                    clampedTime = 0.0f;
//                    m_AnimationSwitched = true;
                }
            } else if (m_CurrentAnimation) {
                CalculateBoneTransform(&m_CurrentAnimation->GetRootNode(), glm::mat4(1.0f), clampedTime);
            }
        }

        // Queue an animation to play after the current one finishes its loop.
        // Replaces any previously queued animation so only the latest request wins.
        void PlayAnimation(std::shared_ptr<Animation> pAnimation) {
            if (!m_CurrentAnimation) {
                m_CurrentAnimation = std::move(pAnimation);
                m_CurrentTime = 0.0f;
                m_LastFrameTime = 0.0f;
                m_AnimationSwitched = true;
            } else {
                // Always overwrite next — no point queuing stale states
                m_NextAnimation = pAnimation;
            }
        }



        // Immediately switches animation regardless of playback position.
        // Only use this for cases where interruption is intentional (e.g. hit stun).
        void ForceAnimation(std::shared_ptr<Animation> pAnimation) {
            m_CurrentAnimation = std::move(pAnimation);
            m_NextAnimation = nullptr;
            m_CurrentTime = 0.0f;
            m_LastFrameTime = 0.0f;
            m_AnimationSwitched = true;
        }

        bool getIsBusy() {
            return m_CurrentAnimation && m_NextAnimation;
        }

        void CalculateBoneTransform(const AssimpNodeData *node, glm::mat4 parentTransform, float animationTime) {
            std::string nodeName = node->name;
            glm::mat4 nodeTransform = node->transformation;

            Bone *Bone = m_CurrentAnimation->FindBone(nodeName);
            if (Bone) {
                Bone->Update(animationTime);
                nodeTransform = Bone->GetLocalTransform();
            }

            glm::mat4 globalTransformation = parentTransform * nodeTransform;

            auto boneInfoMap = m_CurrentAnimation->GetBoneIDMap();


            if (boneInfoMap.find(nodeName) != boneInfoMap.end()) {
                int index = boneInfoMap[nodeName].id;
                glm::mat4 offset = boneInfoMap[nodeName].offset;
                m_FinalBoneMatrices[index] = globalTransformation * offset;

                if (index == 0) {
                    m_RootGlobalTransform = globalTransformation;
                }
            }

            for (int i = 0; i < node->childrenCount; i++)
                CalculateBoneTransform(&node->children[i], globalTransformation, animationTime);
        }

        std::vector<glm::mat4> GetFinalBoneMatrices() {
            return m_FinalBoneMatrices;
        }

        glm::vec3 GetRootBonePosition() const {
            return glm::vec3(m_RootGlobalTransform[3]);
        }

        glm::quat GetRootBoneRotation() const {
            return glm::quat_cast(m_RootGlobalTransform);
        }

        bool DidLoopThisFrame() const { return m_LoopDetected; }

        // True for exactly one tick after an animation transition fires.
        // Used by AnimationModelObject to reset root motion baseline.
        bool WasAnimationSwitched() const { return m_AnimationSwitched; }

        std::shared_ptr<Animation> GetCurrentAnimation() const { return m_CurrentAnimation; }

    private:
        std::vector<glm::mat4> m_FinalBoneMatrices;
        std::shared_ptr<Animation> m_CurrentAnimation;
        std::shared_ptr<Animation> m_NextAnimation;
        glm::mat4 m_RootGlobalTransform;
        float m_CurrentTime;
        float m_LastFrameTime;
        float m_DeltaTime;
        bool m_LoopDetected;
        bool m_AnimationSwitched;
    };
}
#endif //QUESTFARERGAMEENGINE_ANIMATOR_H