#ifndef QUESTFARERGAMEENGINE_ECSMANAGER_H
#define QUESTFARERGAMEENGINE_ECSMANAGER_H

#include "../../globals.h"
#include "PhysicsComponent.h"
#include "AIComponent.h"
#include "CharacterControllerComponent.h"


namespace scene::components{

    class ECSManager{

    private:

        // ECS State

        PhysicsComponent physicsComponentsDense[MAX_ENTITIES];
        int physicsComponentsSparse[MAX_ENTITIES];
        int physicsComponentsAmount = 0;

        AIComponent aiComponentsDense[MAX_ENTITIES];
        int aiComponentsSparse[MAX_ENTITIES];
        int aiComponentsAmount = 0;


        CharacterControllerComponent characterControllerComponentsDense[MAX_ENTITIES];
        int characterControllerComponentsSparse[MAX_ENTITIES];
        int characterControllerComponentsAmount = 0;

    public:

        ECSManager() {}

        PhysicsComponent *getPhysicsComponentsDense() {
            return physicsComponentsDense;
        }

        int getPhysicsComponentsAmount() const {
            return physicsComponentsAmount;
        }


        const AIComponent *getAiComponentsDense() const {
            return aiComponentsDense;
        }

        void addAIComponent(int entityID, const AIComponent &component) {
            aiComponentsSparse[entityID] = aiComponentsAmount;
            aiComponentsDense[aiComponentsAmount] = component;
            aiComponentsAmount++;
        }

        void removeAIComponent(int entityID) {
            int denseIndex = aiComponentsSparse[entityID];

            aiComponentsDense[denseIndex] = aiComponentsDense[aiComponentsAmount - 1];
            aiComponentsSparse[entityID] = -1;

            aiComponentsAmount--;
        }


        void addPhysicsComponent(int entityID, const PhysicsComponent &component) {
            physicsComponentsSparse[entityID] = physicsComponentsAmount;
            physicsComponentsDense[physicsComponentsAmount] = component;
            physicsComponentsAmount++;
        }

        void removePhysicsComponent(int entityID) {
            int denseIndex = physicsComponentsSparse[entityID];

            physicsComponentsDense[denseIndex] = physicsComponentsDense[physicsComponentsAmount - 1];
            physicsComponentsSparse[entityID] = -1;

            physicsComponentsAmount--;
        }


        CharacterControllerComponent& getCharacterControllerComponentFromSparse(int entityID) {
            return characterControllerComponentsDense[characterControllerComponentsSparse[entityID]];
        }


        void addCharacterControllerComponent(int entityID, const CharacterControllerComponent &component) {
            characterControllerComponentsSparse[entityID] = characterControllerComponentsAmount;
            characterControllerComponentsDense[characterControllerComponentsAmount] = component;
            characterControllerComponentsAmount++;
        }

        void removeCharacterControllerComponent(int entityID) {
            int denseIndex = characterControllerComponentsSparse[entityID];

            characterControllerComponentsDense[denseIndex] = characterControllerComponentsDense[characterControllerComponentsAmount - 1];
            characterControllerComponentsSparse[entityID] = -1;

            characterControllerComponentsAmount--;
        }


        void update(){
            // Process AI.

            for (int i = 0; i < aiComponentsAmount; i++) {
                aiComponentsDense[i].update();
            }


            // Update physics.
            for (int i = 0; i < physicsComponentsAmount; i++) {
                physicsComponentsDense[i].update();
            }

            // Update physics.
            for (int i = 0; i < characterControllerComponentsAmount; i++) {
                characterControllerComponentsDense[i].update();
            }
        }
    };
}

#endif //QUESTFARERGAMEENGINE_ECSMANAGER_H
