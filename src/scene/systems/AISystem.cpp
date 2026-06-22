#include "../../../include/scene/systems/AISystem.h"
#include "../../../include/scene/components/ECSManager.h"
#include "../../../include/scene/components/AIComponent.h"
#include "../../../include/scene/components/CharacterControllerComponent.h"
#include <cmath>
#include <algorithm>
#include <random>

namespace scene::components {

    void AISystem::update(ECSManager& ecs, float dt) {
    }

    void AISystem::updateAI(ECSManager& ecs, std::shared_ptr<voxel::Grid> grid, float dt) {
        AIComponent* aiComponents = const_cast<AIComponent*>(ecs.getAiComponentsDense());
        int count = ecs.getAiComponentsAmount();

        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist(-15.0f, 15.0f);

        for (int i = 0; i < count; ++i) {
            AIComponent& ai = aiComponents[i];
            if (ai.getEntityId() == -1) continue;

            CharacterControllerComponent* ctrl = &ecs.getCharacterControllerComponentFromSparse(ai.getEntityId());
            if (!ctrl || !ctrl->transform) continue;

            glm::vec3 currentPos = ctrl->transform->position;
            ai.decisionTimer -= dt;

            if (ai.decisionTimer <= 0.0f || ai.currentPath.empty()) {
                ai.decisionTimer = 5.0f + (dist(gen) * 0.1f);
                glm::vec3 targetGoal = currentPos + glm::vec3(dist(gen), 0.0f, dist(gen));
                ai.currentPath = findPath(grid, currentPos, targetGoal);
                ai.currentPathIndex = 0;
            }

            if (!ai.currentPath.empty() && ai.currentPathIndex < ai.currentPath.size()) {
                glm::vec3 nodeTarget = ai.currentPath[ai.currentPathIndex];
                glm::vec3 diff = nodeTarget - currentPos;
                diff.y = 0.0f;

                if (glm::length(diff) < 0.5f) {
                    ai.currentPathIndex++;
                } else {
                    glm::vec3 dir = glm::normalize(diff);

                    ctrl->transform->rotation = glm::quatLookAt(dir, glm::vec3(0.0f, 1.0f, 0.0f));

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
            for (int yOffset = 1; yOffset <= 3; ++yOffset) {
                if (isValidVoxel(grid, endInt + glm::ivec3(0, yOffset, 0))) {
                    endInt.y += yOffset;
                    break;
                }
                if (isValidVoxel(grid, endInt - glm::ivec3(0, yOffset, 0))) {
                    endInt.y -= yOffset;
                    break;
                }
            }
        }

        std::priority_queue<Node, std::vector<Node>, std::greater<Node>> openSet;
        std::unordered_map<glm::ivec3, glm::ivec3, IntVec3Hash> cameFrom;
        std::unordered_map<glm::ivec3, float, IntVec3Hash> gScore;

        openSet.push({startInt, 0.0f, heuristic(startInt, endInt), startInt});
        gScore[startInt] = 0.0f;

        glm::ivec3 neighbors[6] = {
                {1, 0, 0}, {-1, 0, 0},
                {0, 0, 1}, {0, 0, -1},
                {0, 1, 0}, {0, -1, 0}
        };

        bool found = false;
        int safetyCounter = 0;

        while (!openSet.empty() && safetyCounter++ < 500) {
            Node current = openSet.top();
            openSet.pop();

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

                float tentativeG = gScore[current.pos] + glm::distance(glm::vec3(current.pos), glm::vec3(neighborPos));

                if (gScore.find(neighborPos) == gScore.end() || tentativeG < gScore[neighborPos]) {
                    cameFrom[neighborPos] = current.pos;
                    gScore[neighborPos] = tentativeG;
                    openSet.push({neighborPos, tentativeG, heuristic(neighborPos, endInt), current.pos});
                }
            }
        }

        std::vector<glm::vec3> path;
        if (found) {
            glm::ivec3 curr = endInt;
            while (curr != startInt) {
                path.push_back(glm::vec3(curr) + glm::vec3(0.5f, 0.0f, 0.5f));
                curr = cameFrom[curr];
            }
            std::reverse(path.begin(), path.end());
        }
        return path;
    }
}