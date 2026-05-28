#ifndef QUESTFARERGAMEENGINE_AICOMPONENT_H
#define QUESTFARERGAMEENGINE_AICOMPONENT_H

#include "Component.h"

namespace scene::components {
    class AIComponent : public Component {
    public:
        explicit AIComponent(int entityId);
        AIComponent();

        void receive(int message) override;
        void update() override;
    };
}

#endif //QUESTFARERGAMEENGINE_AICOMPONENT_H