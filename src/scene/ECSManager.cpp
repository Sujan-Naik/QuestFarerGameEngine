#include "../../include/scene/components/ECSManager.h"

namespace scene::components {

    ECSManager::ECSManager() {
        for (int i = 0; i < MAX_ENTITIES; ++i) {
            physicsComponentsSparse[i] = -1;
            aiComponentsSparse[i] = -1;
            characterControllerComponentsSparse[i] = -1;
        }
    }

    ECSManager::~ECSManager() = default;

    PhysicsComponent* ECSManager::getPhysicsComponentsDense() {
        return physicsComponentsDense;
    }

    PhysicsComponent* ECSManager::getPhysicsComponent(int entityID) {
        int denseIndex = physicsComponentsSparse[entityID];
        return &physicsComponentsDense[denseIndex];
    }

    int ECSManager::getPhysicsComponentsAmount() const {
        return physicsComponentsAmount;
    }

    void ECSManager::addPhysicsComponent(int entityID, const PhysicsComponent& component) {
        physicsComponentsSparse[entityID] = physicsComponentsAmount;
        physicsComponentsDense[physicsComponentsAmount] = component;
        physicsComponentsAmount++;
    }

    void ECSManager::removePhysicsComponent(int entityID) {
        int denseIndex = physicsComponentsSparse[entityID];
        physicsComponentsDense[denseIndex] = physicsComponentsDense[physicsComponentsAmount - 1];
        physicsComponentsSparse[entityID] = -1;
        physicsComponentsAmount--;
    }

    const AIComponent* ECSManager::getAiComponentsDense() const {
        return aiComponentsDense;
    }

    void ECSManager::addAIComponent(int entityID, const AIComponent& component) {
        aiComponentsSparse[entityID] = aiComponentsAmount;
        aiComponentsDense[aiComponentsAmount] = component;
        aiComponentsAmount++;
    }

    void ECSManager::removeAIComponent(int entityID) {
        int denseIndex = aiComponentsSparse[entityID];
        aiComponentsDense[denseIndex] = aiComponentsDense[aiComponentsAmount - 1];
        aiComponentsSparse[entityID] = -1;
        aiComponentsAmount--;
    }

    CharacterControllerComponent& ECSManager::getCharacterControllerComponentFromSparse(int entityID) {
        return characterControllerComponentsDense[characterControllerComponentsSparse[entityID]];
    }

    void ECSManager::addCharacterControllerComponent(int entityID, const CharacterControllerComponent& component) {

        characterControllerComponentsSparse[entityID] = characterControllerComponentsAmount;
        characterControllerComponentsDense[characterControllerComponentsAmount] = component;
        characterControllerComponentsAmount++;
    }

    void ECSManager::removeCharacterControllerComponent(int entityID) {
        int denseIndex = characterControllerComponentsSparse[entityID];
        characterControllerComponentsDense[denseIndex] = characterControllerComponentsDense[characterControllerComponentsAmount - 1];
        characterControllerComponentsSparse[entityID] = -1;
        characterControllerComponentsAmount--;
    }

    void ECSManager::update() {
        for (int i = 0; i < aiComponentsAmount; i++) {
            aiComponentsDense[i].update();
        }

        for (int i = 0; i < characterControllerComponentsAmount; i++) {
            characterControllerComponentsDense[i].update();
        }

        for (int i = 0; i < physicsComponentsAmount; i++) {
            physicsComponentsDense[i].update();
        }
    }
}