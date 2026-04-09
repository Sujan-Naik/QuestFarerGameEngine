#ifndef QUESTFARERGAMEENGINE_PHYSICSCOMPONENT_H
#define QUESTFARERGAMEENGINE_PHYSICSCOMPONENT_H

#include "Component.h"
#include "../objects/GameObject.h"

class PhysicsComponent: public Component{
private:
    glm::vec3 velocity = glm::vec3(0.0f);
    glm::vec3 position = glm::vec3(0.0f);


public:

    explicit PhysicsComponent(int entityId) : Component(entityId) {}
    PhysicsComponent() : Component(-1) {}
    void receive(int message) override {

    }

    void update(GameObject* gameObject) override{
        velocity += GRAVITY * glm::vec3(0, FIXED_TIMESTEP, 0);

        position += velocity * glm::vec3(0, FIXED_TIMESTEP, 0);

        gameObject->setPosition(position);


    }

};
#endif //QUESTFARERGAMEENGINE_PHYSICSCOMPONENT_H
