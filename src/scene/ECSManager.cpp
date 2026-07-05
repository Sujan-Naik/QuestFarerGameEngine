
#include "../../include/scene/components/ECSManager.h"

namespace scene::components {

    ECSManager::ECSManager() {
        for (int i = 0; i < MAX_ENTITIES; ++i) {
            physicsComponentsSparse[i] = -1;
            aiComponentsSparse[i] = -1;
            characterControllerComponentsSparse[i] = -1;
            attackComponentsSparse[i] = -1;
            healthComponentsSparse[i] = -1;
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

    CharacterControllerComponent* ECSManager::getCharacterControllerComponent(int entityID) {
        int denseIndex = characterControllerComponentsSparse[entityID];
        if (denseIndex == -1) return nullptr;
        return &characterControllerComponentsDense[denseIndex];
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

    AttackComponent* ECSManager::getAttackComponentsDense() {
        return attackComponentsDense;
    }

    AttackComponent* ECSManager::getAttackComponent(int entityID) {
        int denseIndex = attackComponentsSparse[entityID];
        if (denseIndex == -1) return nullptr;
        return &attackComponentsDense[denseIndex];
    }

    int ECSManager::getAttackComponentsAmount() const {
        return attackComponentsAmount;
    }

    void ECSManager::addAttackComponent(int entityID, const AttackComponent& component) {
        attackComponentsSparse[entityID] = attackComponentsAmount;
        attackComponentsDense[attackComponentsAmount] = component;
        attackComponentsAmount++;
    }

    void ECSManager::removeAttackComponent(int entityID) {
        int denseIndex = attackComponentsSparse[entityID];
        if (denseIndex == -1) return;

        int lastDenseIndex = attackComponentsAmount - 1;
        if (denseIndex != lastDenseIndex) {
            attackComponentsDense[denseIndex] = attackComponentsDense[lastDenseIndex];
            int movedEntityID = attackComponentsDense[denseIndex].getEntityId();
            attackComponentsSparse[movedEntityID] = denseIndex;
        }

        attackComponentsSparse[entityID] = -1;
        attackComponentsAmount--;
    }

    HealthComponent* ECSManager::getHealthComponentsDense() {
        return healthComponentsDense;
    }

    HealthComponent* ECSManager::getHealthComponent(int entityID) {
        int denseIndex = healthComponentsSparse[entityID];
        if (denseIndex == -1) return nullptr;
        return &healthComponentsDense[denseIndex];
    }

    int ECSManager::getHealthComponentsAmount() const {
        return healthComponentsAmount;
    }

    void ECSManager::addHealthComponent(int entityID, const HealthComponent& component) {
        healthComponentsSparse[entityID] = healthComponentsAmount;
        healthComponentsDense[healthComponentsAmount] = component;
        healthComponentsAmount++;
    }

    void ECSManager::removeHealthComponent(int entityID) {
        int denseIndex = healthComponentsSparse[entityID];
        if (denseIndex == -1) return;

        int lastDenseIndex = healthComponentsAmount - 1;
        if (denseIndex != lastDenseIndex) {
            healthComponentsDense[denseIndex] = healthComponentsDense[lastDenseIndex];
            int movedEntityID = healthComponentsDense[denseIndex].getEntityId();
            healthComponentsSparse[movedEntityID] = denseIndex;
        }

        healthComponentsSparse[entityID] = -1;
        healthComponentsAmount--;
    }
}