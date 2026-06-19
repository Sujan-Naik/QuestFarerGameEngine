#include "../../include/physics/PhysicsSystem.h"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/string_cast.hpp"
#include <cmath>
#include <algorithm>
#include <glm/glm.hpp>

namespace physics {

    // Global Constants
    constexpr float TERMINAL_VELOCITY = 50.0f;
    constexpr float SLIDING_THRESHOLD = 0.0001f;

    struct CollisionResult {
        float t = 1.0f;
        glm::vec3 normal = glm::vec3(0.0f);
    };

    /**
     * @brief Checks if a voxel face is exposed to AIR.
     * Prevents "ghost collisions" on internal seams between two solid blocks.
     */
    bool IsFaceExposed(int x, int y, int z, const glm::vec3 &normal, std::shared_ptr<voxel::Grid> grid) {
        int nx = x + static_cast<int>(normal.x);
        int ny = y + static_cast<int>(normal.y);
        int nz = z + static_cast<int>(normal.z);

        // Vertical bounds check (standard world height limits)
        if (ny < 0 || ny >= 256) return true;

        auto it = grid->chunks.find({nx >> 4, nz >> 4});
        if (it == grid->chunks.end()) return true; // Treat unloaded chunks as exposed for safety

        return it->second->GetVoxel(nx & 15, ny, nz & 15) == voxel::VoxelType::AIR;
    }

    /**
     * @brief Standard Swept AABB implementation.
     * @param bMin/Max: Moving object's bounds
     * @param delta: Velocity * dt
     * @param vMin/Max: Static voxel's bounds
     */
    CollisionResult SweptAABB(const glm::vec3 &bMin, const glm::vec3 &bMax, const glm::vec3 &delta,
                              const glm::vec3 &vMin, const glm::vec3 &vMax,
                              int vx, int vy, int vz, std::shared_ptr<voxel::Grid> grid) {

        CollisionResult result;

        // Handle division by zero for stationary axes
        glm::vec3 invDelta;
        invDelta.x = (std::abs(delta.x) < 1e-7f) ? 1e10f : 1.0f / delta.x;
        invDelta.y = (std::abs(delta.y) < 1e-7f) ? 1e10f : 1.0f / delta.y;
        invDelta.z = (std::abs(delta.z) < 1e-7f) ? 1e10f : 1.0f / delta.z;

        glm::vec3 tNear = (vMin - bMax) * invDelta;
        glm::vec3 tFar = (vMax - bMin) * invDelta;

        if (delta.x < 0.0f) std::swap(tNear.x, tFar.x);
        if (delta.y < 0.0f) std::swap(tNear.y, tFar.y);
        if (delta.z < 0.0f) std::swap(tNear.z, tFar.z);

        float tEntry = std::max({tNear.x, tNear.y, tNear.z});
        float tExit = std::min({tFar.x, tFar.y, tFar.z});

        // Check if there is no collision
        if (tEntry > tExit || tExit < 0.0f || tEntry >= 1.0f || tEntry < 0.0f) return result;

        // Determine hit normal based on which axis entered first
        glm::vec3 hitNormal(0.0f);
        if (tNear.x > tNear.y && tNear.x > tNear.z) hitNormal.x = (delta.x < 0.0f ? 1.0f : -1.0f);
        else if (tNear.y > tNear.x && tNear.y > tNear.z) hitNormal.y = (delta.y < 0.0f ? 1.0f : -1.0f);
        else hitNormal.z = (delta.z < 0.0f ? 1.0f : -1.0f);

        // Internal Edge Fix: Ignore collision if the face is buried inside other solid voxels
        if (!IsFaceExposed(vx, vy, vz, hitNormal, grid)) {
            return result;
        }

        result.t = tEntry;
        result.normal = hitNormal;
        return result;
    }

