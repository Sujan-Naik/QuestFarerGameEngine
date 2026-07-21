#ifndef QUESTFARERGAMEENGINE_ANIMATIONSTATE_H
#define QUESTFARERGAMEENGINE_ANIMATIONSTATE_H

#include "../../../animation/Animation.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <string>
#include <algorithm>
#include <vector>
#include <memory>
#include <cmath>

using namespace animation;

namespace scene::components::fsm {

    struct SQTransform {
        glm::vec3 translation = glm::vec3(0.0f);
        glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        glm::vec3 scale = glm::vec3(1.0f);

        glm::mat4 ToMatrix() const {
            glm::mat4 m = glm::toMat4(rotation);
            m[0] *= scale.x;
            m[1] *= scale.y;
            m[2] *= scale.z;
            m[3] = glm::vec4(translation, 1.0f);
            return m;
        }

        static SQTransform Blend(const SQTransform& a, const SQTransform& b, float t) {
            SQTransform result;
            result.translation = glm::mix(a.translation, b.translation, t);
            result.rotation = glm::slerp(a.rotation, b.rotation, t);
            result.scale = glm::mix(a.scale, b.scale, t);
            return result;
        }
    };

    struct BlendParameterContext {
        float speed = 0.0f;
        glm::vec2 direction = glm::vec2(0.0f);
    };

    struct AnimationStateOutput {
        glm::vec3 m_rootBonePosition = glm::vec3(0.0f);
        std::vector<SQTransform> localTransforms;
        glm::mat4 rootGlobalTransform = glm::mat4(1.0f);
        int rootBoneIndex = -1;
    };

