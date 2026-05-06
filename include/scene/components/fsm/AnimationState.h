#ifndef QUESTFARERGAMEENGINE_ANIMATIONSTATE_H
#define QUESTFARERGAMEENGINE_ANIMATIONSTATE_H

#include "../../../animation/Animation.h"
#include <glm/gtx/string_cast.hpp>
#include <string>
#include <algorithm>
#include <vector>
#include <memory>

namespace scene::components::fsm {

    struct AnimationStateOutput {
        glm::vec3 m_rootBonePosition = glm::vec3(0.0f);
        std::vector<glm::mat4> finalBoneMatrices;
        glm::mat4 rootGlobalTransform = glm::mat4(1.0f);
        int rootBoneIndex = -1;
    };

    struct AnimationState {
        AnimationStateOutput output;
        std::vector<std::string> rootBoneNames = {"B-root"};

        bool IsRootBone(const std::string& boneName) const {
            return std::find(rootBoneNames.begin(), rootBoneNames.end(), boneName) != rootBoneNames.end();
        }

        void SetRootBoneNames(const std::vector<std::string>& names) {
            rootBoneNames = names;
        }

        void AddRootBoneName(const std::string& name) {
            if (std::find(rootBoneNames.begin(), rootBoneNames.end(), name) == rootBoneNames.end()) {
                rootBoneNames.push_back(name);
            }
        }

        virtual void entry() {}
        virtual AnimationStateOutput update(float deltaTime) = 0;
        virtual void exit() {}
        virtual ~AnimationState() = default;
    };

    struct SimpleAnimationState : AnimationState {
        std::shared_ptr<Animation> animation;
        float m_currentTime = 0.f;
        bool m_loopedThisFrame = false;

        glm::vec3 m_clipStartRootPos = glm::vec3(0.0f);
        glm::vec3 m_clipEndRootPos = glm::vec3(0.0f);

        void entry() override {
            m_currentTime = 0.f;
            m_loopedThisFrame = false;
            if (animation) {
                output.finalBoneMatrices.assign(animation->GetBoneCount(), glm::mat4(1.0f));

                CalculateBoneTransform(&animation->GetRootNode(), glm::mat4(1.0f), 0.0f);
                m_clipStartRootPos = output.m_rootBonePosition;

                CalculateBoneTransform(&animation->GetRootNode(), glm::mat4(1.0f), animation->GetDuration() - 0.001f);
                m_clipEndRootPos = output.m_rootBonePosition;
            }
            output.m_rootBonePosition = glm::vec3(0.0f);
            output.rootBoneIndex = -1;
        }

        void CalculateBoneTransform(const AssimpNodeData *node, glm::mat4 parentTransform, float animationTime) {
            std::string nodeName = node->name;
            glm::mat4 nodeTransform = node->transformation;

            Bone *bone = animation->FindBone(nodeName);
            if (bone) {
                bone->Update(animationTime);
                nodeTransform = bone->GetLocalTransform();
            }

            glm::mat4 globalTransformation = parentTransform * nodeTransform;
            const auto& boneInfoMap = animation->GetBoneIDMap();

            if (boneInfoMap.find(nodeName) != boneInfoMap.end()) {
                int index = boneInfoMap.at(nodeName).id;
                output.finalBoneMatrices[index] = globalTransformation * boneInfoMap.at(nodeName).offset;

                if (IsRootBone(nodeName)) {
                    output.rootBoneIndex = index;
                    output.m_rootBonePosition = glm::vec3(globalTransformation[3]);
                    output.rootGlobalTransform = globalTransformation;
                }
            }

            for (int i = 0; i < node->childrenCount; i++)
                CalculateBoneTransform(&node->children[i], globalTransformation, animationTime);
        }

