#include "../../../include/scene/systems/AISystem.h"
#include "../../../include/scene/components/ECSManager.h"
#include "../../../include/scene/components/AIComponent.h"
#include "../../../include/scene/components/CharacterControllerComponent.h"
#include <cmath>
#include <algorithm>
#include <random>
#include <queue>
#include <unordered_map>
#include <limits>
#include <glm/gtx/norm.hpp>

namespace scene::components {

    struct Node {
        glm::ivec3 pos;
        float gScore;
        float fScore;

        bool operator>(const Node& other) const {
            return fScore > other.fScore;
        }
    };

    void AISystem::update(ECSManager& ecs, float dt) {
    }

    void AISystem::updateAI(ECSManager& ecs, std::shared_ptr<voxel::Grid> grid, float dt) {
        AIComponent* aiComponents = const_cast<AIComponent*>(ecs.getAiComponentsDense());
        int count = ecs.getAiComponentsAmount();

        for (int i = 0; i < count; ++i) {
            AIComponent& ai = aiComponents[i];
            if (ai.getEntityId() == -1) continue;

            CharacterControllerComponent* ctrl = &ecs.getCharacterControllerComponentFromSparse(ai.getEntityId());
            if (!ctrl || !ctrl->transform) continue;

            glm::vec3 currentPos = ctrl->transform->position;
            ai.decisionTimer -= dt;

            if (ai.decisionTimer <= 0.0f) {
                ai.decisionTimer = 0.5f;

                std::vector<int> nearbyEntities = grid->GetEntitiesNear(currentPos, 2.0f);
                int nearestEntityId = -1;
                float minDistanceSq = std::numeric_limits<float>::max();

                for (int targetId : nearbyEntities) {
                    if (targetId == ai.getEntityId()) continue;

                    CharacterControllerComponent* targetCtrl = &ecs.getCharacterControllerComponentFromSparse(targetId);
                    if (!targetCtrl || !targetCtrl->transform) continue;

                    float distSq = glm::distance2(currentPos, targetCtrl->transform->position);
                    if (distSq < minDistanceSq) {
                        minDistanceSq = distSq;
                        nearestEntityId = targetId;
                    }
                }
                ai.targetEntityId = nearestEntityId;
            }

            glm::vec3 targetGoal;
            bool hasTarget = false;

            if (ai.targetEntityId != -1) {
                CharacterControllerComponent* targetCtrl = &ecs.getCharacterControllerComponentFromSparse(ai.targetEntityId);
                if (targetCtrl && targetCtrl->transform) {
                    targetGoal = targetCtrl->transform->position;
                    hasTarget = true;
                } else {
                    ai.targetEntityId = -1;
                }
            }

            // Fallback: If no target is tracked, stand completely still
            if (!hasTarget) {
                ai.currentPath.clear();
                ctrl->m_currentSpeedFactor = 0.0f;
                ctrl->m_currentDirection = glm::vec2(0.0f);
                continue;
            }

            bool targetMoved = false;
            if (hasTarget && !ai.currentPath.empty()) {
                float driftDistSq = glm::distance2(ai.currentPath.back(), targetGoal);
                if (driftDistSq > 2.25f) {
                    targetMoved = true;
                }
            }

            if (ai.currentPath.empty() || targetMoved) {
                ai.currentPath = findPath(grid, currentPos, targetGoal);
                ai.currentPathIndex = 0;
            }

            if (!ai.currentPath.empty() && ai.currentPathIndex < ai.currentPath.size()) {
                glm::vec3 nodeTarget = ai.currentPath[ai.currentPathIndex];

                // Smooth Orientation: Always face the actual player position rather than individual voxel nodes
                glm::vec3 lookDiff = targetGoal - currentPos;
                lookDiff.y = 0.0f;
                if (glm::length2(lookDiff) > 0.01f) {
                    glm::vec3 lookDir = glm::normalize(lookDiff);
                    // Pass -lookDir to invert GLM's native right-handed bias for your LHS setup
                    ctrl->transform->rotation = glm::quatLookAt(-lookDir, glm::vec3(0.0f, 1.0f, 0.0f));
                }

                // Node navigation
                glm::vec3 diff = nodeTarget - currentPos;
                diff.y = 0.0f;

                if (glm::length(diff) < 0.5f) {
                    ai.currentPathIndex++;
                } else {
                    // Set direction components relative to the movement forward rules
                    ctrl->m_currentDirection = glm::vec2(0.0f, 1.0f) * ai.speed;
                    ctrl->m_currentSpeedFactor = glm::length(ctrl->m_currentDirection);
                }
            } else {
                ctrl->m_currentSpeedFactor = 0.0f;
                ctrl->m_currentDirection = glm::vec2(0.0f);
            }
        }
    }

