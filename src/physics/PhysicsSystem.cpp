#include "../../include/physics/PhysicsSystem.h"
#include "../../include/scene/components/PhysicsComponent.h"
#include "../../include/voxel/Grid.h"

#include <cmath>

using namespace physics;
void PhysicsSystem::step(PhysicsComponent* components, int count,
                         std::unique_ptr<GameObject>* gameObjects,
                         std::shared_ptr<Grid> grid, float dt) {
    for (int i = 0; i < count; ++i) {
        PhysicsComponent& comp = components[i];
        GameObject* obj = gameObjects[comp.getEntityId()].get();

        // Apply gravity and integrate
        comp.applyForce(GRAVITY * comp.mass);
        comp.integrate(dt);

        glm::vec3 pos = obj->getPosition();
        glm::vec3 displacement = comp.velocity * dt;

        // Ground detection
        glm::vec3 belowPos = pos + glm::vec3(0, -comp.halfExtents.y - EPSILON, 0);
        comp.onGround = isColliding(belowPos, comp.halfExtents, *grid);

        // Resolve Y-axis
        pos.y += displacement.y;
        if (isColliding(pos, comp.halfExtents, *grid)) {
            pos.y -= displacement.y;
            comp.velocity.y = 0;
            comp.onGround = (displacement.y < 0);
        }

        // Resolve X-axis
        pos.x += displacement.x;
        if (isColliding(pos, comp.halfExtents, *grid)) {
            pos.x -= displacement.x;
            comp.velocity.x = 0;
        }

        // Resolve Z-axis
        pos.z += displacement.z;
        if (isColliding(pos, comp.halfExtents, *grid)) {
            pos.z -= displacement.z;
            comp.velocity.z = 0;
        }

        obj->setPosition(pos);
    }
}

bool PhysicsSystem::isColliding(const glm::vec3& pos, const glm::vec3& halfExtents,
                                const Grid& grid) {
    int minX = static_cast<int>(std::floor(pos.x - halfExtents.x));
    int maxX = static_cast<int>(std::floor(pos.x + halfExtents.x));
    int minY = static_cast<int>(std::floor(pos.y - halfExtents.y));
    int maxY = static_cast<int>(std::floor(pos.y + halfExtents.y));
    int minZ = static_cast<int>(std::floor(pos.z - halfExtents.z));
    int maxZ = static_cast<int>(std::floor(pos.z + halfExtents.z));

    for (int x = minX; x <= maxX; ++x) {
        for (int y = minY; y <= maxY; ++y) {
            for (int z = minZ; z <= maxZ; ++z) {
                if (grid.isSolid(x, y, z)) return true;
            }
        }
    }
    return false;
}