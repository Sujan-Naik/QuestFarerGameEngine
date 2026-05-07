#ifndef PLAYER_CONTROLLER_COMPONENT_H
#define PLAYER_CONTROLLER_COMPONENT_H

#include <stdexcept>
#include <memory>
#include "CharacterControllerComponent.h"
#include "Vector3F.h"
#include "FQuat.h"
#include "Globals.h"

namespace VoxelLib::Entity::Components {

class PlayerControllerComponent : public CharacterControllerComponent
{
private:
    static constexpr float JUMP_FORCE = 100.0f;
    
    Vector3F cameraOffset;
    float deltaPitch = 0.0f;
    float deltaYaw = 0.0f;
    float forward = 0.0f;
    float sinceLastUpdateActive = 0.0f;

public:
    PlayerControllerComponent(
        int team,
        int characterID,
        const Vector3F& position,
        const FQuat& rotation,
        float mass,
        const Vector3F& cameraOffset)
        : CharacterControllerComponent(characterID, team, position, rotation, mass),
          cameraOffset(cameraOffset)
    {
        if (player != nullptr)
        {
            throw std::runtime_error("Accidentally registering 2 Player Components!");
        }
        
        player = std::make_shared<PlayerControllerComponent>(*this);
    }

    void HandleInputs(
        float forward,
        bool right,
        bool left,
        float yaw,
        float pitch,
        bool jump,
        bool attack)
    {
        this->forward = forward * Globals::FORWARD_ACCELERATION_FORCE;

        // if (right) acceleration += transform->Right();
        // if (left) acceleration += transform->Left();

        deltaYaw = yaw;
        deltaPitch = pitch;

        if (!isJumping && !isAttacking)
        {
            if (jump && isOnGround)
            {
                this->jump = true;
                ApplyForce(transform->Up() * JUMP_FORCE);
            }
            else if (attack)
            {
                this->attack = true;
            }
        }
    }

    void Update(float deltaTime) override
    {
        CharacterControllerComponent::Update(deltaTime);

        ApplyForce(transform->Forward() * forward * deltaTime);
        transform->Rotate(FQuat::AngleAxis(
            transform->Up(),
            deltaYaw * -Globals::YAW_ROT_SPEED * deltaTime
        ));

        sinceLastUpdateActive -= deltaTime;
        if (sinceLastUpdateActive < 0.0f)
        {
            sinceLastUpdateActive = Globals::UPDATE_ACTIVE_CD;

            for (auto& characterComponent : characters)
            {
                if (characterComponent->DistanceSquared(player) 
                    Globals::NPC_PLAYER_ACTIVE_DISTANCE_SQUARED)
                {
                    characterComponent->SetActive(true);
                }
                else
                {
                    characterComponent->SetActive(false);
                }
            }
        }
    }

    Vector3F GetCameraPos() const
    {
        return transform->GetPosition() + 
               (transform->GetRotation() * cameraOffset);
    }
};

}

#endif // PLAYER_CONTROLLER_COMPONENT_H