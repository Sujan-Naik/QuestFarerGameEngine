#ifndef TRANSITION_H
#define TRANSITION_H

#include <memory>

namespace VoxelLib::Entity::FSM {

// Forward declaration
class CharacterControllerComponent;

/**
 * Inspired by Game AI Pro Chapter 12
 */
class Transition
{
protected:
    std::shared_ptr<CharacterControllerComponent> characterControllerComponent;

public:
    explicit Transition(std::shared_ptr<CharacterControllerComponent> characterControllerComponent)
        : characterControllerComponent(characterControllerComponent)
    {
    }

    virtual ~Transition() = default;

    std::shared_ptr<CharacterControllerComponent> GetCharacterController() const
    {
        return characterControllerComponent;
    }

    virtual bool ShouldTransition() = 0;
};

}

#endif // TRANSITION_H