#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <memory>
#include <unordered_map>
#include <vector>
#include "State.h"
#include "Transition.h"

namespace VoxelLib::Entity::FSM {

/**
 * Inspired by Game AI Pro Chapter 12
 */
class StateMachine
{
public:
    struct TransitionStatePair
    {
        std::shared_ptr<State> state;
        std::shared_ptr<Transition> transition;

        TransitionStatePair(
            std::shared_ptr<State> state,
            std::shared_ptr<Transition> transition)
            : state(state), transition(transition)
        {
        }
    };

private:
    using TransitionMap = std::unordered_map
        std::shared_ptr<State>,
        std::vector<TransitionStatePair>
    >;

    TransitionMap transitionsDictionary;
    std::shared_ptr<State> currentState;
    bool currentStateEntered = false;

public:
    StateMachine(
        const TransitionMap& transitionsDictionary,
        std::shared_ptr<State> currentState)
        : transitionsDictionary(transitionsDictionary),
          currentState(currentState)
    {
    }

    virtual ~StateMachine() = default;

    void Update(float deltaTime)
    {
        if (currentState != nullptr)
        {
            if (!currentStateEntered)
            {
                currentState->Enter();
                currentStateEntered = true;
            }

            currentState->Update(deltaTime);
        }

        // Check if current state exists in transitions dictionary
        auto it = transitionsDictionary.find(currentState);
        if (it == transitionsDictionary.end())
        {
            return;
        }

        const std::vector<TransitionStatePair>& transitionList = it->second;
        for (const auto& transitionStatePair : transitionList)
        {
            if (transitionStatePair.transition->ShouldTransition())
            {
                currentState->Exit();
                currentState = transitionStatePair.state;
                currentState->Enter();
                break;
            }
        }
    }

    std::shared_ptr<State> GetCurrentState() const
    {
        return currentState;
    }

    void SetCurrentState(std::shared_ptr<State> newState)
    {
        if (currentState != nullptr)
        {
            currentState->Exit();
        }
        currentState = newState;
        currentStateEntered = false;
    }
};

}

#endif // STATE_MACHINE_H