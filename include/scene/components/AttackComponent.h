#pragma once

#include "Component.h"
#include <glm/glm.hpp>
#include <unordered_set>
#include <unordered_map>

namespace scene::components {

    class AttackComponent : public Component {
    public:
        int damagingBoneIndex = -1;
        float attackRadius = 0.5f;
        bool isAttackActive = false;

        std::unordered_set<int> entitiesAlreadyHit;
        std::unordered_map<int, glm::vec3> previousHandPositions;

        AttackComponent(int entityId) : Component(entityId) {}
        AttackComponent() : Component() {}
        ~AttackComponent() override = default;
    };

}