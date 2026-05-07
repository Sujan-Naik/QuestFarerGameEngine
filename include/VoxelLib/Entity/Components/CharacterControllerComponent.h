#ifndef CHARACTER_CONTROLLER_COMPONENT_H
#define CHARACTER_CONTROLLER_COMPONENT_H

#include <vector>
#include <memory>
#include <cmath>
#include "Transform.h"
#include "Avatar.h"
#include "Vector3F.h"
#include "FQuat.h"
#include "Terrain.h"
#include "Globals.h"

namespace VoxelLib::Entity::Components {

class CharacterControllerComponent
{
public:
    static std::vector<std::shared_ptr<CharacterControllerComponent>> characters;
    static std::shared_ptr<class PlayerControllerComponent> player;

protected:
    int ID;
    int team;
    bool active = true;
    bool attack = false;
    std::shared_ptr<Avatar> avatar;
    float deltaTime = 0.0f;
    float health = Globals::START_HEALTH;
    bool isAttacking = false;
    bool isDead = false;
    bool isJumping = false;
    bool isOnGround = false;
    bool jump = false;
    float mass = 0.0f;
    std::shared_ptr<Transform> transform;
    Vector3F velocity;
    Vector3F acceleration;

public:
    CharacterControllerComponent(
        int characterID,
        int team,
        const Vector3F& position,
        const FQuat& rotation,
        float mass)
        : ID(characterID), team(team), mass(mass)
    {
        characters.push_back(std::make_shared<CharacterControllerComponent>(*this));
        
        float groundHeight = Terrain::GetInstance().GetGroundHeight(position.x, position.z);
        transform = std::make_shared<Transform>(
            rotation,
            Vector3F(
                position.x,
                groundHeight + Globals::PLAYER_HEIGHT,
                position.z
            )
        );
        
        avatar = std::make_shared<Avatar>(this);
    }

    virtual ~CharacterControllerComponent() = default;

    std::shared_ptr<Transform> GetTransform() const
    {
        return transform;
    }

    int GetID() const
    {
        return ID;
    }

    void SetActive(bool newActive)
    {
        active = newActive;
    }

    std::shared_ptr<Transform> GetAvatarBoneTransformByName(const std::string& boneName)
    {
        return avatar->GetTransformByName(boneName);
    }

    bool ShouldJump() const
    {
        return jump;
    }

    bool IsJumping() const
    {
        return isJumping;
    }

    void SetIsJumping(bool newJumping)
    {
        isJumping = newJumping;
    }

    bool ShouldAttack() const
    {
        return attack;
    }

    bool IsAttacking() const
    {
        return isAttacking;
    }

    void SetIsAttacking(bool newAttacking)
    {
        isAttacking = newAttacking;
    }

    std::shared_ptr<Avatar> GetAvatar() const
    {
        return avatar;
    }

    float GetSpeed() const
    {
        return std::sqrt(velocity.x * velocity.x + velocity.z * velocity.z);
    }

    void SetPosition(const Vector3F& newPosition)
    {
        transform->SetPosition(newPosition);
    }

    void SetRotation(const FQuat& newRotation)
    {
        transform->SetRotation(newRotation);
    }

    void Rotate(const FQuat& newRotation)
    {
        transform->Rotate(newRotation);
    }

    void ApplyForce(const Vector3F& force)
    {
        acceleration += force / mass;
    }

    float Distance(const std::shared_ptr<CharacterControllerComponent>& other) const
    {
        return (transform->GetPosition() - other->GetTransform()->GetPosition()).Magnitude();
    }

    float DistanceSquared(const std::shared_ptr<CharacterControllerComponent>& other) const
    {
        return (transform->GetPosition() - other->GetTransform()->GetPosition()).MagnitudeSquared();
    }

    bool IsDead() const
    {
        return isDead;
    }

    void Hurt(float damage)
    {
        health -= damage;
        if (health < 0.0f)
        {
            isDead = true;
        }
    }

    virtual void Update(float deltaTime)
    {
        if (!active) return;

        this->deltaTime = deltaTime;
        CheckIsOnGround();

        if (velocity.Magnitude() > Globals::PLAYER_MAX_SPEED)
        {
            velocity = velocity.Normalized() * Globals::PLAYER_MAX_SPEED;
        }

        Vector3F currentPos = transform->GetPosition();
        transform->SetPosition(currentPos + velocity * deltaTime);

        currentPos = transform->GetPosition();
        if (currentPos.x < 0.0f)
        {
            transform->SetPosition(Vector3F(0.0f, currentPos.y, currentPos.z));
            velocity += Vector3F(1.0f, 0.0f, 0.0f);
        }
        else if (currentPos.x > Globals::MAX_X)
        {
            transform->SetPosition(Vector3F(Globals::MAX_X, currentPos.y, currentPos.z));
            velocity += Vector3F(-1.0f, 0.0f, 0.0f);
        }
        else if (currentPos.z < 0.0f)
        {
            transform->SetPosition(Vector3F(currentPos.x, currentPos.y, 0.0f));
            velocity += Vector3F(0.0f, 0.0f, 1.0f);
        }
        else if (currentPos.z > Globals::MAX_Z)
        {
            transform->SetPosition(Vector3F(currentPos.x, currentPos.y, Globals::MAX_Z));
            velocity += Vector3F(0.0f, 0.0f, -1.0f);
        }

        avatar->Update(deltaTime);

        if (!isOnGround)
        {
            acceleration += Globals::GRAVITY_ACCELERATION * deltaTime;
        }
        else
        {
            if (velocity.Magnitude() == 0.0f)
            {
                // Static Friction
                float maximumFrictionMagnitude = Globals::LIMITING_STATIC_FRICTION_COEFFICIENT *
                                                 (-Globals::GRAVITY_ACCELERATION.y) * mass;
                Vector3F frictionForce = -acceleration * mass;

                if (frictionForce.Magnitude() > maximumFrictionMagnitude)
                {
                    frictionForce = -acceleration.Normalized() * maximumFrictionMagnitude;
                }

                acceleration += frictionForce / mass * deltaTime;
            }
            else
            {
                // Kinetic Friction
                float frictionMagnitude = Globals::KINETIC_FRICTION_COEFFICIENT *
                                         (-Globals::GRAVITY_ACCELERATION.y) * mass;
                Vector3F frictionForce = -velocity.Normalized() * frictionMagnitude;

                acceleration += frictionForce / mass * deltaTime;

                if (Vector3F::Dot(velocity, acceleration) < 0.0f)
                {
                    velocity = Vector3F(0.0f, 0.0f, 0.0f);
                }
            }
        }

        velocity += acceleration * deltaTime;

        jump = false;
        attack = false;
    }

private:
    void CheckIsOnGround()
    {
        Vector3F currentPos = transform->GetPosition();
        int blockTop = static_cast<int>(
            Terrain::GetInstance().GetGroundHeight(currentPos.x, currentPos.z)
        ) + 1;
        float pelvisGroundHeight = blockTop + Globals::PLAYER_HEIGHT;

        if (currentPos.y > pelvisGroundHeight)
        {
            isOnGround = false;
        }
        else
        {
            transform->SetPosition(
                Vector3F(currentPos.x, pelvisGroundHeight, currentPos.z)
            );
            isOnGround = true;
        }
    }
};

}

#endif // CHARACTER_CONTROLLER_COMPONENT_H