#include "../../../include/scene/systems/AttackSystem.h"
#include "../../../include/scene/components/ECSManager.h"
#include "../../../include/scene/components/HealthComponent.h"
#include "../../../include/scene/components/AttackComponent.h"
#include "../../../include/scene/components/CharacterControllerComponent.h"
#include "../../../include/scene/components/PhysicsComponent.h"
#include "../../../include/scene/components/fsm/ConcreteStates.h"
#include <cmath>
#include <iostream>
#include <algorithm>
#include <limits>

namespace scene::components {

    static bool intersectSegmentAABB(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& boxMin, const glm::vec3& boxMax) {
        glm::vec3 d = p1 - p0;
        float tMin = 0.0f;
        float tMax = 1.0f;

        for (int i = 0; i < 3; ++i) {
            if (std::abs(d[i]) < 1e-6f) {
                if (p0[i] < boxMin[i] || p0[i] > boxMax[i]) {
                    return false;
                }
            } else {
                float ood = 1.0f / d[i];
                float t1 = (boxMin[i] - p0[i]) * ood;
                float t2 = (boxMax[i] - p0[i]) * ood;

                if (t1 > t2) std::swap(t1, t2);

                tMin = std::max(tMin, t1);
                tMax = std::min(tMax, t2);

                if (tMin > tMax) {
                    return false;
                }
            }
        }
        return true;
    }

    static bool checkCollision(const glm::vec3& p0, const glm::vec3& p1, bool useRaySweep,
                               const glm::vec3& handBoxMin, const glm::vec3& handBoxMax,
                               const glm::vec3& victimBoxMin, const glm::vec3& victimBoxMax) {
        if (useRaySweep) {
            if (intersectSegmentAABB(p0, p1, victimBoxMin, victimBoxMax)) {
                return true;
            }
        }
        bool overlapX = (handBoxMin.x <= victimBoxMax.x) && (handBoxMax.x >= victimBoxMin.x);
        bool overlapY = (handBoxMin.y <= victimBoxMax.y) && (handBoxMax.y >= victimBoxMin.y);
        bool overlapZ = (handBoxMin.z <= victimBoxMax.z) && (handBoxMax.z >= victimBoxMin.z);
        return overlapX && overlapY && overlapZ;
    }

    static glm::mat4 computeHitboxModelMatrix(const Transform* characterTransform,
                                              const glm::mat4& boneMatrix,
                                              const glm::vec3& localMin,
                                              const glm::vec3& localMax) {
        glm::vec3 localCenter = (localMin + localMax) * 0.5f;
        glm::vec3 localSize   = localMax - localMin;

        glm::mat4 hitboxScaleTranslation = glm::mat4(1.0f);
        hitboxScaleTranslation = glm::translate(hitboxScaleTranslation, localCenter);
        hitboxScaleTranslation = glm::scale(hitboxScaleTranslation, localSize);

        return characterTransform->matrix() * boneMatrix * hitboxScaleTranslation;
    }

    static void computeUnitCubeWorldAABB(const glm::mat4& modelMatrix, glm::vec3& outMin, glm::vec3& outMax) {
        glm::vec3 corners[8] = {
                {-0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f, -0.5f},
                {-0.5f,  0.5f, -0.5f}, { 0.5f,  0.5f, -0.5f},
                {-0.5f, -0.5f,  0.5f}, { 0.5f, -0.5f,  0.5f},
                {-0.5f,  0.5f,  0.5f}, { 0.5f,  0.5f,  0.5f}
        };

        outMin = glm::vec3(std::numeric_limits<float>::max());
        outMax = glm::vec3(-std::numeric_limits<float>::max());
        for (int k = 0; k < 8; ++k) {
            glm::vec3 worldCorner = glm::vec3(modelMatrix * glm::vec4(corners[k], 1.0f));
            outMin = glm::min(outMin, worldCorner);
            outMax = glm::max(outMax, worldCorner);
        }
    }

