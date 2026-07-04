#include "../../../include/scene/systems/PhysicsSystem.h"
#include "../../../include/scene/components/ECSManager.h"
#include "../../../include/globals.h"
#include <cmath>
#include <algorithm>
#include <iostream>

namespace physics {

    constexpr float TERMINAL_VELOCITY = 50.0f;
    constexpr float SLIDING_THRESHOLD = 0.0001f;

    void PhysicsSystem::update(scene::components::ECSManager& ecs, float dt) {
    }

    bool PhysicsSystem::IsFaceExposed(int x, int y, int z, const glm::vec3 &normal, std::shared_ptr<voxel::Grid> grid) {
        int nx = x + static_cast<int>(normal.x);
        int ny = y + static_cast<int>(normal.y);
        int nz = z + static_cast<int>(normal.z);

        if (ny < 0 || ny >= 256) return true;

        auto chunk = grid->GetChunk(nx >> 4, nz >> 4);
        if (!chunk) return true;

        return chunk->GetVoxel(nx & 15, ny, nz & 15) == voxel::VoxelType::AIR;
    }

    bool PhysicsSystem::IsPositionClear(const glm::vec3 &testPos, const scene::components::PhysicsComponent &comp,
                                        std::shared_ptr<voxel::Grid> grid) {
        for (const auto &hb: comp.hitboxes) {
            glm::vec3 testMin = testPos + hb.currentMin;
            glm::vec3 testMax = testPos + hb.currentMax;

            int minX = static_cast<int>(std::floor(testMin.x));
            int maxX = static_cast<int>(std::floor(testMax.x));
            int minY = static_cast<int>(std::floor(testMin.y));
            int maxY = static_cast<int>(std::floor(testMax.y));
            int minZ = static_cast<int>(std::floor(testMin.z));
            int maxZ = static_cast<int>(std::floor(testMax.z));

            for (int x = minX; x <= maxX; ++x) {
                for (int z = minZ; z <= maxZ; ++z) {
                    for (int y = minY; y <= maxY; ++y) {
                        if (grid->IsSolid(x, y, z)) {
                            return false;
                        }
                    }
                }
            }
        }
        return true;
    }

    bool PhysicsSystem::TryStepUp(glm::vec3 &pos, const glm::vec3 &remainingMove,
                                  const scene::components::PhysicsComponent &comp,
                                  std::shared_ptr<voxel::Grid> grid) {
        glm::vec3 horizontalMove = remainingMove;
        horizontalMove.y = 0.0f;

        if (glm::length(horizontalMove) < SLIDING_THRESHOLD) {
            return false;
        }

        for (int stepUp = 0; stepUp <= 3; ++stepUp) {
            glm::vec3 testPos = pos + glm::vec3(0.0f, static_cast<float>(stepUp), 0.0f);

            if (IsPositionClear(testPos, comp, grid)) {
                pos = testPos - glm::vec3(0,1,0);
                return true;
            }
        }
        return false;
    }

    float PhysicsSystem::GetTerrainHeightAtXZ(std::shared_ptr<voxel::Grid> grid, glm::vec3 pos) {
        int x = static_cast<int>(std::floor(pos.x));
        int z = static_cast<int>(std::floor(pos.z));

        int cx = (x >= 0) ? x / 16 : (x - 15) / 16;
        int cz = (z >= 0) ? z / 16 : (z - 15) / 16;

        auto chunk = grid->GetChunk(cx, cz);
        if (!chunk) return 0.0f;

        int localX = ((x % 16) + 16) % 16;
        int localZ = ((z % 16) + 16) % 16;

        for (int y = Y_CHUNK_SIZE - 1; y >= 0; --y) {
            int idx = chunk->GetIndex(localX, y, localZ);
            if (chunk->getData()[idx] != voxel::VoxelType::AIR) {
                return static_cast<float>(y + 1);
            }
        }
        return 0.0f;
    }

