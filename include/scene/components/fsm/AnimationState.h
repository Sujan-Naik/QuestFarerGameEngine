#ifndef QUESTFARERGAMEENGINE_ANIMATIONSTATE_H
#define QUESTFARERGAMEENGINE_ANIMATIONSTATE_H

#include "../../../animation/Animation.h"
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <string>
#include <algorithm>
#include <vector>
#include <memory>

using namespace animation;

namespace scene::components::fsm {

    struct BlendParameterContext {
        float speed = 0.0f;
        glm::vec2 direction = glm::vec2(0.0f);
    };

    struct AnimationStateOutput {
        glm::vec3 m_rootBonePosition = glm::vec3(0.0f);
        std::vector<glm::mat4> finalBoneMatrices;
        glm::mat4 rootGlobalTransform = glm::mat4(1.0f);
        int rootBoneIndex = -1;
    };

    class BlendNode {
    public:
        virtual ~BlendNode() = default;
        virtual void Evaluate(float phase, const BlendParameterContext& ctx,
                              std::vector<glm::mat4>& outLocalMatrices,
                              glm::vec3& outRootPos, bool ignoreRootMotion) = 0;
        virtual float GetDurationSec(const BlendParameterContext& ctx) = 0;
        virtual size_t GetMaxBoneCount() = 0;
    };

    class ClipNode : public BlendNode {
    public:
        std::shared_ptr<Animation> animation;

        ClipNode(std::shared_ptr<Animation> anim) : animation(anim) {}

        float GetDurationSec(const BlendParameterContext& ctx) override {
            if (!animation) return 0.01f;
            return animation->GetDuration() / std::max(animation->GetTicksPerSecond(), 1.0f);
        }

        size_t GetMaxBoneCount() override {
            return animation ? animation->GetBoneCount() : 0;
        }

        void Evaluate(float phase, const BlendParameterContext& ctx,
                      std::vector<glm::mat4>& outLocalMatrices,
                      glm::vec3& outRootPos, bool ignoreRootMotion) override {
            if (!animation) return;
            float animationTime = phase * animation->GetDuration();
            CalculateBoneTransform(&animation->GetRootNode(), glm::mat4(1.0f), animationTime, outLocalMatrices, outRootPos, ignoreRootMotion);
        }

    private:
        void CalculateBoneTransform(const AssimpNodeData* node, glm::mat4 parentTransform, float animationTime,
                                    std::vector<glm::mat4>& outMatrices, glm::vec3& outRootPos, bool ignoreRootMotion) {
            std::string nodeName = node->name;
            glm::mat4 nodeTransform = node->transformation;

            Bone* bone = animation->FindBone(nodeName);
            if (bone) {
                bone->Update(animationTime);
                nodeTransform = bone->GetLocalTransform();
                if (ignoreRootMotion && nodeName == "B-root") {
                    nodeTransform[3][0] = 0.0f;
                    nodeTransform[3][1] = 0.0f;
                    nodeTransform[3][2] = 0.0f;
                }
            }

            glm::mat4 globalTransformation = parentTransform * nodeTransform;
            const auto& boneInfoMap = animation->GetBoneIDMap();

            if (boneInfoMap.find(nodeName) != boneInfoMap.end()) {
                int index = boneInfoMap.at(nodeName).id;
                if (index < outMatrices.size()) {
                    outMatrices[index] = globalTransformation * boneInfoMap.at(nodeName).offset;
                }
                if (nodeName == "B-root") {
                    outRootPos = glm::vec3(globalTransformation[3]);
                }
            }

            for (int i = 0; i < node->childrenCount; i++) {
                CalculateBoneTransform(&node->children[i], globalTransformation, animationTime, outMatrices, outRootPos, ignoreRootMotion);
            }
        }
    };

    class LinearBlendNode : public BlendNode {
    public:
        std::unique_ptr<BlendNode> childA;
        std::unique_ptr<BlendNode> childB;
        float thresholdA = 0.0f;
        float thresholdB = 1.0f;

        LinearBlendNode(std::unique_ptr<BlendNode> a, std::unique_ptr<BlendNode> b, float tA = 0.0f, float tB = 1.0f)
                : childA(std::move(a)), childB(std::move(b)), thresholdA(tA), thresholdB(tB) {}

        float GetDurationSec(const BlendParameterContext& ctx) override {
            float t = glm::clamp((ctx.speed - thresholdA) / (thresholdB - thresholdA), 0.0f, 1.0f);
            return glm::mix(childA->GetDurationSec(ctx), childB->GetDurationSec(ctx), t);
        }

