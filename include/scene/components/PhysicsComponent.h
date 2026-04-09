#ifndef QUESTFARERGAMEENGINE_PHYSICSCOMPONENT_H
#define QUESTFARERGAMEENGINE_PHYSICSCOMPONENT_H


#include "Component.h"
#include <glm/glm.hpp>

class PhysicsComponent : public Component {
public:
    glm::vec3 velocity{0.0f};
    glm::vec3 forceAccumulator{0.0f};

    float mass = 1.0f;
    float friction = 0.95f;      // "Air resistance" / Damping
    glm::vec3 halfExtents{0.5f}; // Hitbox: distance from center to faces
    bool onGround = false;

    explicit PhysicsComponent(int entityId) : Component(entityId) {}

    PhysicsComponent() : Component(-1) {}

    void receive(int message) override {

    }

    void update(GameObject* gameObject) override {

    }

    void applyForce(glm::vec3 force) {
        forceAccumulator += force;
    }

    // Newtonian integration: v = u + (F/m)t
    void integrate(float dt) {
        if (mass <= 0.0f) return;

        glm::vec3 acceleration = forceAccumulator / mass;
        acceleration += glm::vec3(0, -25.0f, 0); // Stronger Gravity for voxels

        velocity += acceleration * dt;
        velocity *= friction; // Apply damping

        forceAccumulator = glm::vec3(0.0f); // Reset for next frame
    }
};
#endif //QUESTFARERGAMEENGINE_PHYSICSCOMPONENT_H
