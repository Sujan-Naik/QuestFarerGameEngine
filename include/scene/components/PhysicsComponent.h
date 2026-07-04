#ifndef QUESTFARERGAMEENGINE_PHYSICSCOMPONENT_H
#define QUESTFARERGAMEENGINE_PHYSICSCOMPONENT_H

#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include "Component.h"
#include "../../rendering/model/ModelAnimation.h"

namespace scene::components {

    struct BoneHitbox {
        int boneIndex;
        glm::vec3 localMin;
        glm::vec3 localMax;
        glm::vec3 currentMin;
        glm::vec3 currentMax;

    };

    class PhysicsComponent : public Component {
    public:
        glm::vec3 kinematicDisplacement;


        glm::vec3 localTotalMin{0.0f};
        glm::vec3 localTotalMax{0.0f};

        std::shared_ptr<ModelAnimation> model;

        glm::vec3 velocity{0.0f};
        glm::vec3 forceAccumulator{0.0f};
        float mass = 100.0f;
        float dragCoefficient = 0.99f;
        float frictionCoefficient = 0.01f;
        bool onGround = false;

        std::vector<BoneHitbox> hitboxes;

        explicit PhysicsComponent(int entityId);
        PhysicsComponent();

        void addModel(std::shared_ptr<ModelAnimation> mod);
        void applyForce(glm::vec3 force);
        void addVelocity(glm::vec3 vel);
        void integrate(float dt);

        void setKinematicDisplacement(glm::vec3 displacementThisFrame);
    };
}

#endif