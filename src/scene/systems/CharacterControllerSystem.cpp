#include "../../../include/scene/systems/CharacterControllerSystem.h"
#include "../../../include/scene/components/CharacterControllerComponent.h"
#include "../../../include/scene/components/PhysicsComponent.h"
#include "../../../include/scene/components/ECSManager.h"
#include "../../../include/globals.h"
#include <glm/gtx/quaternion.hpp>
#include <cmath>
#include <functional>
#include <vector>

namespace scene::components {

    void BuildSkinningMatrices(
            const AssimpNodeData* node,
            const std::vector<SQTransform>& localTransforms,
            const std::map<std::string, BoneInfo>& boneInfoMap,
            const glm::mat4& parentTransform,
            std::vector<glm::mat4>& finalSkinningMatrices
    ) {
        std::string nodeName = node->name;
        glm::mat4 nodeTransform = node->transformation;

        // If this node corresponds to a bone tracked in localTransforms
        auto it = boneInfoMap.find(nodeName);
        if (it != boneInfoMap.end()) {
            int boneId = it->second.id;
            if (boneId >= 0 && boneId < static_cast<int>(localTransforms.size())) {
                nodeTransform = localTransforms[boneId].ToMatrix();
            }
        }

        glm::mat4 globalTransform = parentTransform * nodeTransform;

        if (it != boneInfoMap.end()) {
            int boneId = it->second.id;
            if (boneId >= 0 && boneId < static_cast<int>(finalSkinningMatrices.size())) {
                finalSkinningMatrices[boneId] = globalTransform * it->second.offset;
            }
        }

        for (const auto& child : node->children) {
            BuildSkinningMatrices(&child, localTransforms, boneInfoMap, globalTransform, finalSkinningMatrices);
        }
    }

    static void updatePhysicalBone(float dt, DynamicBoneState &bone, float stiffness, float damping) {
        float angle = glm::angle(bone.dynamicRotation);
        glm::vec3 axis = glm::axis(bone.dynamicRotation);

        if (angle > glm::pi<float>()) {
            angle -= glm::two_pi<float>();
        }

        glm::vec3 displacement = axis * angle;
        glm::vec3 springTorque = -stiffness * displacement - damping * bone.angularVelocity;

        bone.angularVelocity += springTorque * dt;
        glm::quat rotationStep = glm::quat(bone.angularVelocity * dt);
        bone.dynamicRotation = rotationStep * bone.dynamicRotation;
    }