    static void applySkeletalImpulse(CharacterControllerComponent* victimCtrl, int hitBoneIndex,
                                     const glm::vec3& baseTorque, float hitForce, size_t totalBones) {
        const auto& boneInfoMap = victimCtrl->skeleton->GetBoneInfoMap();
        int currentBone = hitBoneIndex;

        std::string initialBoneName = "UNKNOWN";
        for (const auto& [name, info] : boneInfoMap) {
            if (info.id == currentBone) {
                initialBoneName = name;
                break;
            }
        }
        if (initialBoneName.find("shin") != std::string::npos ||
            initialBoneName.find("thigh") != std::string::npos ||
            initialBoneName.find("foot") != std::string::npos ||
            initialBoneName == "B-hips" ||
            initialBoneName == "B-root" || initialBoneName == "Armature") {
            currentBone = victimCtrl->skeleton->GetBoneIndex("B-spine");
        }

        float totalForceMagnitude = glm::length(baseTorque) * hitForce;
        std::cout << "[PHYSICS IMPULSE] Initial Hit Bone: " << initialBoneName
                  << " | Total Force Magnitude: " << totalForceMagnitude << std::endl;

        float forceFalloff = 1.0f;
        while (currentBone != -1 && forceFalloff > 0.05f) {
            if (currentBone >= static_cast<int>(totalBones)) break;

            std::string boneName = "UNKNOWN";
            for (const auto& [name, info] : boneInfoMap) {
                if (info.id == currentBone) {
                    boneName = name;
                    break;
                }
            }

            DynamicBoneState& boneState = victimCtrl->dynamicBones[currentBone];
            boneState.angularVelocity += baseTorque * forceFalloff * hitForce;

            float percentOfForce = forceFalloff * 100.0f;
            float appliedForceMagnitude = totalForceMagnitude * forceFalloff;

            std::cout << "  -> Bone: " << boneName << " (ID: " << currentBone << ") | Force Share: "
                      << percentOfForce << "% (" << appliedForceMagnitude << " / " << totalForceMagnitude << ")" << std::endl;

            forceFalloff *= 0.45f;
            currentBone = victimCtrl->skeleton->GetParentBoneId(currentBone);
        }
    }

    static void applyEpicKnockback(scene::components::CharacterControllerComponent* attackerCtrl,
                                   scene::components::CharacterControllerComponent* victimCtrl,
                                   scene::components::PhysicsComponent* victimPhysics,
                                   int hitBoneIndex,
                                   BoxingPunch punchType) {
        if (!victimCtrl || !attackerCtrl) return;

        // 1. Calculate base impact vectors
        glm::vec3 attackerForward = attackerCtrl->transform->getForward();
        glm::vec3 knockbackDir   = glm::normalize(victimCtrl->transform->position - attackerCtrl->transform->position);
        knockbackDir.y = 0.0f; // Keep initial planar direction flat

        // Default impact parameters
        float impulseForce = 20.0f;
        float verticalLift = 3.5f;
        float torqueForce   = 70.0f;

        // 2. Tailor forces based on punch intent for maximum impact personality
        switch (punchType) {
            case BoxingPunch::Cross:
                // Heavy linear push back
                impulseForce = 32.0f;
                verticalLift = 2.0f;
                torqueForce   = 85.0f;
                break;

            case BoxingPunch::LeftHook:
            case BoxingPunch::RightHook:
                // High rotational spinning torque + wide sweep back
                impulseForce = 22.0f;
                verticalLift = 1.5f;
                torqueForce   = 140.0f;
                break;

            case BoxingPunch::LeftUppercut:
                // High vertical launch!
                impulseForce = 12.0f;
                verticalLift = 12.0f;
                torqueForce   = 90.0f;
                break;

            case BoxingPunch::Jab:
            default:
                // Quick light flinch
                impulseForce = 14.0f;
                verticalLift = 0.5f;
                torqueForce   = 40.0f;
                break;
        }

        // 3. Apply Linear Impulse to Victim Physics Velocity
        if (victimPhysics) {
            // Clear existing Y velocity if launched upwards so gravity doesn't fight the launch
            if (verticalLift > 2.0f) {
                victimPhysics->velocity.y = 0.0f;
            }

            glm::vec3 totalVelocityImpulse = (knockbackDir * impulseForce) + glm::vec3(0.0f, verticalLift, 0.0f);
            victimPhysics->addVelocity(totalVelocityImpulse);
        }

        // 4. Calculate Directional Skeletal Torque
        glm::vec3 victimForward = victimCtrl->transform->getForward();
        glm::vec3 victimRight   = glm::cross(victimForward, glm::vec3(0.0f, 1.0f, 0.0f));

        float rightDot = glm::dot(attackerForward, victimRight);
        float upDot    = attackerForward.y;

        glm::vec3 baseTorque(0.0f);
        baseTorque.y = -rightDot * glm::radians(45.0f); // Spin on Y axis based on side hit
        baseTorque.x = (upDot > 0.2f) ? -glm::radians(30.0f) : -glm::radians(20.0f); // Pitch back

        // 5. Apply Skeletal Impulse cascade
        size_t totalBones = victimCtrl->skeleton ? victimCtrl->skeleton->GetFinalBoneMatrices().size() : 0;
        applySkeletalImpulse(victimCtrl, hitBoneIndex, baseTorque, torqueForce, totalBones);
    }

