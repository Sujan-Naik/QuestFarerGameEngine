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
            m_RootGlobalTransform = glm::mat4(1.0f);

            m_FinalBoneMatrices.reserve(100);
            for (int i = 0; i < 100; i++)
                m_FinalBoneMatrices.push_back(glm::mat4(1.0f));
        }

        void UpdateAnimation(float dt) {
            m_DeltaTime = dt;
            if (m_CurrentAnimation) {
                m_LastFrameTime = m_CurrentTime;
                m_CurrentTime += m_CurrentAnimation->GetTicksPerSecond() * dt;

                float duration = m_CurrentAnimation->GetDuration();

                float clampedTime = fmod(m_CurrentTime, duration);
                m_LoopDetected = (clampedTime < m_LastFrameTime) && (m_CurrentTime >= duration);

                if (m_LoopDetected) {
                    m_CurrentTime = clampedTime;
                }

                CalculateBoneTransform(&m_CurrentAnimation->GetRootNode(), glm::mat4(1.0f), clampedTime);
            }
        }

        void PlayAnimation(std::shared_ptr<Animation> pAnimation) {
            m_CurrentAnimation = std::move(pAnimation);
            m_CurrentTime = 0.0f;
            m_LastFrameTime = 0.0f;
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

                // NEW: Store the PURE global transform of the root bone (Index 0)
                // This gives us unpolluted root motion data.
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

        // NEW: Return pure translation without offset matrix mapping
        glm::vec3 GetRootBonePosition() const {
            return glm::vec3(m_RootGlobalTransform[3]);
        }

        // NEW: Return pure rotation without offset matrix mapping
        glm::quat GetRootBoneRotation() const {
            return glm::quat_cast(m_RootGlobalTransform);
        }

        bool DidLoopThisFrame() const { return m_LoopDetected; }
        std::shared_ptr<Animation> GetCurrentAnimation() const { return m_CurrentAnimation; }

    private:
        std::vector<glm::mat4> m_FinalBoneMatrices;
        std::shared_ptr<Animation> m_CurrentAnimation;
        glm::mat4 m_RootGlobalTransform; // NEW: Track pure root transform
        float m_CurrentTime;
        float m_LastFrameTime;
        float m_DeltaTime;
        bool m_LoopDetected;
    };
}
#endif //QUESTFARERGAMEENGINE_ANIMATOR_H