        AnimationStateOutput update(float deltaTime) override {
            if (!animation) return output;

            float duration = animation->GetDuration();
            float ticksPerSecond = animation->GetTicksPerSecond();
            float previousAnimTime = fmod(m_currentTime * ticksPerSecond, duration);

            m_currentTime += deltaTime;
            float animationTime = fmod(m_currentTime * ticksPerSecond, duration);

            m_loopedThisFrame = (previousAnimTime > animationTime);

            CalculateBoneTransform(&animation->GetRootNode(), glm::mat4(1.0f), animationTime);
            return output;
        }

        bool GetLoopedThisFrame() const { return m_loopedThisFrame; }
        glm::vec3 GetClipStartRootPos() const { return m_clipStartRootPos; }
        glm::vec3 GetClipEndRootPos() const { return m_clipEndRootPos; }
    };

    struct LocomotionBlendState : AnimationState {
        std::shared_ptr<Animation> walk;
        std::shared_ptr<Animation> run;

        float m_speed = 0.f;
        float m_phase = 0.f;
        bool m_loopedThisFrame = false;

        glm::vec3 m_walkRootPos = glm::vec3(0.0f);
        glm::vec3 m_runRootPos = glm::vec3(0.0f);

        glm::vec3 m_clipStartRootPos = glm::vec3(0.0f);
        glm::vec3 m_clipEndRootPos = glm::vec3(0.0f);

        void entry() override {
            m_phase = 0.f;
            m_loopedThisFrame = false;
            m_walkRootPos = m_runRootPos = glm::vec3(0.0f);
            size_t maxBones = std::max(walk->GetBoneCount(), run->GetBoneCount());
            output.finalBoneMatrices.assign(maxBones, glm::mat4(1.0f));
        }

        void calculatePositions() {
            size_t maxBones = std::max(walk->GetBoneCount(), run->GetBoneCount());
            std::vector<glm::mat4> tempBones(maxBones, glm::mat4(1.0f));

            glm::vec3 walkStart(0.0f), runStart(0.0f);
            CalculateBoneTransform(walk, &walk->GetRootNode(), glm::mat4(1.0f), 0.0f, tempBones, walkStart);
            CalculateBoneTransform(run,  &run->GetRootNode(),  glm::mat4(1.0f), 0.0f, tempBones, runStart);
            m_clipStartRootPos = glm::mix(walkStart, runStart, m_speed);

            // Tick-based epsilon to prevent 0.0 duration crashes
            float walkEndT = walk->GetDuration() - (1.0f / std::max(walk->GetTicksPerSecond(), 1.0f));
            float runEndT  = run->GetDuration()  - (1.0f / std::max(run->GetTicksPerSecond(), 1.0f));

            glm::vec3 walkEnd(0.0f), runEnd(0.0f);
            CalculateBoneTransform(walk, &walk->GetRootNode(), glm::mat4(1.0f), walkEndT, tempBones, walkEnd);
            CalculateBoneTransform(run,  &run->GetRootNode(),  glm::mat4(1.0f), runEndT,  tempBones, runEnd);
            m_clipEndRootPos = glm::mix(walkEnd, runEnd, m_speed);
        }

        AnimationStateOutput update(float deltaTime) override {
            calculatePositions();

            float walkDurationSec = walk->GetDuration() / walk->GetTicksPerSecond();
            float runDurationSec  = run->GetDuration()  / run->GetTicksPerSecond();
            float blendedDurationSec = glm::mix(walkDurationSec, runDurationSec, m_speed);

            m_phase += deltaTime / blendedDurationSec;

            if (m_phase >= 1.0f) {
                m_phase = fmod(m_phase, 1.0f);
                m_loopedThisFrame = true;
            } else {
                m_loopedThisFrame = false;
            }

            float walkAnimTime = m_phase * walk->GetDuration();
            float runAnimTime  = m_phase * run->GetDuration();

            std::vector<glm::mat4> bonesWalk(output.finalBoneMatrices.size(), glm::mat4(1.0f));
            std::vector<glm::mat4> bonesRun (output.finalBoneMatrices.size(), glm::mat4(1.0f));

            m_walkRootPos = m_runRootPos = glm::vec3(0.0f);

            CalculateBoneTransform(walk, &walk->GetRootNode(), glm::mat4(1.0f), walkAnimTime, bonesWalk, m_walkRootPos);
            CalculateBoneTransform(run,  &run->GetRootNode(),  glm::mat4(1.0f), runAnimTime,  bonesRun,  m_runRootPos);

            float t = m_speed;
            output.m_rootBonePosition = glm::mix(m_walkRootPos, m_runRootPos, t);

            CalculateBoneTransformNoRM(walk, &walk->GetRootNode(), glm::mat4(1.0f), walkAnimTime, bonesWalk);
            CalculateBoneTransformNoRM(run,  &run->GetRootNode(),  glm::mat4(1.0f), runAnimTime,  bonesRun);

            for (size_t i = 0; i < output.finalBoneMatrices.size(); i++) {
                output.finalBoneMatrices[i] = bonesWalk[i] * (1.0f - t) + bonesRun[i] * t;
            }

            return output;
        }

