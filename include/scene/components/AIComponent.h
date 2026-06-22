#ifndef QUESTFARERGAMEENGINE_AICOMPONENT_H
#define QUESTFARERGAMEENGINE_AICOMPONENT_H

#include "Component.h"
#include <glm/glm.hpp>
#include <vector>

namespace scene::components {

    class AIComponent : public Component {
    public:
        glm::vec3 currentGoal{0.0f};
        std::vector<glm::vec3> currentPath;
        int currentPathIndex = 0;
        float decisionTimer = 0.0f;
        float speed = 0.5f;

        explicit AIComponent(int entityId);
        AIComponent();
    };
}

#endif