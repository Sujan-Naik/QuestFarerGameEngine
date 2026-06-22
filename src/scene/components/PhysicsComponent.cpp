#include "../../../include/scene/components/PhysicsComponent.h"
#include <utility>

namespace scene::components {

    PhysicsComponent::PhysicsComponent(int entityId) : Component(entityId) {}
    PhysicsComponent::PhysicsComponent() : Component(-1) {}

    void PhysicsComponent::addModel(std::shared_ptr<ModelAnimation> mod) {
        this->model = std::move(mod);
        if (!model) return;

        hitboxes.clear();
        auto& boneMap = model->GetBoneInfoMap();
        float sz = 0.1f;

        for (auto const& [name, info] : boneMap) {
            BoneHitbox hb;
            hb.boneIndex = info.id;
            hb.localMin = glm::vec3(-sz);
            hb.localMax = glm::vec3(sz);
            hitboxes.push_back(hb);
        }
    }

    void PhysicsComponent::applyForce(glm::vec3 force) { forceAccumulator += force; }
    void PhysicsComponent::addVelocity(glm::vec3 vel) { velocity += vel; }
    void PhysicsComponent::integrate(float dt) {
        if (mass <= 0.0f) return;
        velocity += (forceAccumulator / mass) * dt;
        forceAccumulator = glm::vec3(0.0f);
    }
}