    void CharacterControllerSystem::update(ECSManager &ecs, float dt) {
        CharacterControllerComponent *controllers = ecs.getCharacterControllerComponentsDense();
        int count = ecs.getCharacterControllerComponentsAmount();

        for (int i = 0; i < count; ++i) {
            CharacterControllerComponent &ctrl = controllers[i];
            if (ctrl.getEntityId() == -1 || !ctrl.fsm) continue;

            PhysicsComponent *physics = ecs.getPhysicsComponent(ctrl.getEntityId());
            auto previousState = ctrl.fsm->GetCurrentState();

            bool highPhysicalImpact = false;
            float maxAngularVelocity = 0.0f;
            float maxDisplacementAngle = 0.0f;

            for (auto &[boneId, boneState]: ctrl.dynamicBones) {
                updatePhysicalBone(FIXED_TIMESTEP, boneState, 35.0f, 6.0f);

                float vel = glm::length(boneState.angularVelocity);
                if (vel > maxAngularVelocity) {
                    maxAngularVelocity = vel;
                }

                float angle = glm::angle(boneState.dynamicRotation);
                if (angle > glm::pi<float>()) {
                    angle = std::abs(angle - glm::two_pi<float>());
                }
                if (angle > maxDisplacementAngle) {
                    maxDisplacementAngle = angle;
                }
            }

            if (maxAngularVelocity > 0.5f || maxDisplacementAngle > glm::radians(0.75f)) {
                highPhysicalImpact = true;
            }

            if (ctrl.shouldUpdateAnimations() && !highPhysicalImpact) {
                ctrl.fsm->UpdateParameters(ctrl.m_currentSpeedFactor, ctrl.m_currentDirection);
                ctrl.fsm->Update(FIXED_TIMESTEP);

                auto currentState = ctrl.fsm->GetCurrentState();
                if (currentState) {
                    if (currentState != previousState) {
                        fsm::BlendParameterContext ctx;
                        ctx.speed = ctrl.m_currentSpeedFactor;
                        ctx.direction = ctrl.m_currentDirection;
                        currentState->Update(FIXED_TIMESTEP, ctx);
                    }

                    glm::vec3 delta = currentState->GetRootDeltaThisFrame();

                    // Map Blender local axes to engine world direction vectors
                    glm::vec3 worldDelta = (-ctrl.transform->getForward() * delta.y) +
                                           (ctrl.transform->getRight() * delta.x) +
                                           (ctrl.transform->getUp() * delta.z);

                    if (physics) {
                        physics->setKinematicDisplacement(worldDelta * ( MODEL_SCALE * 100)) ;
                    }
                }
            }

            if (ctrl.skeleton) {
                fsm::AnimationStateOutput asmOutput = ctrl.fsm->GetOutput();
                std::vector<fsm::SQTransform> localPose = asmOutput.localTransforms;

                if (!localPose.empty()) {
                    for (auto &[boneId, boneState] : ctrl.dynamicBones) {
                        if (boneId >= 0 && boneId < static_cast<int>(localPose.size())) {
                            localPose[boneId].rotation = localPose[boneId].rotation * boneState.dynamicRotation;
                        }
                    }

                    std::vector<glm::mat4> finalSkinningMatrices(ctrl.skeleton->GetBoneCount(), glm::mat4(1.0f));

                    BuildSkinningMatrices(
                            ctrl.skeleton->GetRootNode(),
                            localPose,
                            ctrl.skeleton->GetBoneInfoMap(),
                            glm::mat4(1.0f),
                            finalSkinningMatrices
                    );
                    ctrl.skeleton->SetFinalBoneMatrices(finalSkinningMatrices);
                }
            }
        }
    }

    void CharacterControllerSystem::setLocomotionInput(CharacterControllerComponent &ctrl, float forward, float strafe, bool isSprinting) {
        glm::vec2 input(strafe, forward);

        // Prevent diagonal inputs from exceeding magnitude of 1.0
        if (glm::length(input) > 1.0f) {
            input = glm::normalize(input);
        }

        float speedMultiplier = isSprinting ? 1.0f : 0.5f;
        ctrl.m_currentDirection = input * speedMultiplier;
        ctrl.m_currentSpeedFactor = glm::length(ctrl.m_currentDirection);
    }

    void CharacterControllerSystem::triggerJump(ECSManager &ecs, CharacterControllerComponent &ctrl) {
        ctrl.triggerJump();
    }

    void CharacterControllerSystem::triggerPunch(ECSManager &ecs, CharacterControllerComponent &ctrl) {
        ctrl.triggerPunch();
    }

    void CharacterControllerSystem::receiveMessage(CharacterControllerComponent &ctrl, int message) {
        switch (message) {
            case 0: // Idle
                setLocomotionInput(ctrl, 0.0f, 0.0f, false);
                break;
            case 1: // Walk Forward
                setLocomotionInput(ctrl, 1.0f, 0.0f, false);
                break;
            case 2: // Sprint Forward
                setLocomotionInput(ctrl, 1.0f, 0.0f, true);
                break;
            case 3: // Walk Backward
                setLocomotionInput(ctrl, -1.0f, 0.0f, false);
                break;
            case 4: // Sprint Backward
                setLocomotionInput(ctrl, -1.0f, 0.0f, true);
                break;
        }
    }
}