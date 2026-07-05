#include "../../../include/scene/components/PhysicsComponent.h"
#include <utility>
#include <algorithm>
#include <limits>
#include <iomanip>

namespace scene::components {

    PhysicsComponent::PhysicsComponent(int entityId) : Component(entityId) {}
    PhysicsComponent::PhysicsComponent() : Component(-1) {}

    void PhysicsComponent::addModel(std::shared_ptr<ModelAnimation> mod) {
        this->model = std::move(mod);
        if (!model) return;

        hitboxes.clear();
        auto& boneMap = model->GetBoneInfoMap();

        // Map strictly by bone ID to fix the alphabetical array sorting mismatch
        hitboxes.resize(boneMap.size());

        for (auto const& [name, info] : boneMap) {
            BoneHitbox hb;
            hb.boneIndex = info.id;
            hb.localMin = glm::vec3(std::numeric_limits<float>::max());
            hb.localMax = glm::vec3(-std::numeric_limits<float>::max());
            hitboxes[info.id] = hb;
        }

        for (const auto& mesh : model->meshes) {
            for (const auto& vertex : mesh.vertices) {
                for (int i = 0; i < 4; ++i) {
                    int boneID = vertex.m_BoneIDs[i];
                    float weight = vertex.m_Weights[i];

                    if (boneID < 0 || boneID >= static_cast<int>(hitboxes.size()) || weight < 0.20f) {
                        continue;
                    }

                    // Keep coordinates in standard Mesh Space (Relative to model origin)
                    glm::vec3 meshSpacePos = glm::vec3(vertex.Position.x, vertex.Position.y, vertex.Position.z);

                    auto& hb = hitboxes[boneID];
                    hb.localMin = glm::min(hb.localMin, meshSpacePos);
                    hb.localMax = glm::max(hb.localMax, meshSpacePos);
                }
            }
        }

        for (auto& hb : hitboxes) {
            if (hb.localMin.x > hb.localMax.x) {
                float fallbackSize = 0.2f;
                hb.localMin = glm::vec3(-fallbackSize);
                hb.localMax = glm::vec3(fallbackSize);
            } else {
                hb.localMin -= glm::vec3(0.05f);
                hb.localMax += glm::vec3(0.05f);
            }
        }
    }

    void PhysicsComponent::applyForce(glm::vec3 force) { forceAccumulator += force; }
    void PhysicsComponent::setKinematicDisplacement(glm::vec3 displacementThisFrame) { kinematicDisplacement = displacementThisFrame; }
    void PhysicsComponent::addVelocity(glm::vec3 vel) { velocity += vel; }
    void PhysicsComponent::integrate(float dt) {
        if (mass <= 0.0f) return;
        velocity += (forceAccumulator / mass) * dt;
        forceAccumulator = glm::vec3(0.0f);
    }

    bool PhysicsComponent::hasGravity() const {
        return gravityEnabled;
    }

    void PhysicsComponent::setGravity(bool newGravity) {
        PhysicsComponent::gravityEnabled = newGravity;
    }
}