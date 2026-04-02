#ifndef QUESTFARERGAMEENGINE_COMPONENT_H
#define QUESTFARERGAMEENGINE_COMPONENT_H

#include "../objects/GameObject.h"

class Component{
protected:
    int entityId;

public:

    Component(int entityId) : entityId(entityId) {}

    virtual ~Component() = default;

    virtual void receive(int message) = 0;

    virtual void update(GameObject* gameObject) = 0;

    int getEntityId(){return entityId;}
};

#endif //QUESTFARERGAMEENGINE_COMPONENT_H