    void AttackSystem::update(ECSManager& ecs, float dt) {
        AttackComponent* attacks = ecs.getAttackComponentsDense();
        int attackCount = ecs.getAttackComponentsAmount();

        HealthComponent* healths = ecs.getHealthComponentsDense();
        int healthCount = ecs.getHealthComponentsAmount();

        for (int i = 0; i < attackCount; ++i) {
            AttackComponent& attack = attacks[i];
            int attackerId = attack.getEntityId();
            if (attackerId == -1) continue;

            CharacterControllerComponent* ctrl = ecs.getCharacterControllerComponent(attackerId);
            if (!ctrl || !ctrl->fsm || !ctrl->transform) continue;

            auto currentState = ctrl->fsm->GetCurrentState();
            auto punchState = std::dynamic_pointer_cast<fsm::PunchState>(currentState);

            if (!punchState) {
                attack.isAttackActive = false;
                attack.damagingBoneIndex = -1;
                attack.entitiesAlreadyHit.clear();
                attack.previousHandPositions.clear();
                continue;
            }

            if (ctrl->skeleton) {
                std::string targetBoneName = "B-hand.L";
                switch (ctrl->m_activePunchIntent) {
                    case BoxingPunch::Jab:
                    case BoxingPunch::LeftHook:
                    case BoxingPunch::LeftUppercut:
                        targetBoneName = "B-hand.L";
                        break;
                    case BoxingPunch::Cross:
                    case BoxingPunch::RightHook:
                        targetBoneName = "B-hand.R";
                        break;
                    default:
                        targetBoneName = "B-hand.L";
                        break;
                }
                attack.damagingBoneIndex = ctrl->skeleton->GetBoneIndex(targetBoneName);
            }

            if (!ctrl->skeleton || attack.damagingBoneIndex == -1) continue;

            const auto& attackerMatrices = ctrl->skeleton->GetFinalBoneMatrices();
            if (attackerMatrices.empty() || attack.damagingBoneIndex >= static_cast<int>(attackerMatrices.size())) continue;

            PhysicsComponent* attackerPhysics = ecs.getPhysicsComponent(attackerId);
            if (!attackerPhysics) continue;

            const BoneHitbox* attackerHitbox = nullptr;
            for (const auto& hb : attackerPhysics->hitboxes) {
                if (hb.boneIndex == attack.damagingBoneIndex) {
                    attackerHitbox = &hb;
                    break;
                }
            }

            if (!attackerHitbox) continue;

            glm::mat4 attackerHandMatrix = computeHitboxModelMatrix(
                    ctrl->transform,
                    attackerMatrices[attack.damagingBoneIndex],
                    attackerHitbox->localMin,
                    attackerHitbox->localMax
            );

            const auto& attackerBoneInfoMap = ctrl->skeleton->GetBoneInfoMap();
            std::string actualBoneName = "UNKNOWN";
            for (const auto& [name, info] : attackerBoneInfoMap) {
                if (info.id == attack.damagingBoneIndex) {
                    actualBoneName = name;
                    break;
                }
            }

            glm::vec3 currentHandPos = glm::vec3(attackerHandMatrix[3]);
            auto it = attack.previousHandPositions.find(attack.damagingBoneIndex);
            bool hasPreviousPos = (it != attack.previousHandPositions.end());
            glm::vec3 p0 = hasPreviousPos ? it->second : currentHandPos;
            glm::vec3 p1 = currentHandPos;

            attack.previousHandPositions[attack.damagingBoneIndex] = currentHandPos;

            if (!hasPreviousPos) {
                attack.isAttackActive = false;
                continue;
            }

            glm::vec3 handDelta = p1 - p0;
            if (dt > 0.0001f && glm::length(handDelta) > 2.5f) {
                p0 = p1;
                handDelta = glm::vec3(0.0f);
            }

            glm::vec3 characterForward = ctrl->transform->getForward();
            float forwardSpeed = (dt > 0.0001f) ? glm::dot(handDelta / dt, characterForward) : 0.0f;

            const float STRIKE_SPEED_THRESHOLD = 1.8f;
            bool isForwardThrust = (forwardSpeed > STRIKE_SPEED_THRESHOLD);

            if (!isForwardThrust) {
                attack.isAttackActive = false;
                continue;
            }

            attack.isAttackActive = true;
            bool useRaySweep = true;

            glm::vec3 handBoxMin, handBoxMax;
            computeUnitCubeWorldAABB(attackerHandMatrix, handBoxMin, handBoxMax);

            for (int j = 0; j < healthCount; ++j) {
                HealthComponent& victimHealth = healths[j];
                int victimId = victimHealth.getEntityId();

                if (victimId == -1 || victimHealth.isDead || victimId == attackerId) continue;
                if (attack.entitiesAlreadyHit.count(victimId)) continue;

                PhysicsComponent* victimPhysics = ecs.getPhysicsComponent(victimId);
                if (!victimPhysics || victimPhysics->hitboxes.empty()) continue;

                CharacterControllerComponent* victimCtrl = ecs.getCharacterControllerComponent(victimId);
                if (!victimCtrl || !victimCtrl->transform || !victimCtrl->skeleton) continue;

                const auto& victimBonesOriginal = victimCtrl->skeleton->GetFinalBoneMatrices();
                if (victimBonesOriginal.empty()) continue;

                bool isHit = false;
                int hitBoneIndex = -1;

                for (const auto& hb : victimPhysics->hitboxes) {
                    if (hb.boneIndex < 0 || hb.boneIndex >= static_cast<int>(victimBonesOriginal.size())) continue;

                    glm::mat4 victimBoneMatrix = computeHitboxModelMatrix(
                            victimCtrl->transform,
                            victimBonesOriginal[hb.boneIndex],
                            hb.localMin,
                            hb.localMax
                    );

                    glm::vec3 victimBoxMin, victimBoxMax;
                    computeUnitCubeWorldAABB(victimBoneMatrix, victimBoxMin, victimBoxMax);

                    if (checkCollision(p0, p1, useRaySweep, handBoxMin, handBoxMax, victimBoxMin, victimBoxMax)) {
                        isHit = true;
                        hitBoneIndex = hb.boneIndex;
                        break;
                    }
                }

                if (isHit) {
                    victimHealth.currentHealth -= 25.0f;
                    attack.entitiesAlreadyHit.insert(victimId);

                    std::cout << "[HIT] Entity " << attackerId << " (" << actualBoneName
                              << ") hit Entity " << victimId << " (Bone Index: " << hitBoneIndex
                              << ") | HP: " << victimHealth.currentHealth << std::endl;

                    if (victimHealth.currentHealth <= 0.0f) {
                        victimHealth.isDead = true;
                    }

                    glm::vec3 pushDirection = glm::normalize(victimCtrl->transform->position - ctrl->transform->position);
                    pushDirection.y = 0.0f;

                    glm::vec3 fakeSegmentDir = ctrl->transform->getForward();
                    glm::vec3 victimForward = victimCtrl->transform->getForward();
                    glm::vec3 victimRight   = glm::cross(victimForward, glm::vec3(0.0f, 1.0f, 0.0f));

                    float rightDot = glm::dot(fakeSegmentDir, victimRight);
                    float upDot    = fakeSegmentDir.y;

                    float maxTorqueYaw   = glm::radians(35.0f);
                    float maxTorquePitch = glm::radians(25.0f);

                    glm::vec3 baseTorque(0.0f);
                    baseTorque.y = rightDot * maxTorqueYaw;
                    baseTorque.x = (upDot > 0.3f) ? (-upDot * maxTorquePitch) : (-abs(glm::dot(fakeSegmentDir, victimForward)) * 0.3f * maxTorquePitch);

                    float hitForce = 55.0f;
                    applySkeletalImpulse(victimCtrl, hitBoneIndex, baseTorque, hitForce, victimBonesOriginal.size());

                    if (victimPhysics) {
                        victimPhysics->addVelocity(pushDirection * 15.0f);
                    }

                    applyEpicKnockback(ctrl, victimCtrl, victimPhysics, hitBoneIndex, ctrl->m_activePunchIntent);

                }
            }
        }
    }
}