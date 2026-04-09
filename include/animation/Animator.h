#ifndef QUESTFARERGAMEENGINE_ANIMATOR_H
#define QUESTFARERGAMEENGINE_ANIMATOR_H

#include "glm/glm.hpp"
#include "Animation.h"
#include <map>
#include <vector>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>

namespace animation {
    class Animator {
    public:
        Animator(std::unique_ptr<Animation> animation) {
            m_CurrentTime = 0.0;
            m_CurrentAnimation = std::move(animation);

            m_FinalBoneMatrices.reserve(100);

            for (int i = 0; i < 100; i++)
                m_FinalBoneMatrices.push_back(glm::mat4(1.0f));
        }

        void UpdateAnimation(float dt) {
            m_DeltaTime = dt;
            if (m_CurrentAnimation) {
                m_CurrentTime += m_CurrentAnimation->GetTicksPerSecond() * dt;
                m_CurrentTime = fmod(m_CurrentTime, m_CurrentAnimation->GetDuration());
                CalculateBoneTransform(&m_CurrentAnimation->GetRootNode(), glm::mat4(1.0f));
            }
        }

        void PlayAnimation(std::unique_ptr<Animation> pAnimation) {
            m_CurrentAnimation =  std::move(pAnimation);
            m_CurrentTime = 0.0f;
        }

        void CalculateBoneTransform(const AssimpNodeData *node, glm::mat4 parentTransform) {
            std::string nodeName = node->name;
            glm::mat4 nodeTransform = node->transformation;

            Bone *Bone = m_CurrentAnimation->FindBone(nodeName);

            if (Bone) {
                Bone->Update(m_CurrentTime);
                nodeTransform = Bone->GetLocalTransform();
            }

            glm::mat4 globalTransformation = parentTransform * nodeTransform;

            auto boneInfoMap = m_CurrentAnimation->GetBoneIDMap();
            if (boneInfoMap.find(nodeName) != boneInfoMap.end()) {
                int index = boneInfoMap[nodeName].id;
                glm::mat4 offset = boneInfoMap[nodeName].offset;
                m_FinalBoneMatrices[index] = globalTransformation * offset;
            }

            for (int i = 0; i < node->childrenCount; i++)
                CalculateBoneTransform(&node->children[i], globalTransformation);
        }

        std::vector<glm::mat4> GetFinalBoneMatrices() {
            return m_FinalBoneMatrices;
        }

    private:
        std::vector<glm::mat4> m_FinalBoneMatrices;
        std::unique_ptr<Animation> m_CurrentAnimation;
        float m_CurrentTime;
        float m_DeltaTime;

    };

}

#endif //QUESTFARERGAMEENGINE_ANIMATOR_H
