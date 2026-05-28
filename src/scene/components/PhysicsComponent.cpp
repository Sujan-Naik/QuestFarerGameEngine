#include "../../../include/scene/components/PhysicsComponent.h"
#include <algorithm>

namespace scene::components {

    PhysicsComponent::PhysicsComponent(int entityId) : Component(entityId) {}
    PhysicsComponent::PhysicsComponent() : Component(-1) {}

    void PhysicsComponent::receive(int message) {}

    void PhysicsComponent::addModel(std::shared_ptr<ModelAnimation> mod) {
        this->model = std::move(mod);
        if (!model) return;

        hitboxes.clear();
        auto& boneMap = model->GetBoneInfoMap();

        // Adjust this sz to match the thickness of your character's limbs
        // If your character is 1.8 units tall, 0.1f is a good bone radius
        float sz = 0.1f;

        for (auto const& [name, info] : boneMap) {
            BoneHitbox hb;
            hb.boneIndex = info.id;
            hb.localMin = glm::vec3(-sz);
            hb.localMax = glm::vec3(sz);
            hitboxes.push_back(hb);
        }

        this->update();
    }

    void PhysicsComponent::update() {
        if (!model || hitboxes.empty()) return;
        addVelocity(glm::vec3(0,0,1) * 0.1f);

        const auto& boneMatrices = model->GetFinalBoneMatrices();
        if (boneMatrices.empty()) return;

        localTotalMin = glm::vec3(1e10f);
        localTotalMax = glm::vec3(-1e10f);

        // This scale MUST match the scale you gave the character in initialisePlayer
        const float modelScale = 0.01f;

        for (auto& hb : hitboxes) {
            if (hb.boneIndex >= (int)boneMatrices.size()) continue;

            // Transform local bone box by the animated matrix
            const glm::mat4& m = boneMatrices[hb.boneIndex];

            glm::vec3 corners[8] = {
                    glm::vec3(m * glm::vec4(hb.localMin.x, hb.localMin.y, hb.localMin.z, 1.0f)),
                    glm::vec3(m * glm::vec4(hb.localMax.x, hb.localMin.y, hb.localMin.z, 1.0f)),
                    glm::vec3(m * glm::vec4(hb.localMin.x, hb.localMax.y, hb.localMin.z, 1.0f)),
                    glm::vec3(m * glm::vec4(hb.localMax.x, hb.localMax.y, hb.localMin.z, 1.0f)),
                    glm::vec3(m * glm::vec4(hb.localMin.x, hb.localMin.y, hb.localMax.z, 1.0f)),
                    glm::vec3(m * glm::vec4(hb.localMax.x, hb.localMin.y, hb.localMax.z, 1.0f)),
                    glm::vec3(m * glm::vec4(hb.localMin.x, hb.localMax.y, hb.localMax.z, 1.0f)),
                    glm::vec3(m * glm::vec4(hb.localMax.x, hb.localMax.y, hb.localMax.z, 1.0f))
            };

            hb.currentMin = glm::vec3(1e10f);
            hb.currentMax = glm::vec3(-1e10f);

            for (auto& c : corners) {
                // Apply the game-world scale to the raw bone data
                c *= modelScale;
                hb.currentMin = glm::min(hb.currentMin, c);
                hb.currentMax = glm::max(hb.currentMax, c);
            }

            localTotalMin = glm::min(localTotalMin, hb.currentMin);
            localTotalMax = glm::max(localTotalMax, hb.currentMax);
        }

        // --- THE FLOATING FIX ---
        // If the character's feet (localTotalMin.y) are calculated to be above 0,
        // it means the physics system will stop the character early, causing floating.
        // We ensure the bottom of the hull is at least at the GameObject's origin.
        if (localTotalMin.y > 0.0f) localTotalMin.y = 0.0f;
    }

    void PhysicsComponent::applyForce(glm::vec3 force) { forceAccumulator += force; }
    void PhysicsComponent::addVelocity(glm::vec3 vel) { velocity += vel; }
    void PhysicsComponent::integrate(float dt) {
        if (mass <= 0.0f) return;
        velocity += (forceAccumulator / mass) * dt;
        forceAccumulator = glm::vec3(0.0f);
    }
}