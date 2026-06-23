#ifndef QUESTFARERGAMEENGINE_ECSMANAGER_H
#define QUESTFARERGAMEENGINE_ECSMANAGER_H

#include "../../globals.h"
#include "PhysicsComponent.h"
#include "AIComponent.h"
#include "CharacterControllerComponent.h"
#include "AttackComponent.h"
#include "HealthComponent.h"

namespace scene::components {

    class ECSManager {
    private:
        PhysicsComponent physicsComponentsDense[MAX_ENTITIES];
        int physicsComponentsSparse[MAX_ENTITIES];
        int physicsComponentsAmount = 0;

        AIComponent aiComponentsDense[MAX_ENTITIES];
        int aiComponentsSparse[MAX_ENTITIES];
        int aiComponentsAmount = 0;

        CharacterControllerComponent characterControllerComponentsDense[MAX_ENTITIES];
        int characterControllerComponentsSparse[MAX_ENTITIES];
        int characterControllerComponentsAmount = 0;

        AttackComponent attackComponentsDense[MAX_ENTITIES];
        int attackComponentsSparse[MAX_ENTITIES];
        int attackComponentsAmount = 0;

        HealthComponent healthComponentsDense[MAX_ENTITIES];
        int healthComponentsSparse[MAX_ENTITIES];
        int healthComponentsAmount = 0;

    public:
        ECSManager();
        ~ECSManager();

        PhysicsComponent* getPhysicsComponentsDense();
        PhysicsComponent* getPhysicsComponent(int entityID);
        int getPhysicsComponentsAmount() const;
        void addPhysicsComponent(int entityID, const PhysicsComponent& component);
        void removePhysicsComponent(int entityID);

        const AIComponent* getAiComponentsDense() const;
        int getAiComponentsAmount() const;
        void addAIComponent(int entityID, const AIComponent& component);
        void removeAIComponent(int entityID);

        CharacterControllerComponent* getCharacterControllerComponentsDense();
        CharacterControllerComponent* getCharacterControllerComponent(int entityID);
        CharacterControllerComponent& getCharacterControllerComponentFromSparse(int entityID);
        int getCharacterControllerComponentsAmount() const;
        void addCharacterControllerComponent(int entityID, const CharacterControllerComponent& component);
        void removeCharacterControllerComponent(int entityID);

        AttackComponent* getAttackComponentsDense();
        AttackComponent* getAttackComponent(int entityID);
        int getAttackComponentsAmount() const;
        void addAttackComponent(int entityID, const AttackComponent& component);
        void removeAttackComponent(int entityID);

        HealthComponent* getHealthComponentsDense();
        HealthComponent* getHealthComponent(int entityID);
        int getHealthComponentsAmount() const;
        void addHealthComponent(int entityID, const HealthComponent& component);
        void removeHealthComponent(int entityID);
    };
}

#endif