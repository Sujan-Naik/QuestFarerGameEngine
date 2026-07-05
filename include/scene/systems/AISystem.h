#ifndef QUESTFARERGAMEENGINE_AISYSTEM_H
#define QUESTFARERGAMEENGINE_AISYSTEM_H

#include "System.h"
#include "../../voxel/Grid.h"
#include "../components/ECSManager.h"
#include <glm/glm.hpp>
#include <vector>
#include <memory>

namespace scene::components {

    class AISystem : public System {
    public:
        void update(ECSManager& ecs, float dt) override;
        void updateAI(ECSManager& ecs, std::shared_ptr<voxel::Grid> grid, float dt);

    private:
        struct IntVec3Hash {
            std::size_t operator()(const glm::ivec3& v) const {
                return (v.x * 73856093) ^ (v.y * 19349663) ^ (v.z * 83492791);
            }
        };

        std::vector<glm::vec3> findPath(std::shared_ptr<voxel::Grid> grid, glm::vec3 start, glm::vec3 end);
        float heuristic(glm::ivec3 a, glm::ivec3 b);
        bool isValidVoxel(std::shared_ptr<voxel::Grid> grid, glm::ivec3 pos);
    };
}

#endif