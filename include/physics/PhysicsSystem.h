#ifndef QUESTFARERGAMEENGINE_PHYSICSSYSTEM_H
#define QUESTFARERGAMEENGINE_PHYSICSSYSTEM_H

#include <memory>
#include <vector>
#include "../scene/components/PhysicsComponent.h"
#include "../voxel/Grid.h"

namespace physics {
    class PhysicsSystem {
    public:
        void step(scene::components::PhysicsComponent* components, int count,
                  std::unique_ptr<GameObject>* gameObjects,
                  std::shared_ptr<voxel::Grid> grid, float dt);

    private:
        const glm::vec3 GRAVITY = glm::vec3(0.0f, -18.0f, 0.0f);
    };
}

#endif