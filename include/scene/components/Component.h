#ifndef QUESTFARERGAMEENGINE_COMPONENT_H
#define QUESTFARERGAMEENGINE_COMPONENT_H

#include <memory>

namespace scene::components {

    class ECSManager;

    class Component {
    protected:
        int entityId = -1;
        std::shared_ptr<ECSManager> ecsManager;

    public:
        Component(int entityId) : entityId(entityId) {}
        Component() = default;
        virtual ~Component() = default;

        void setECSManager(std::shared_ptr<ECSManager> newEcsManager) {
            ecsManager = std::move(newEcsManager);
        }

        int getEntityId() const { return entityId; }
    };
}

#endif