    /**
     * @brief Tests if a position is clear of collisions.
     */
    bool IsPositionClear(const glm::vec3 &testPos, const scene::components::PhysicsComponent &comp,
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
                    auto it = grid->chunks.find({x >> 4, z >> 4});
                    if (it == grid->chunks.end()) continue;

                    for (int y = minY; y <= maxY; ++y) {
                        if (it->second->GetVoxel(x & 15, y, z & 15) != voxel::VoxelType::AIR) {
                            return false;
                        }
                    }
                }
            }
        }
        return true;
    }

    /**
     * @brief Try to step up by testing 1-3 units above collision point
     */
    bool TryStepUp(glm::vec3 &pos, const glm::vec3 &remainingMove,
                   const scene::components::PhysicsComponent &comp,
                   std::shared_ptr<voxel::Grid> grid) {
        glm::vec3 horizontalMove = remainingMove;
        horizontalMove.y = 0.0f;

        if (glm::length(horizontalMove) < SLIDING_THRESHOLD) {
            return false;
        }

        // Try stepping up 1, 2, or 3 units
        for (int stepUp = 0; stepUp <= 3; ++stepUp) {
            glm::vec3 testPos = pos + glm::vec3(0.0f, static_cast<float>(stepUp), 0.0f);

            if (IsPositionClear(testPos, comp, grid)) {
                pos = testPos - glm::vec3(0,1,0);
                return true;
            }
        }
        return false;
    }

    float GetTerrainHeightAtXZ(std::shared_ptr<voxel::Grid> grid, glm::vec3 pos) {
        int x = static_cast<int>(std::floor(pos.x));
        int z = static_cast<int>(std::floor(pos.z));

        // 1. Chunk Lookup
        auto it = grid->chunks.find({x >> 4, z >> 4});
        if (it == grid->chunks.end()) return 0.0f;

        auto& chunk = it->second;

        // 2. Extract local coordinates (0-15)
        int localX = x & 15;
        int localZ = z & 15;

        // 3. Scan down from the actual configured maximum chunk height
        // Using Y_CHUNK_SIZE keeps it perfectly in sync with your engine's globals
        for (int y = Y_CHUNK_SIZE - 1; y >= 0; --y) {
            // Direct, fast index lookup bypassing redundant bounds checks
            int idx = chunk->GetIndex(localX, y, localZ);
            if (chunk->getData()[idx] != voxel::VoxelType::AIR) {
                return static_cast<float>(y + 1); // Surface top
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

            GameObject *obj = gameObjects[comp.getEntityId()].get();
            glm::vec3 pos = obj->getPosition();
            float friction = std::exp(-comp.frictionCoefficient * dt);
            comp.velocity *= glm::vec3(friction, 1, friction);

            glm::vec3 vectorToMove = comp.velocity * dt;
            glm::vec3 proposedPos = pos + vectorToMove;
            auto modelAnimation = comp.model;
            modelAnimation->ClearFootAdjustments();

            // Check horizontal collision
            if (!IsPositionClear(proposedPos, comp, grid)) {
                comp.onGround = true;
                comp.velocity.y = 0;

                // Try stepping up
                if (!TryStepUp(proposedPos, vectorToMove, comp, grid)) {
                    // Step up failed, motion blocked entirely
                    proposedPos = pos;
                }
            } else {
                // Horizontal clear, apply gravity
                comp.velocity.y += GRAVITY.y * dt;
                if (comp.velocity.y < -TERMINAL_VELOCITY) comp.velocity.y = -TERMINAL_VELOCITY;
            }

            // Create the matrix representing the PROPOSED frame state
            glm::mat4 proposedModelMatrix = obj->getModelMatrix();
            proposedModelMatrix[3] = glm::vec4(proposedPos, 1.0f);

            // Check if feet would hit terrain using the proposed matrix
            bool feetWouldBlock = modelAnimation->WouldFeetHitTerrain(
                    proposedModelMatrix,
                    modelAnimation->GetFinalBoneMatrices(),
                    3.0f,
                    [grid](glm::vec3 footPos) {
                        return GetTerrainHeightAtXZ(grid, footPos);
                    }
            );

            if (feetWouldBlock) {
                // Feet can't reach, reject the move and revert matrix to old position
                proposedPos = pos;
                comp.velocity.y = 0;
                proposedModelMatrix = obj->getModelMatrix();
                proposedModelMatrix[3] = glm::vec4(proposedPos, 1.0f);
            }

            // Compute IK using the exact mat4 matrix matching your updated function
            auto adjustedBones = modelAnimation->AdjustBonesForTerrainCollisionIK(
                    modelAnimation->GetFinalBoneMatrices(),
                    proposedModelMatrix,
                    100.0f,
                    [grid](glm::vec3 footPos) {
                        return GetTerrainHeightAtXZ(grid, footPos);
                    }
            );

            modelAnimation->SetFootAdjustments(adjustedBones);
            obj->setPosition(proposedPos);
        }
    }
//
////            float drag = std::exp(-comp.dragCoefficient * dt);
////            comp.velocity.x *= drag;
////            comp.velocity.z *= drag;
//
//            comp.onGround = false;
//
//            glm::vec3 vectorToMove = comp.velocity * dt;
//
////            if (glm::length(vectorToMove) < SLIDING_THRESHOLD) break;
//
//            CollisionResult earliestHit;
//
//            glm::vec3 sweepMin = glm::min(pos + comp.localTotalMin, pos + vectorToMove + comp.localTotalMin);
//            glm::vec3 sweepMax = glm::max(pos + comp.localTotalMax, pos + vectorToMove + comp.localTotalMax);
//
//            int minX = static_cast<int>(std::floor(sweepMin.x));
//            int maxX = static_cast<int>(std::floor(sweepMax.x));
//            int minY = static_cast<int>(std::floor(sweepMin.y));
//            int maxY = static_cast<int>(std::floor(sweepMax.y));
//            int minZ = static_cast<int>(std::floor(sweepMin.z));
//            int maxZ = static_cast<int>(std::floor(sweepMax.z));
//
//            for (int x = minX; x <= maxX; ++x) {
//                for (int z = minZ; z <= maxZ; ++z) {
//                    auto it = grid->chunks.find({x >> 4, z >> 4});
//                    if (it == grid->chunks.end()) continue;
//
//                    for (int y = minY; y <= maxY; ++y) {
//                        if (it->second->GetVoxel(x & 15, y, z & 15) == voxel::VoxelType::AIR)
//                            continue;
//
//                        glm::vec3 vMin(x, y, z);
//                        glm::vec3 vMax(x + 1.0f, y + 1.0f, z + 1.0f);
//
//                        for (auto &hb: comp.hitboxes) {
//                            CollisionResult hit = SweptAABB(pos + hb.currentMin, pos + hb.currentMax,
//                                                            vectorToMove, vMin, vMax, x, y, z, grid);
//
//                            if (hit.t < earliestHit.t) {
//                                earliestHit = hit;
//                            }
//                        }
//                    }
//                }
//            }
//
//
//            if (earliestHit.t < 1.0f) {
//                std::cout << "collision" << std::endl;
//                if (earliestHit.normal.y > 0.7f) {
//                    comp.onGround = true;
//                }
//
////                TryStepUp(pos, vectorToMove, comp, grid);
//
//                pos += vectorToMove * std::max(0.0f, earliestHit.t );
//
//                float dot = glm::dot(comp.velocity, earliestHit.normal);
//                if (dot < 0.0f) {
//                    comp.velocity -= earliestHit.normal * dot;
//                }
//
//                float timeLeft = 1.0f - earliestHit.t;
//                vectorToMove = comp.velocity * (dt * timeLeft);
//
//                pos += vectorToMove;
//            } else {
//                pos += vectorToMove;
//            }
////            std::cout << "earliest hit at " << std::to_string(earliestHit.t) << std::endl;
//
//
////            float friction = std::exp(-comp.frictionCoefficient * dt);
////            comp.velocity *= glm::vec3(friction, 1, friction);
//            obj->setPosition(pos);
//        }
    }
