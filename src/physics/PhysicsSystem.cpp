#include "../../include/physics/PhysicsSystem.h"
#include "../../include/scene/components/PhysicsComponent.h"
#include "../../include/voxel/Grid.h"

#include <cmath>

void PhysicsSystem::step(PhysicsComponent* components, int count,
                         std::unique_ptr<GameObject>* gameObjects,
                         std::shared_ptr<Grid> grid, float dt) {
    for (int i = 0; i < count; ++i) {
        PhysicsComponent& comp = components[i];
        GameObject* obj = gameObjects[comp.getEntityId()].get();

        // 1. Newtonian Integration (Update Velocity)
        comp.integrate(dt);

        glm::vec3 pos = obj->getPosition();
        glm::vec3 moveStep = comp.velocity * dt;

        // 2. Resolve Y-axis (Priority for gravity/jumping)
        pos.y += moveStep.y;
        comp.onGround = false;
        if (isColliding(pos, comp.halfExtents, *grid)) {
            if (comp.velocity.y < 0) comp.onGround = true;
            pos.y -= moveStep.y;
            comp.velocity.y = 0;
        }

        // 3. Resolve X-axis
        pos.x += moveStep.x;
        if (isColliding(pos, comp.halfExtents, *grid)) {
            pos.x -= moveStep.x;
            comp.velocity.x = 0;
        }

        // 4. Resolve Z-axis
        pos.z += moveStep.z;
        if (isColliding(pos, comp.halfExtents, *grid)) {
            pos.z -= moveStep.z;
            comp.velocity.z = 0;
        }

        obj->setPosition(pos);
    }
}

bool PhysicsSystem::isColliding(const glm::vec3& pos, const glm::vec3& halfExtents, const Grid& grid) {
    // Determine voxel bounds to check
    int minX = std::floor(pos.x - halfExtents.x);
    int maxX = std::floor(pos.x + halfExtents.x);
    int minY = std::floor(pos.y - halfExtents.y);
    int maxY = std::floor(pos.y + halfExtents.y);
    int minZ = std::floor(pos.z - halfExtents.z);
    int maxZ = std::floor(pos.z + halfExtents.z);

    // Query the world grid
    for (int x = minX; x <= maxX; ++x) {
        for (int y = minY; y <= maxY; ++y) {
            for (int z = minZ; z <= maxZ; ++z) {
                if (grid.isSolid(x, y, z)) return true;
            }
        }
    }
    return false;
}