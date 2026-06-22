#ifndef QUESTFARERGAMEENGINE_PHYSICSSYSTEM_H
#define QUESTFARERGAMEENGINE_PHYSICSSYSTEM_H

#include <memory>
#include "../components/PhysicsComponent.h"
#include "../../voxel/Grid.h"
#include "System.h"
#include "../objects/GameObject.h"

namespace physics {
    class PhysicsSystem : public scene::components::System {
    public:
        void update(scene::components::ECSManager& ecs, float dt) override;

        void step(scene::components::PhysicsComponent* components, int count,
                  std::unique_ptr<GameObject>* gameObjects,
                  std::shared_ptr<voxel::Grid> grid, float dt);

    private:
        const glm::vec3 GRAVITY = glm::vec3(0.0f, -18.0f, 0.0f);
        bool IsPositionClear(const glm::vec3 &testPos, const scene::components::PhysicsComponent &comp, std::shared_ptr<voxel::Grid> grid);
        bool TryStepUp(glm::vec3 &pos, const glm::vec3 &remainingMove, const scene::components::PhysicsComponent &comp, std::shared_ptr<voxel::Grid> grid);
        float GetTerrainHeightAtXZ(std::shared_ptr<voxel::Grid> grid, glm::vec3 pos);
        bool IsFaceExposed(int x, int y, int z, const glm::vec3 &normal, std::shared_ptr<voxel::Grid> grid);
    };
}

#endif