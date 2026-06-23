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

namespace scene::components {

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
                attack.entitiesAlreadyHit.clear();
                attack.previousHandPositions.erase(attackerId);
                continue;
            }

            attack.isAttackActive = true;

            if (!ctrl->skeleton || attack.damagingBoneIndex == -1) continue;

            const auto& matrices = ctrl->skeleton->GetFinalBoneMatrices();
            if (matrices.empty() || attack.damagingBoneIndex >= static_cast<int>(matrices.size())) continue;

            glm::mat4 boneMatrix = matrices[attack.damagingBoneIndex];
            const auto& boneInfoMap = ctrl->skeleton->GetBoneInfoMap();

            std::string actualBoneName = "UNKNOWN";
            for (const auto& [name, info] : boneInfoMap) {
                if (info.id == attack.damagingBoneIndex) {
                    actualBoneName = name;
                    break;
                }
            }

            glm::mat4 assetCorrection = glm::mat4(
                    -1.0f,  0.0f,  0.0f,  0.0f,
                    0.0f,  0.0f,  1.0f,  0.0f,
                    0.0f,  1.0f,  0.0f,  0.0f,
                    0.0f,  0.0f,  0.0f,  1.0f
            );

            glm::mat4 worldMatrix = glm::translate(glm::mat4(1.0f), ctrl->transform->position)
                                    * glm::toMat4(ctrl->transform->rotation)
                                    * glm::scale(glm::mat4(1.0f), glm::vec3(0.05f));

            glm::vec3 currentHandPos = glm::vec3(worldMatrix * assetCorrection * boneMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

            bool hasPreviousPos = attack.previousHandPositions.count(attackerId);
            glm::vec3 previousHandPos = hasPreviousPos ? attack.previousHandPositions[attackerId] : currentHandPos;
            attack.previousHandPositions[attackerId] = currentHandPos;

            if (!hasPreviousPos) continue;

            glm::vec3 segment = currentHandPos - previousHandPos;
            float segmentLength = glm::length(segment);
            glm::vec3 segmentDir = (segmentLength > 0.0001f) ? (segment / segmentLength) : glm::vec3(0.0f);

            for (int j = 0; j < healthCount; ++j) {
                HealthComponent& victimHealth = healths[j];
                int victimId = victimHealth.getEntityId();

                if (victimId == -1 || victimHealth.isDead || victimId == attackerId) continue;
                if (attack.entitiesAlreadyHit.count(victimId)) continue;

                PhysicsComponent* victimPhysics = ecs.getPhysicsComponent(victimId);
                if (!victimPhysics || victimPhysics->hitboxes.empty()) continue;

                CharacterControllerComponent* victimCtrl = ecs.getCharacterControllerComponent(victimId);
                if (!victimCtrl || !victimCtrl->transform) continue;

                glm::vec3 victimWorldPos = victimCtrl->transform->position;
                bool isHit = false;
                int hitBoneIndex = -1;
                glm::vec3 hitBoxMin{0.0f}, hitBoxMax{0.0f};

                for (const auto& hb : victimPhysics->hitboxes) {
                    glm::vec3 worldMin = victimWorldPos + hb.currentMin;
                    glm::vec3 worldMax = victimWorldPos + hb.currentMax;

                    float tMin = 0.0f;
                    float tMax = segmentLength;

                    if (segmentLength <= 0.0001f) {
                        if (currentHandPos.x >= worldMin.x - attack.attackRadius && currentHandPos.x <= worldMax.x + attack.attackRadius &&
                            currentHandPos.y >= worldMin.y - attack.attackRadius && currentHandPos.y <= worldMax.y + attack.attackRadius &&
                            currentHandPos.z >= worldMin.z - attack.attackRadius && currentHandPos.z <= worldMax.z + attack.attackRadius) {
                            isHit = true;
                            hitBoneIndex = hb.boneIndex;
                            hitBoxMin = worldMin;
                            hitBoxMax = worldMax;
                            break;
                        }
                        continue;
                    }

                    for (int axis = 0; axis < 3; ++axis) {
                        float invD = 1.0f / (segmentDir[axis] == 0.0f ? 1e-6f : segmentDir[axis]);
                        float t0 = ((worldMin[axis] - attack.attackRadius) - previousHandPos[axis]) * invD;
                        float t1 = ((worldMax[axis] + attack.attackRadius) - previousHandPos[axis]) * invD;

                        if (invD < 0.0f) std::swap(t0, t1);

                        tMin = std::max(tMin, t0);
                        tMax = std::min(tMax, t1);

                        if (tMax < tMin) break;
                    }

                    if (tMax >= tMin) {
                        isHit = true;
                        hitBoneIndex = hb.boneIndex;
                        hitBoxMin = worldMin;
                        hitBoxMax = worldMax;
                        break;
                    }
                }

                if (isHit) {
                    victimHealth.currentHealth -= 25.0f;
                    attack.entitiesAlreadyHit.insert(victimId);

//                    std::cout << "[COLLISION REGISTERED]\n"
//                              << "  -> Attacker ID:   " << attackerId << " (" << actualBoneName << ")\n"
//                              << "  -> Victim ID:     " << victimId << " (Hit Bone Index: " << hitBoneIndex << ")\n"
//                              << "  -> Sweep Vector:  From (" << previousHandPos.x << ", " << previousHandPos.y << ", " << previousHandPos.z << ")\n"
//                              << "                    To   (" << currentHandPos.x << ", " << currentHandPos.y << ", " << currentHandPos.z << ")\n"
//                              << "  -> Sweep Length:  " << segmentLength << " units\n"
//                              << "  -> Target Box:    Min(" << hitBoxMin.x << ", " << hitBoxMin.y << ", " << hitBoxMin.z << ")\n"
//                              << "                    Max(" << hitBoxMax.x << ", " << hitBoxMax.y << ", " << hitBoxMax.z << ")\n"
//                              << "  -> Victim Health: " << victimHealth.currentHealth << "\n" << std::endl;

                    if (victimHealth.currentHealth <= 0.0f) {
                        victimHealth.isDead = true;
                    }

                    glm::vec3 pushDirection = glm::normalize(victimWorldPos - ctrl->transform->position);
                    pushDirection.y = 0.0f;
                    victimPhysics->addVelocity(pushDirection * 15.0f);
                }
            }
        }
    }
}