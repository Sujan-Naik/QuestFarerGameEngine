#ifndef QUESTFARERGAMEENGINE_PHYSICSSYSTEM_H
#define QUESTFARERGAMEENGINE_PHYSICSSYSTEM_H


#include <memory>

#include "../scene/components/PhysicsComponent.h"
#include "../voxel/Grid.h"

class PhysicsSystem {
public:
    void step(PhysicsComponent* components, int count,
              std::unique_ptr<GameObject>* gameObjects,
              std::shared_ptr<Grid> grid, float dt);

private:
    // Checks if an AABB overlaps any solid voxel in the Grid
    bool isColliding(const glm::vec3& pos, const glm::vec3& halfExtents, const Grid& grid);
};

#endif //QUESTFARERGAMEENGINE_PHYSICSSYSTEM_H
