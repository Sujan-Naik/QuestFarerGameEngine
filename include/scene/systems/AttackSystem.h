#pragma once
#include "System.h"

namespace scene::components {

    class AttackSystem : public System {
    public:
        virtual ~AttackSystem() = default;
        virtual void update(ECSManager& ecs, float dt) override;
    };
}