#include "../../../include/scene/components/PhysicsComponent.h"
#include <utility>
#include <algorithm>
#include <limits>
#include <iomanip>
#include <vector>

namespace scene::components {

    PhysicsComponent::PhysicsComponent(int entityId) : Component(entityId) {}
    PhysicsComponent::PhysicsComponent() : Component(-1) {}

    void PhysicsComponent::addModel(std::shared_ptr<ModelAnimation> mod) {
        this->model = std::move(mod);
        if (!model) return;

        hitboxes.clear();
        auto& boneMap = model->GetBoneInfoMap();
        size_t numBones = boneMap.size();

        // 1. Temporary buffer for collecting bounds per bone ID
        struct TempBounds {
            int boneIndex = -1;
            std::string name;
            glm::vec3 minBound{std::numeric_limits<float>::max()};
            glm::vec3 maxBound{-std::numeric_limits<float>::max()};
            bool hasVertices = false;
        };

        std::vector<TempBounds> tempBounds(numBones);
        for (auto const& [name, info] : boneMap) {
            if (info.id >= 0 && info.id < static_cast<int>(numBones)) {
                tempBounds[info.id].boneIndex = info.id;
                tempBounds[info.id].name = name;
            }
        }

        // 2. Accumulate Mesh-Space bounds
        for (const auto& mesh : model->meshes) {
            for (const auto& vertex : mesh.vertices) {
                for (int i = 0; i < 4; ++i) {
                    int boneID = vertex.m_BoneIDs[i];
                    float weight = vertex.m_Weights[i];

                    if (boneID < 0 || boneID >= static_cast<int>(numBones) || weight < 0.40f) {
                        continue;
                    }

                    glm::vec3 meshSpacePos = glm::vec3(vertex.Position.x, vertex.Position.y, vertex.Position.z);

                    auto& tb = tempBounds[boneID];
                    tb.minBound = glm::min(tb.minBound, meshSpacePos);
                    tb.maxBound = glm::max(tb.maxBound, meshSpacePos);
                    tb.hasVertices = true;
                }
            }
        }

        // 3. Keep ONLY valid, non-empty, non-end hitboxes
        for (const auto& tb : tempBounds) {
            // Skip unassigned bones or bones without enough vertices
            if (!tb.hasVertices || tb.minBound.x > tb.maxBound.x) {
                continue;
            }

            // Skip exporter leaf/end bones completely
            if (tb.name.find("_end") != std::string::npos) {
                continue;
            }

            // Check for collapsed single-point boxes (zero volume)
            glm::vec3 size = tb.maxBound - tb.minBound;
            if (size.x <= 0.001f && size.y <= 0.001f && size.z <= 0.001f) {
                continue;
            }

            // Add valid hitbox to final list
            BoneHitbox hb;
            hb.boneIndex = tb.boneIndex;
            hb.localMin = tb.minBound - glm::vec3(0.01f);
            hb.localMax = tb.maxBound + glm::vec3(0.01f);

            hitboxes.push_back(hb);
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