        size_t GetMaxBoneCount() override {
            return std::max(childA->GetMaxBoneCount(), childB->GetMaxBoneCount());
        }

        void Evaluate(float phase, const BlendParameterContext& ctx,
                      std::vector<glm::mat4>& outLocalMatrices,
                      glm::vec3& outRootPos, bool ignoreRootMotion) override {
            float t = glm::clamp((ctx.speed - thresholdA) / (thresholdB - thresholdA), 0.0f, 1.0f);
            std::vector<glm::mat4> matricesA(outLocalMatrices.size(), glm::mat4(1.0f));
            std::vector<glm::mat4> matricesB(outLocalMatrices.size(), glm::mat4(1.0f));
            glm::vec3 posA(0.0f), posB(0.0f);

            childA->Evaluate(phase, ctx, matricesA, posA, ignoreRootMotion);
            childB->Evaluate(phase, ctx, matricesB, posB, ignoreRootMotion);

            outRootPos = glm::mix(posA, posB, t);
            for (size_t i = 0; i < outLocalMatrices.size(); ++i) {
                outLocalMatrices[i] = matricesA[i] * (1.0f - t) + matricesB[i] * t;
                if (!ignoreRootMotion && i == 0) {
                    outLocalMatrices[i][3] = glm::vec4(outRootPos, 1.0f);
                }
            }
        }
    };

    class DirectionalBlendTree2D : public BlendNode {
    public:
        struct BlendElement {
            std::unique_ptr<BlendNode> node;
            glm::vec2 position;
        };
        std::vector<BlendElement> elements;

        void AddNode(std::unique_ptr<BlendNode> node, glm::vec2 pos) {
            elements.push_back({std::move(node), pos});
        }

        size_t GetMaxBoneCount() override {
            size_t maxBones = 0;
            for (const auto& el : elements) {
                maxBones = std::max(maxBones, el.node->GetMaxBoneCount());
            }
            return maxBones;
        }

        float GetDurationSec(const BlendParameterContext& ctx) override {
            auto weights = CalculateWeights(ctx.direction);
            float blendedDuration = 0.0f;
            for (size_t i = 0; i < elements.size(); ++i) {
                blendedDuration += weights[i] * elements[i].node->GetDurationSec(ctx);
            }
            return blendedDuration > 0.01f ? blendedDuration : 0.4f;
        }

        void Evaluate(float phase, const BlendParameterContext& ctx,
                      std::vector<glm::mat4>& outLocalMatrices,
                      glm::vec3& outRootPos, bool ignoreRootMotion) override {
            auto weights = CalculateWeights(ctx.direction);
            size_t boneCount = outLocalMatrices.size();

            std::vector<glm::mat4> totalMatrices(boneCount, glm::mat4(0.0f));
            glm::vec3 totalRootPos(0.0f);

            std::vector<glm::mat4> tempMatrices(boneCount, glm::mat4(1.0f));
            glm::vec3 tempRootPos(0.0f);

            for (size_t i = 0; i < elements.size(); ++i) {
                if (weights[i] <= 0.0001f) continue;

                std::fill(tempMatrices.begin(), tempMatrices.end(), glm::mat4(1.0f));
                tempRootPos = glm::vec3(0.0f);

                elements[i].node->Evaluate(phase, ctx, tempMatrices, tempRootPos, ignoreRootMotion);

                totalRootPos += tempRootPos * weights[i];
                for (size_t b = 0; b < boneCount; ++b) {
                    totalMatrices[b] += tempMatrices[b] * weights[i];
                }
            }
            outRootPos = totalRootPos;
            outLocalMatrices = totalMatrices;

            if (!ignoreRootMotion && boneCount > 0) {
                outLocalMatrices[0][3] = glm::vec4(outRootPos, 1.0f);
            }
        }

    private:
        std::vector<float> CalculateWeights(glm::vec2 inputPos) {
            std::vector<float> weights(elements.size(), 0.0f);
            if (elements.empty()) return weights;

            if (glm::length(inputPos) < 0.01f) {
                weights[0] = 1.0f;
                return weights;
            }

            float totalWeight = 0.0f;
            for (size_t i = 0; i < elements.size(); ++i) {
                float d = glm::distance(inputPos, elements[i].position);
                if (d < 0.001f) {
                    std::fill(weights.begin(), weights.end(), 0.0f);
                    weights[i] = 1.0f;
                    return weights;
                }

                float dotProd = glm::dot(glm::normalize(inputPos), glm::normalize(elements[i].position));
                if (dotProd < 0.0f && glm::length(elements[i].position) > 0.01f) {
                    weights[i] = 0.0f;
                    continue;
                }

                float w = std::max(0.0f, 1.0f - d);
                weights[i] = w;
                totalWeight += w;
            }

            if (totalWeight > 0.0f) {
                for (size_t i = 0; i < weights.size(); ++i) {
                    weights[i] /= totalWeight;
                }
            } else {
                weights[0] = 1.0f;
            }
            return weights;
        }
    };

