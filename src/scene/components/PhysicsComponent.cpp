#include "../../../include/scene/components/PhysicsComponent.h"
#include <utility>
#include <algorithm>
#include <limits>

namespace scene::components {

    PhysicsComponent::PhysicsComponent(int entityId) : Component(entityId) {}
    PhysicsComponent::PhysicsComponent() : Component(-1) {}

    void PhysicsComponent::addModel(std::shared_ptr<ModelAnimation> mod) {
        this->model = std::move(mod);
        if (!model) return;

        hitboxes.clear();
        auto& boneMap = model->GetBoneInfoMap();

        // 1. Initialize empty tracking boxes for every bone inside the skeleton map
        for (auto const& [name, info] : boneMap) {
            BoneHitbox hb;
            hb.boneIndex = info.id;
            hb.localMin = glm::vec3(std::numeric_limits<float>::max());
            hb.localMax = glm::vec3(-std::numeric_limits<float>::max());
            hitboxes.push_back(hb);
        }

        // 2. Loop directly over your model's meshes and their internal vertices
        for (const auto& mesh : model->meshes) {
            for (const auto& vertex : mesh.vertices) {
                // Loop through your vertex bone influence channels (MAX_BONE_INFLUENCE = 4)
                for (int i = 0; i < 4; ++i) {
                    int boneID = vertex.m_BoneIDs[i];
                    float weight = vertex.m_Weights[i];

                    // Guard against unbound joints or weak skin weights (< 20% structural hold)
                    if (boneID < 0 || boneID >= static_cast<int>(hitboxes.size()) || weight < 0.20f) {
                        continue;
                    }

                    // Look up the corresponding bone's unique offset matrix
                    glm::mat4 offsetMatrix(1.0f);
                    for (auto const& [name, info] : boneMap) {
                        if (info.id == boneID) {
                            offsetMatrix = info.offset;
                            break;
                        }
                    }

                    // Convert standard mesh-space position directly to bone-local space
                    glm::vec4 meshLocalPos(vertex.Position.x, vertex.Position.y, vertex.Position.z, 1.0f);
                    glm::vec3 boneLocalPos = glm::vec3(offsetMatrix * meshLocalPos);

                    // Tighten the boundary encapsulating this vertex inside the local joint footprint
                    auto& hb = hitboxes[boneID];
                    hb.localMin = glm::min(hb.localMin, boneLocalPos);
                    hb.localMax = glm::max(hb.localMax, boneLocalPos);
                }
            }
        }

        // 3. Fallback normalization pass
        for (auto& hb : hitboxes) {
            if (hb.localMin.x > hb.localMax.x) {
                // Fallback dimensions for helper/utility joints without vertex weight mappings
                float fallbackSize = 0.2f;
                hb.localMin = glm::vec3(-fallbackSize);
                hb.localMax = glm::vec3(fallbackSize);
            } else {
                // Uniform padding offset to guarantee the bounding shell sits slightly past vertex bounds
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