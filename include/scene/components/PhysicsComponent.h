#ifndef QUESTFARERGAMEENGINE_PHYSICSCOMPONENT_H
#define QUESTFARERGAMEENGINE_PHYSICSCOMPONENT_H

#include "Component.h"
#include <glm/glm.hpp>
#include <vector>

struct CollisionMesh {
    std::vector<glm::vec3> positions;
    std::vector<unsigned int> indices;
    glm::vec3 minBounds;
    glm::vec3 maxBounds;
};

class PhysicsComponent : public Component {
public:
    glm::vec3 velocity{0.0f};
    glm::vec3 forceAccumulator{0.0f};

    float mass = 1.0f;
    float friction = 0.95f;
    glm::vec3 halfExtents{0.5f};
    bool onGround = false;

    std::vector<CollisionMesh> collisionMeshes;
    bool useMeshCollision = false;

    explicit PhysicsComponent(int entityId) : Component(entityId) {}

    PhysicsComponent() : Component(-1) {}

    void receive(int message) override {}

    void update(GameObject* gameObject) override {}

    void addCollisionMesh(const std::vector<glm::vec3>& positions,
                          const std::vector<unsigned int>& indices) {
        CollisionMesh mesh;
        mesh.positions = positions;
        mesh.indices = indices;
        calculateMeshBounds(mesh);
        collisionMeshes.push_back(mesh);
        useMeshCollision = true;
    }

    void addCollisionMeshesFromModel(const std::vector<std::vector<glm::vec3>>& meshVertices,
                                     const std::vector<std::vector<unsigned int>>& meshIndices) {
        unsigned int indexOffset = 0;

        for (size_t i = 0; i < meshVertices.size(); ++i) {
            CollisionMesh mesh;
            mesh.positions = meshVertices[i];

            for (unsigned int index : meshIndices[i]) {
                mesh.indices.push_back(index + indexOffset);
            }

            calculateMeshBounds(mesh);
            collisionMeshes.push_back(mesh);
            indexOffset += meshVertices[i].size();
        }

        useMeshCollision = true;
    }

    void calculateMeshBounds(CollisionMesh& mesh) {
        if (mesh.positions.empty()) {
            mesh.minBounds = glm::vec3(0.0f);
            mesh.maxBounds = glm::vec3(0.0f);
            return;
        }

        mesh.minBounds = mesh.positions[0];
        mesh.maxBounds = mesh.positions[0];

        for (const auto& pos : mesh.positions) {
            mesh.minBounds = glm::min(mesh.minBounds, pos);
            mesh.maxBounds = glm::max(mesh.maxBounds, pos);
        }
    }

    glm::vec3 getWorldMinBounds(const glm::vec3& position, size_t meshIndex) const {
        if (meshIndex < collisionMeshes.size()) {
            return position + collisionMeshes[meshIndex].minBounds;
        }
        return position;
    }

    glm::vec3 getWorldMaxBounds(const glm::vec3& position, size_t meshIndex) const {
        if (meshIndex < collisionMeshes.size()) {
            return position + collisionMeshes[meshIndex].maxBounds;
        }
        return position;
    }

    void applyForce(glm::vec3 force) {
        forceAccumulator += force;
    }

    void integrate(float dt) {
        if (mass <= 0.0f) return;

        glm::vec3 acceleration = forceAccumulator / mass;

        velocity += acceleration * dt;
        velocity *= friction;

        forceAccumulator = glm::vec3(0.0f);
    }
};

#endif //QUESTFARERGAMEENGINE_PHYSICSCOMPONENT_H