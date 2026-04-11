#ifndef QUESTFARERGAMEENGINE_AICOMPONENT_H
#define QUESTFARERGAMEENGINE_AICOMPONENT_H


#include "Component.h"

namespace scene::components {
    class AIComponent : public Component {
    public:

        explicit AIComponent(int entityId) : Component(entityId) {}

        AIComponent() : Component(-1) {}

        void receive(int message) override {

        }

        void update() override {

        }
    };
}
#endif //QUESTFARERGAMEENGINE_AICOMPONENT_H