    void PhysicsSystem::step(scene::components::PhysicsComponent *components, int count,
                             std::unique_ptr<GameObject> *gameObjects,
                             std::shared_ptr<voxel::Grid> grid,
                             float dt)
    {
        for (int i = 0; i < count; ++i) {
            auto &comp = components[i];
            if (comp.getEntityId() == -1) continue;

            if (comp.model && !comp.hitboxes.empty()) {
                const auto& boneMatrices = comp.model->GetFinalBoneMatrices();
                if (!boneMatrices.empty()) {
                    comp.localTotalMin = glm::vec3(1e10f);
                    comp.localTotalMax = glm::vec3(-1e10f);
                    const float modelScale = 0.01f;

                    for (auto& hb : comp.hitboxes) {
                        if (hb.boneIndex >= (int)boneMatrices.size()) continue;

                        const glm::mat4& m = boneMatrices[hb.boneIndex];
                        glm::vec3 corners[8] = {
                                glm::vec3(m * glm::vec4(hb.localMin.x, hb.localMin.y, hb.localMin.z, 1.0f)),
                                glm::vec3(m * glm::vec4(hb.localMax.x, hb.localMin.y, hb.localMin.z, 1.0f)),
                                glm::vec3(m * glm::vec4(hb.localMin.x, hb.localMax.y, hb.localMin.z, 1.0f)),
                                glm::vec3(m * glm::vec4(hb.localMax.x, hb.localMax.y, hb.localMin.z, 1.0f)),
                                glm::vec3(m * glm::vec4(hb.localMin.x, hb.localMin.y, hb.localMax.z, 1.0f)),
                                glm::vec3(m * glm::vec4(hb.localMax.x, hb.localMin.y, hb.localMax.z, 1.0f)),
                                glm::vec3(m * glm::vec4(hb.localMin.x, hb.localMax.y, hb.localMax.z, 1.0f)),
                                glm::vec3(m * glm::vec4(hb.localMax.x, hb.localMax.y, hb.localMax.z, 1.0f))
                        };

                        hb.currentMin = glm::vec3(1e10f);
                        hb.currentMax = glm::vec3(-1e10f);

                        for (auto& c : corners) {
                            c *= modelScale;
                            hb.currentMin = glm::min(hb.currentMin, c);
                            hb.currentMax = glm::max(hb.currentMax, c);
                        }
                        comp.localTotalMin = glm::min(comp.localTotalMin, hb.currentMin);
                        comp.localTotalMax = glm::max(comp.localTotalMax, hb.currentMax);
                    }
                    if (comp.localTotalMin.y > 0.0f) comp.localTotalMin.y = 0.0f;
                }
            }

            GameObject *obj = gameObjects[comp.getEntityId()].get();
            glm::vec3 oldPos = obj->getPosition();



            glm::vec3 vectorToMove = comp.velocity * dt;
            glm::vec3 proposedPos = oldPos + vectorToMove;
            auto modelAnimation = comp.model;
            modelAnimation->ClearFootAdjustments();

            comp.onGround = false;

            if (!IsPositionClear(proposedPos, comp, grid)) {
                if (TryStepUp(proposedPos, vectorToMove, comp, grid)) {
                    comp.onGround = true;
                    comp.velocity.y = 0.0f;
                } else {
                    glm::vec3 verticalTestPos = oldPos + glm::vec3(0.0f, vectorToMove.y, 0.0f);
                    if (!IsPositionClear(verticalTestPos, comp, grid) && vectorToMove.y <= 0.0f) {
                        comp.onGround = true;
                        comp.velocity.y = 0.0f;
                    }

                    proposedPos = oldPos;
                    if (!comp.onGround) {
                        comp.velocity.x = 0.0f;
                        comp.velocity.z = 0.0f;
                        if (vectorToMove.y > 0.0f) {
                            comp.velocity.y = 0.0f;
                        }
                    }
                }
            }

            if (!comp.onGround) {
//                std::cout << "[Physics] Entity ID: " << comp.getEntityId() << " is NOT on the ground!\n";
                comp.velocity.y += GRAVITY.y * dt;
                if (comp.velocity.y < -TERMINAL_VELOCITY) comp.velocity.y = -TERMINAL_VELOCITY;
            }

            glm::mat4 proposedModelMatrix = obj->getModelMatrix();
            proposedModelMatrix[3] = glm::vec4(proposedPos, 1.0f);

            bool feetWouldBlock = modelAnimation->WouldFeetHitTerrain(
                    proposedModelMatrix,
                    modelAnimation->GetFinalBoneMatrices(),
                    3.0f,
                    [this, grid](glm::vec3 footPos) {
                        return GetTerrainHeightAtXZ(grid, footPos);
                    }
            );

            if (feetWouldBlock) {
                proposedPos = oldPos;
                comp.velocity.y = 0;
                proposedModelMatrix = obj->getModelMatrix();
                proposedModelMatrix[3] = glm::vec4(proposedPos, 1.0f);
            }

            auto adjustedBones = modelAnimation->AdjustBonesForTerrainCollisionIK(
                    modelAnimation->GetFinalBoneMatrices(),
                    proposedModelMatrix,
                    100.0f,
                    [this, grid](glm::vec3 footPos) {
                        return GetTerrainHeightAtXZ(grid, footPos);
                    }
            );

            modelAnimation->SetFootAdjustments(adjustedBones);
            obj->setPosition(proposedPos);
//            if (comp.onGround){
                float friction = std::pow(comp.frictionCoefficient, dt * 60.0f);
                comp.velocity *= glm::vec3(friction, 1.0f, friction);
//            }

            grid->UpdateEntitySpatialPosition(comp.getEntityId(), oldPos, proposedPos);
        }
    }
}