    class BlendNode {
    public:
        virtual ~BlendNode() = default;
        virtual void Evaluate(float phase, const BlendParameterContext& ctx,
                              std::vector<SQTransform>& outLocalPose,
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
                      std::vector<SQTransform>& outLocalPose,
                      glm::vec3& outRootPos, bool ignoreRootMotion) override {
            if (!animation) return;
            float animationTime = phase * animation->GetDuration();

            const auto& boneInfoMap = animation->GetBoneIDMap();
            for (const auto& [nodeName, boneInfo] : boneInfoMap) {
                int boneId = boneInfo.id;
                if (boneId >= static_cast<int>(outLocalPose.size())) continue;

                Bone* bone = animation->FindBone(nodeName);
                if (bone) {
                    bone->Update(animationTime);
                    glm::mat4 localMat = bone->GetLocalTransform();

                    if (ignoreRootMotion && nodeName == "B-root") {
                        localMat[3][0] = 0.0f;
                        localMat[3][1] = 0.0f;
                        localMat[3][2] = 0.0f;
                    }

                    // Translation
                    outLocalPose[boneId].translation = glm::vec3(localMat[3]);

                    // Scale
                    glm::vec3 scale;
                    scale.x = glm::length(glm::vec3(localMat[0]));
                    scale.y = glm::length(glm::vec3(localMat[1]));
                    scale.z = glm::length(glm::vec3(localMat[2]));
                    outLocalPose[boneId].scale = scale;

                    // Rotation
                    glm::mat3 rotMat;
                    rotMat[0] = glm::vec3(localMat[0]) / (scale.x > 0.00001f ? scale.x : 1.0f);
                    rotMat[1] = glm::vec3(localMat[1]) / (scale.y > 0.00001f ? scale.y : 1.0f);
                    rotMat[2] = glm::vec3(localMat[2]) / (scale.z > 0.00001f ? scale.z : 1.0f);

                    outLocalPose[boneId].rotation = glm::quat_cast(rotMat);

                    if (nodeName == "B-root") {
                        outRootPos = outLocalPose[boneId].translation;
                    }
                }
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
                      std::vector<SQTransform>& outLocalPose,
                      glm::vec3& outRootPos, bool ignoreRootMotion) override {
            float t = glm::clamp((ctx.speed - thresholdA) / (thresholdB - thresholdA), 0.0f, 1.0f);
            std::vector<SQTransform> poseA(outLocalPose.size());
            std::vector<SQTransform> poseB(outLocalPose.size());
            glm::vec3 posA(0.0f), posB(0.0f);

            childA->Evaluate(phase, ctx, poseA, posA, ignoreRootMotion);
            childB->Evaluate(phase, ctx, poseB, posB, ignoreRootMotion);

            outRootPos = glm::mix(posA, posB, t);
            for (size_t i = 0; i < outLocalPose.size(); ++i) {
                outLocalPose[i] = SQTransform::Blend(poseA[i], poseB[i], t);
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
                if (el.node) {
                    maxBones = std::max(maxBones, el.node->GetMaxBoneCount());
                }
            }
            return maxBones;
        }

        float GetDurationSec(const BlendParameterContext& ctx) override {
            auto weights = CalculateWeights(ctx.direction);
            float blendedDuration = 0.0f;
            for (size_t i = 0; i < elements.size(); ++i) {
                if (weights[i] > 0.001f && elements[i].node) {
                    blendedDuration += weights[i] * elements[i].node->GetDurationSec(ctx);
                }
            }
            return blendedDuration > 0.01f ? blendedDuration : 0.4f;
        }

        void Evaluate(float phase, const BlendParameterContext& ctx,
                      std::vector<SQTransform>& outLocalPose,
                      glm::vec3& outRootPos, bool ignoreRootMotion) override {
            auto weights = CalculateWeights(ctx.direction);
            size_t boneCount = outLocalPose.size();

            outRootPos = glm::vec3(0.0f);
            std::vector<glm::vec3> accumTranslations(boneCount, glm::vec3(0.0f));
            std::vector<glm::vec3> accumScales(boneCount, glm::vec3(0.0f));
            std::vector<glm::quat> accumRotations(boneCount, glm::quat(0.0f, 0.0f, 0.0f, 0.0f));

            // Thread-local buffer prevents heap reallocations during evaluation loops
            thread_local std::vector<SQTransform> childPose;
            if (childPose.size() != boneCount) {
                childPose.resize(boneCount);
            }

            bool firstActive = true;

            for (size_t i = 0; i < elements.size(); ++i) {
                float w = weights[i];
                if (w <= 0.001f || !elements[i].node) continue;

                glm::vec3 childRootPos(0.0f);
                elements[i].node->Evaluate(phase, ctx, childPose, childRootPos, ignoreRootMotion);

                outRootPos += childRootPos * w;

                for (size_t b = 0; b < boneCount; ++b) {
                    accumTranslations[b] += childPose[b].translation * w;
                    accumScales[b] += childPose[b].scale * w;

                    glm::quat q = childPose[b].rotation;
                    if (firstActive) {
                        accumRotations[b] = q * w;
                    } else {
                        if (glm::dot(accumRotations[b], q) < 0.0f) {
                            q = -q;
                        }
                        accumRotations[b] += q * w;
                    }
                }
                firstActive = false;
            }

            for (size_t b = 0; b < boneCount; ++b) {
                outLocalPose[b].translation = accumTranslations[b];
                outLocalPose[b].scale = accumScales[b];
                outLocalPose[b].rotation = glm::length(accumRotations[b]) > 0.0001f
                                           ? glm::normalize(accumRotations[b])
                                           : glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
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

                // Culling opposite direction nodes (from reference code)
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
                output.localTransforms.resize(boneCount);
                BlendParameterContext dummyCtx;
                std::vector<SQTransform> tempPose(boneCount);
                rootNode->Evaluate(0.0f, dummyCtx, tempPose, m_clipStartRootPos, false);
                rootNode->Evaluate(0.99f, dummyCtx, tempPose, m_clipEndRootPos, false);
            }
        }

        virtual AnimationStateOutput update(float deltaTime, const BlendParameterContext& ctx) {
            if (!rootNode) return output;

            m_previousPhase = m_phase;
            float duration = rootNode->GetDurationSec(ctx);

            if (ctx.speed > 0.0f || duration > 0.0f) {
                m_phase += deltaTime / duration;
            }

            if (m_phase >= 1.0f) {
                m_phase = std::fmod(m_phase, 1.0f);
                m_loopedThisFrame = true;
            } else {
                m_loopedThisFrame = false;
            }

            size_t boneCount = rootNode->GetMaxBoneCount();
            if (output.localTransforms.size() != boneCount) {
                output.localTransforms.resize(boneCount);
            }

            thread_local std::vector<SQTransform> scratchPose;
            if (scratchPose.size() != boneCount) scratchPose.resize(boneCount);

            glm::vec3 rootPosAtCurrentPhase(0.0f);
            rootNode->Evaluate(m_phase, ctx, scratchPose, rootPosAtCurrentPhase, false);

            if (m_loopedThisFrame) {
                glm::vec3 rootPosAtEnd(0.0f);
                glm::vec3 rootPosAtStart(0.0f);

                rootNode->Evaluate(0.999f, ctx, scratchPose, rootPosAtEnd, false);
                rootNode->Evaluate(0.0f, ctx, scratchPose, rootPosAtStart, false);

                glm::vec3 rootPosAtPrev(0.0f);
                rootNode->Evaluate(m_previousPhase, ctx, scratchPose, rootPosAtPrev, false);

                m_rootDeltaThisFrame = (rootPosAtEnd - rootPosAtPrev) + (rootPosAtCurrentPhase - rootPosAtStart);
            } else {
                glm::vec3 rootPosAtPrev(0.0f);
                rootNode->Evaluate(m_previousPhase, ctx, scratchPose, rootPosAtPrev, false);
                m_rootDeltaThisFrame = rootPosAtCurrentPhase - rootPosAtPrev;
            }

            rootNode->Evaluate(m_phase, ctx, output.localTransforms, output.m_rootBonePosition, true);

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