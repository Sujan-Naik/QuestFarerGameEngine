#ifndef STATE_H
#define STATE_H

#include <memory>

namespace VoxelLib::Entity::FSM {

// Forward declaration
class CharacterControllerComponent;

/**
 * Inspired by Game AI Pro Chapter 12
 */
class State
{
protected:
    std::shared_ptr<CharacterControllerComponent> characterControllerComponent;

public:
    explicit State(std::shared_ptr<CharacterControllerComponent> characterControllerComponent)
        : characterControllerComponent(characterControllerComponent)
    {
    }

    virtual ~State() = default;

    virtual void Enter() = 0;
    virtual void Exit() = 0;
    virtual void Update(float deltaTime) = 0;
};

}

#endif // STATE_H