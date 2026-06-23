#ifndef QUESTFARERGAMEENGINE_HEALTHCOMPONENT_H
#define QUESTFARERGAMEENGINE_HEALTHCOMPONENT_H

#include "Component.h"

namespace scene::components {

    class HealthComponent : public Component {
    public:
        using Component::Component;

        float currentHealth = 1000.0f;
        float maxHealth = 1000.0f;
        bool isDead = false;
    };
}

#endif