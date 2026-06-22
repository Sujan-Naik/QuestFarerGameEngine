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
        if (denseIndex == -1) return nullptr;
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
        if (denseIndex == -1) return;

        int lastDenseIndex = physicsComponentsAmount - 1;
        if (denseIndex != lastDenseIndex) {
            physicsComponentsDense[denseIndex] = physicsComponentsDense[lastDenseIndex];
            int movedEntityID = physicsComponentsDense[denseIndex].getEntityId();
            physicsComponentsSparse[movedEntityID] = denseIndex;
        }

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
        if (denseIndex == -1) return;

        int lastDenseIndex = aiComponentsAmount - 1;
        if (denseIndex != lastDenseIndex) {
            aiComponentsDense[denseIndex] = aiComponentsDense[lastDenseIndex];
            int movedEntityID = aiComponentsDense[denseIndex].getEntityId();
            aiComponentsSparse[movedEntityID] = denseIndex;
        }

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
        if (denseIndex == -1) return;

        int lastDenseIndex = characterControllerComponentsAmount - 1;
        if (denseIndex != lastDenseIndex) {
            characterControllerComponentsDense[denseIndex] = characterControllerComponentsDense[lastDenseIndex];
            int movedEntityID = characterControllerComponentsDense[denseIndex].getEntityId();
            characterControllerComponentsSparse[movedEntityID] = denseIndex;
        }

        characterControllerComponentsSparse[entityID] = -1;
        characterControllerComponentsAmount--;
    }

    CharacterControllerComponent* ECSManager::getCharacterControllerComponentsDense() {
        return characterControllerComponentsDense;
    }

    int ECSManager::getCharacterControllerComponentsAmount() const {
        return characterControllerComponentsAmount;
    }

    int ECSManager::getAiComponentsAmount() const {
        return aiComponentsAmount;
    }
}