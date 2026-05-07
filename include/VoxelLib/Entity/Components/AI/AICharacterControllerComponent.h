#ifndef AI_CHARACTER_CONTROLLER_COMPONENT_H
#define AI_CHARACTER_CONTROLLER_COMPONENT_H

#include <memory>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cmath>
#include "CharacterControllerComponent.h"
#include "StateMachine.h"
#include "AISensing.h"
#include "AStar.h"
#include "Vector2I.h"
#include "Vector3F.h"
#include "FQuat.h"
#include "Globals.h"
#include "KillEnemies.h"

namespace VoxelLib::Entity::Components::AI {

class AICharacterControllerComponent : public CharacterControllerComponent
{
private:
    static constexpr float RECALCULATE_PATH_CD = 10.0f;
    
    std::shared_ptr<AISensing> aiSensing;
    std::shared_ptr<AStar> aStar;
    std::shared_ptr<StateMachine> stateMachine;
    std::vector<Vector2I> path;
    
    float timeToNextRecalculation = 0.0f;

public:
    AICharacterControllerComponent(
        int team, 
        int characterID, 
        const Vector3F& position, 
        const FQuat& rotation, 
        float mass)
        : CharacterControllerComponent(characterID, team, position, rotation, mass),
          aStar(std::make_shared<AStar>())
    {
        auto emptyTransitions = std::unordered_map<std::shared_ptr<State>, 
            std::vector<StateMachine::TransitionStatePair>>{};
        
        stateMachine = std::make_shared<StateMachine>(
            emptyTransitions,
            std::make_shared<KillEnemies>(this)
        );
    }

    std::vector<AICharacterControllerComponent*> GetFriendlyAI()
    {
        std::vector<AICharacterControllerComponent*> friendlyAI;
        
        for (auto& component : characters)
        {
            auto aiComponent = dynamic_cast<AICharacterControllerComponent*>(component.get());
            if (aiComponent && aiComponent->team == team)
            {
                friendlyAI.push_back(aiComponent);
            }
        }
        
        return friendlyAI;
    }

    std::vector<AICharacterControllerComponent*> GetEnemyAI()
    {
        std::vector<AICharacterControllerComponent*> enemyAI;
        
        for (auto& component : characters)
        {
            auto aiComponent = dynamic_cast<AICharacterControllerComponent*>(component.get());
            if (aiComponent && aiComponent->team != team)
            {
                enemyAI.push_back(aiComponent);
            }
        }
        
        return enemyAI;
    }

    void SetTargetPos(const Vector2I& goal)
    {
        if (timeToNextRecalculation < RECALCULATE_PATH_CD &&
            aStar->GetPreviousPathGoal() != goal)
        {
            Vector2I aiPos2D(
                static_cast<int>(transform.GetPosition().x),
                static_cast<int>(transform.GetPosition().z)
            );
            path = aStar->ComputeAStar(aiPos2D, goal);
        }
    }

    void ClearPath()
    {
        path.clear();
    }

    void Update(float deltaTime) override
    {
        CharacterControllerComponent::Update(deltaTime);
        stateMachine->Update(deltaTime);
        
        timeToNextRecalculation -= deltaTime;
        
        if (!path.empty() && path.size() > 1)
        {
            FollowPath(deltaTime);
        }
    }

    void WalkForward(float deltaTime)
    {
        ApplyForce(transform.GetForward() * deltaTime * Globals::FORWARD_ACCELERATION_FORCE);
    }

private:
    void FollowPath(float deltaTime)
    {
        const Vector2I& newPosI = path[1];
        Vector2I aiPos2D(
            static_cast<int>(transform.GetPosition().x),
            static_cast<int>(transform.GetPosition().z)
        );
        
        Vector2I diff2D = aiPos2D - newPosI;
        if (diff2D.MagnitudeSquared() < 1)
        {
            path.erase(path.begin() + 1);
            return;
        }
        
        Vector3F diff(
            static_cast<float>(newPosI.x),
            0.0f,
            static_cast<float>(newPosI.z)
        );
        diff -= Vector3F(
            static_cast<float>(aiPos2D.x),
            0.0f,
            static_cast<float>(aiPos2D.z)
        );
        
        if (diff.Magnitude() != 0.0f)
        {
            diff = diff.Normalized();
            ApplyForce(diff * deltaTime * Globals::FORWARD_ACCELERATION_FORCE);
            
            float angle = std::atan2(diff.x, diff.z);
            SetRotation(FQuat::AngleAxis(Vector3F(0.0f, 1.0f, 0.0f), angle));
        }
    }
};

}

#endif // AI_CHARACTER_CONTROLLER_COMPONENT_H