    class AnimationState {
    public:
        AnimationStateOutput output;
        std::unique_ptr<BlendNode> rootNode;
        std::vector<std::string> rootBoneNames = {"B-root"};

        float m_phase = 0.0f;
        float m_previousPhase = 0.0f;
        bool m_loopedThisFrame = false;

        glm::vec3 m_clipStartRootPos = glm::vec3(0.0f);
        glm::vec3 m_clipEndRootPos = glm::vec3(0.0f);
        glm::vec3 m_rootDeltaThisFrame = glm::vec3(0.0f);

        AnimationState(std::unique_ptr<BlendNode> root) : rootNode(std::move(root)) {}
        virtual ~AnimationState() = default;

        bool IsRootBone(const std::string& boneName) const {
            return std::find(rootBoneNames.begin(), rootBoneNames.end(), boneName) != rootBoneNames.end();
        }

        virtual void entry() {
            m_phase = 0.0f;
            m_previousPhase = 0.0f;
            m_loopedThisFrame = false;
            m_rootDeltaThisFrame = glm::vec3(0.0f);
            if (rootNode) {
                size_t boneCount = rootNode->GetMaxBoneCount();
                output.finalBoneMatrices.assign(boneCount, glm::mat4(1.0f));
                BlendParameterContext dummyCtx;
                std::vector<glm::mat4> tempBones(boneCount, glm::mat4(1.0f));
                rootNode->Evaluate(0.0f, dummyCtx, tempBones, m_clipStartRootPos, false);
                rootNode->Evaluate(0.99f, dummyCtx, tempBones, m_clipEndRootPos, false);
            }
        }

        virtual AnimationStateOutput update(float deltaTime, const BlendParameterContext& ctx) {
            if (!rootNode) return output;

            m_previousPhase = m_phase;
            float duration = rootNode->GetDurationSec(ctx);
            m_phase += deltaTime / duration;

            if (m_phase >= 1.0f) {
                m_phase = fmod(m_phase, 1.0f);
                m_loopedThisFrame = true;
            } else {
                m_loopedThisFrame = false;
            }

            size_t boneCount = rootNode->GetMaxBoneCount();
            if (output.finalBoneMatrices.size() != boneCount) {
                output.finalBoneMatrices.assign(boneCount, glm::mat4(1.0f));
            }

            glm::vec3 rootPosAtCurrentPhase(0.0f);
            rootNode->Evaluate(m_phase, ctx, output.finalBoneMatrices, rootPosAtCurrentPhase, false);

            if (m_loopedThisFrame) {
                glm::vec3 rootPosAtEnd(0.0f);
                glm::vec3 rootPosAtStart(0.0f);
                rootNode->Evaluate(0.999f, ctx, output.finalBoneMatrices, rootPosAtEnd, false);
                rootNode->Evaluate(0.0f, ctx, output.finalBoneMatrices, rootPosAtStart, false);

                glm::vec3 rootPosAtPrev(0.0f);
                rootNode->Evaluate(m_previousPhase, ctx, output.finalBoneMatrices, rootPosAtPrev, false);

                m_rootDeltaThisFrame = (rootPosAtEnd - rootPosAtPrev) + (rootPosAtCurrentPhase - rootPosAtStart);
            } else {
                glm::vec3 rootPosAtPrev(0.0f);
                rootNode->Evaluate(m_previousPhase, ctx, output.finalBoneMatrices, rootPosAtPrev, false);
                m_rootDeltaThisFrame = rootPosAtCurrentPhase - rootPosAtPrev;
            }

            rootNode->Evaluate(m_phase, ctx, output.finalBoneMatrices, output.m_rootBonePosition, true);

            output.rootGlobalTransform = glm::translate(glm::mat4(1.0f), output.m_rootBonePosition);
            output.rootBoneIndex = 0;

            return output;
        }

        virtual void exit() {}

        bool GetLoopedThisFrame() const { return m_loopedThisFrame; }
        glm::vec3 GetClipStartRootPos() const { return m_clipStartRootPos; }
        glm::vec3 GetClipEndRootPos() const { return m_clipEndRootPos; }
        glm::vec3 GetRootDeltaThisFrame() const { return m_rootDeltaThisFrame; }
    };
}

#endif