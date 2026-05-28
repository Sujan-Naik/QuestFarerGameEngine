
#include "../../../include/scene/components/AIComponent.h"

namespace scene::components {

    AIComponent::AIComponent(int entityId) : Component(entityId) {}

    AIComponent::AIComponent() : Component(-1) {}

    void AIComponent::receive(int message) {}

    void AIComponent::update() {}
}