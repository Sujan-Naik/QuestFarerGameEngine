#ifndef ANIMATOR_H
#define ANIMATOR_H

#include <memory>
#include <vector>
#include <unordered_map>
#include "StateMachine.h"
#include "CharacterControllerComponent.h"
#include "AnimationWalkState.h"
#include "AnimationJumpState.h"
#include "AnimationAttackState.h"
#include "TransitionToWalk.h"
#include "TransitionToJump.h"
#include "TransitionToAttack.h"

namespace VoxelLib::Entity::Character {

class Animator
{
public:
    std::shared_ptr<StateMachine> fsm;

    explicit Animator(std::shared_ptr<CharacterControllerComponent> characterControllerComponent)
    {
        auto walk = std::make_shared<AnimationWalkState>(characterControllerComponent);
        auto walkTransition = std::make_shared<TransitionToWalk>(characterControllerComponent);

        auto jump = std::make_shared<AnimationJumpState>(characterControllerComponent);
        auto jumpTransition = std::make_shared<TransitionToJump>(characterControllerComponent);

        auto attack = std::make_shared<AnimationAttackState>(characterControllerComponent);
        auto attackTransition = std::make_shared<TransitionToAttack>(characterControllerComponent);

        auto walkTransitions = std::vector<StateMachine::TransitionStatePair>{
            StateMachine::TransitionStatePair(jump, jumpTransition),
            StateMachine::TransitionStatePair(attack, attackTransition)
        };

        auto attackTransitions = std::vector<StateMachine::TransitionStatePair>{
            StateMachine::TransitionStatePair(walk, walkTransition)
        };

        auto jumpTransitions = std::vector<StateMachine::TransitionStatePair>{
            StateMachine::TransitionStatePair(walk, walkTransition)
        };

        auto transitionsDictionary = 
            std::unordered_map<std::shared_ptr<State>, std::vector<StateMachine::TransitionStatePair>>{
                // {walk, walkTransitions},
                // {attack, attackTransitions},
                // {jump, jumpTransitions}
            };

        fsm = std::make_shared<StateMachine>(transitionsDictionary, walk);
    }

    void Update(float deltaTime)
    {
        fsm->Update(deltaTime);
    }
};

}

#endif // ANIMATOR_H