#ifndef QUESTFARERGAMEENGINE_COMPONENT_H
#define QUESTFARERGAMEENGINE_COMPONENT_H

#include <memory>
#include "../objects/GameObject.h"


namespace scene::components {

    class ECSManager;

    class Component {
    protected:
        int entityId;
        std::shared_ptr<ECSManager> ecsManager;
    public:

        Component(int entityId) : entityId(entityId) {}

        void setECSManager(std::shared_ptr<ECSManager> newEcsManager){
            ecsManager = std::move(newEcsManager);
        }

        virtual ~Component() = default;

        virtual void receive(int message) = 0;

        virtual void update() = 0;

        int getEntityId() { return entityId; }
    };
}
#endif //QUESTFARERGAMEENGINE_COMPONENT_H