        glm::vec3 GetClipStartRootPos() const { return m_clipStartRootPos; }
        glm::vec3 GetClipEndRootPos()   const { return m_clipEndRootPos;   }
        bool      GetLoopedThisFrame()  const { return m_loopedThisFrame;  }
        void      SetSpeed(float speed)       { m_speed = glm::clamp(speed, 0.0f, 1.0f); }
        float     getSpeed()            const { return m_speed; }

    private:
        void CalculateBoneTransform(
                std::shared_ptr<Animation> animation,
                const AssimpNodeData *node,
                glm::mat4 parentTransform,
                float animationTime,
                std::vector<glm::mat4> &outMatrices,
                glm::vec3 &outRootPos) {

            std::string nodeName = node->name;
            glm::mat4 nodeTransform = node->transformation;

            Bone *bone = animation->FindBone(nodeName);
            if (bone) {
                bone->Update(animationTime);
                nodeTransform = bone->GetLocalTransform();
            }

            glm::mat4 globalTransformation = parentTransform * nodeTransform;
            const auto& boneInfoMap = animation->GetBoneIDMap();

            if (boneInfoMap.find(nodeName) != boneInfoMap.end()) {
                int index = boneInfoMap.at(nodeName).id;
                outMatrices[index] = globalTransformation * boneInfoMap.at(nodeName).offset;
            }

            if (IsRootBone(nodeName)) {
                outRootPos = glm::vec3(globalTransformation[3]);
            }

            for (int i = 0; i < node->childrenCount; i++) {
                CalculateBoneTransform(animation, &node->children[i], globalTransformation, animationTime, outMatrices, outRootPos);
            }
        }

        void CalculateBoneTransformNoRM(
                std::shared_ptr<Animation> animation,
                const AssimpNodeData *node,
                glm::mat4 parentTransform,
                float animationTime,
                std::vector<glm::mat4> &outMatrices) {

            std::string nodeName = node->name;
            glm::mat4 nodeTransform = node->transformation;

            Bone *bone = animation->FindBone(nodeName);
            if (bone) {
                bone->Update(animationTime);
                nodeTransform = bone->GetLocalTransform();
                if (std::find(rootBoneNames.begin(), rootBoneNames.end(), nodeName) != rootBoneNames.end()) {
                    nodeTransform[3][0] = 0.0f;
                    nodeTransform[3][1] = 0.0f;
                    nodeTransform[3][2] = 0.0f;
                }
            }

            glm::mat4 globalTransformation = parentTransform * nodeTransform;
            const auto& boneInfoMap = animation->GetBoneIDMap();

            if (boneInfoMap.find(nodeName) != boneInfoMap.end()) {
                int index = boneInfoMap.at(nodeName).id;
                outMatrices[index] = globalTransformation * boneInfoMap.at(nodeName).offset;
            }

            for (int i = 0; i < node->childrenCount; i++) {
                CalculateBoneTransformNoRM(animation, &node->children[i], globalTransformation, animationTime, outMatrices);
            }
        }
    };
}

#endif