    float AISystem::heuristic(glm::ivec3 a, glm::ivec3 b) {
        return glm::distance(glm::vec3(a), glm::vec3(b));
    }

    bool AISystem::isValidVoxel(std::shared_ptr<voxel::Grid> grid, glm::ivec3 pos) {
        bool bodyClear = !grid->IsSolid(pos.x, pos.y, pos.z);
        bool headClear = !grid->IsSolid(pos.x, pos.y + 1, pos.z);
        bool groundSolid = grid->IsSolid(pos.x, pos.y - 1, pos.z);
        return bodyClear && headClear && groundSolid;
    }

    std::vector<glm::vec3> AISystem::findPath(std::shared_ptr<voxel::Grid> grid, glm::vec3 start, glm::vec3 end) {
        glm::ivec3 startInt = glm::floor(start);
        glm::ivec3 endInt = glm::floor(end);

        if (!isValidVoxel(grid, endInt)) {
            bool targetAdjusted = false;
            for (int yOffset = 1; yOffset <= 3; ++yOffset) {
                if (isValidVoxel(grid, endInt + glm::ivec3(0, yOffset, 0))) {
                    endInt.y += yOffset;
                    targetAdjusted = true;
                    break;
                }
                if (isValidVoxel(grid, endInt - glm::ivec3(0, yOffset, 0))) {
                    endInt.y -= yOffset;
                    targetAdjusted = true;
                    break;
                }
            }
            if (!targetAdjusted && !isValidVoxel(grid, startInt)) return {};
        }

        std::priority_queue<Node, std::vector<Node>, std::greater<Node>> openSet;
        std::unordered_map<glm::ivec3, glm::ivec3, IntVec3Hash> cameFrom;
        std::unordered_map<glm::ivec3, float, IntVec3Hash> gScore;

        float startH = heuristic(startInt, endInt);
        openSet.push({startInt, 0.0f, startH});
        gScore[startInt] = 0.0f;

        const glm::ivec3 neighbors[6] = {
                {1, 0, 0}, {-1, 0, 0},
                {0, 0, 1}, {0, 0, -1},
                {0, 1, 0}, {0, -1, 0}
        };

        bool found = false;
        int iterations = 0;
        const int MAX_ITERATIONS = 1000;

        while (!openSet.empty() && iterations++ < MAX_ITERATIONS) {
            Node current = openSet.top();
            openSet.pop();

            if (current.gScore > gScore[current.pos]) {
                continue;
            }

            if (current.pos == endInt) {
                found = true;
                break;
            }

            for (const auto& offset : neighbors) {
                glm::ivec3 neighborPos = current.pos + offset;

                if (!isValidVoxel(grid, neighborPos)) {
                    if (offset.y == 0 && !grid->IsSolid(neighborPos.x, neighborPos.y + 1, neighborPos.z) && grid->IsSolid(neighborPos.x, neighborPos.y, neighborPos.z)) {
                        neighborPos.y += 1;
                    } else if (offset.y == 0 && !grid->IsSolid(neighborPos.x, neighborPos.y, neighborPos.z) && !grid->IsSolid(neighborPos.x, neighborPos.y - 1, neighborPos.z)) {
                        neighborPos.y -= 1;
                    } else {
                        continue;
                    }
                }

                float tentativeG = current.gScore + glm::distance(glm::vec3(current.pos), glm::vec3(neighborPos));

                auto it = gScore.find(neighborPos);
                if (it == gScore.end() || tentativeG < it->second) {
                    cameFrom[neighborPos] = current.pos;
                    gScore[neighborPos] = tentativeG;

                    float fScore = tentativeG + heuristic(neighborPos, endInt);
                    openSet.push({neighborPos, tentativeG, fScore});
                }
            }
        }

        std::vector<glm::vec3> path;
        if (found) {
            glm::ivec3 curr = endInt;
            while (curr != startInt) {
                path.push_back(glm::vec3(curr) + glm::vec3(0.5f, 0.1f, 0.5f));
                curr = cameFrom[curr];
            }
            std::reverse(path.begin(), path.end());
        }
        return path;
    }
}