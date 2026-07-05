#ifndef QUESTFARERGAMEENGINE_SYSTEM_H
#define QUESTFARERGAMEENGINE_SYSTEM_H

namespace scene::components {

    class ECSManager;

    class System {
    public:
        virtual ~System() = default;
        virtual void update(ECSManager& ecs, float dt) = 0;
    };
}

#endif