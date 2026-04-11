#ifndef QUESTFARERGAMEENGINE_WORLD_H
#define QUESTFARERGAMEENGINE_WORLD_H


#include "../scene/objects/GameObject.h"
#include "../voxel/Grid.h"
#include "../physics/PhysicsSystem.h"
#include "../player/Player.h"
#include "../rendering/LightingRenderer.h"
#include "../scene/components/PhysicsComponent.h"
#include "../scene/components/AIComponent.h"
#include "../rendering/VoxelRenderer.h"
#include "../scene/objects/CustomMeshObject.h"
#include "../scene/objects/AnimationModelObject.h"
#include "../scene/components/ECSManager.h"

using namespace logger;
using namespace player;
using namespace physics;
using namespace scene::components;


namespace world {
    class World {

    private:

        std::shared_ptr<Grid> grid = std::make_shared<Grid>();
        std::shared_ptr<PhysicsSystem> physicsSystem = std::make_shared<PhysicsSystem>();
        std::shared_ptr<Player> player;
        std::unique_ptr<LightingRenderer> lighting;



        std::shared_ptr<Logger> logger = std::make_shared<Logger>("world_log.txt");

        GLFWwindow *window;

        std::unique_ptr<GameObject> gameObjects[MAX_ENTITIES];
        int numEntities = 0;

        std::shared_ptr<ECSManager> ecsManager;

        void createVoxels() {
            auto voxelShader = std::make_unique<Shader>(
                    "../shader/vertex/voxel-shader.vs",
                    "../shader/fragment/voxel-shader.fs"
            );
            voxelShader->use();
//    voxelShader->setInt("texture_diffuse1", 0);
//
//    unsigned int voxelTexture;
//    createTexture(voxelTexture, "resources/images/voxel_atlas.png");


            auto voxelRenderer = std::make_unique<VoxelRenderer>(logger, std::move(voxelShader), grid);
            voxelRenderer->setup();

            gameObjects[numEntities] = std::make_unique<CustomMeshObject>(numEntities, std::move(voxelRenderer),
                                                                          Transform{});

            numEntities++;
        }





        void initialisePlayer(glm::vec3 pos) {



            std::shared_ptr<ModelAnimation> ourModel = std::make_unique<ModelAnimation>(
                    "resources/objects/Ch36_1001.dae");
            auto ourShader = std::make_unique<Shader>(
                    "../shader/vertex/anim_model.vs",
                    "../shader/fragment/anim_model.fs"
            );

            std::shared_ptr<animation::Animator> animator = std::make_unique<animation::Animator>();


            auto characterTransform = Transform{ourModel->getMeshVerticesCollapsed()};

            characterTransform.position = pos;

            gameObjects[numEntities] = std::make_unique<AnimationModelObject>(numEntities, ourModel,
                                                                              std::move(ourShader), characterTransform,
                                                                              animator);


            CharacterControllerComponent characterControllerComponent(numEntities, &gameObjects[numEntities]->transform);
            characterControllerComponent.initialize(ourModel, animator);
            characterControllerComponent.registerAnimation(CharacterControllerComponent::AnimationState::IDLE,"resources/objects/animation/humanoid/idle.dae");
            characterControllerComponent.registerAnimation(CharacterControllerComponent::AnimationState::WALK,"resources/objects/animation/humanoid/walking.dae");
            characterControllerComponent.registerAnimation(CharacterControllerComponent::AnimationState::RUN,"resources/objects/animation/humanoid/running.dae");
            characterControllerComponent.registerAnimation(CharacterControllerComponent::AnimationState::JUMP,"resources/objects/animation/humanoid/jump.dae");

            characterControllerComponent.switchAnimation(scene::components::CharacterControllerComponent::AnimationState::IDLE);
            ecsManager->addCharacterControllerComponent(numEntities, characterControllerComponent);


            player = std::make_shared<Player>(grid, &ecsManager->getCharacterControllerComponentFromSparse(numEntities));

            glfwSetWindowUserPointer(window, player.get());


            // Extract collision data before moving
            PhysicsComponent physicsComp(numEntities);

            physicsComp.addCollisionMeshesFromModel(ourModel->getMeshVertices(), ourModel->getMeshIndices());



            ecsManager->addPhysicsComponent(numEntities, physicsComp);
            numEntities++;

        }


        void createEntity(glm::vec3 pos) {
            std::unique_ptr<ModelAnimation> ourModel = std::make_unique<ModelAnimation>(
                    "resources/objects/animation/vampire/dancing_vampire.dae");
            auto ourShader = std::make_unique<Shader>(
                    "../shader/vertex/anim_model.vs",
                    "../shader/fragment/anim_model.fs"
            );

            std::unique_ptr<animation::Animation> danceAnimation = std::make_unique<animation::Animation>(
                    "resources/objects/animation/vampire/dancing_vampire.dae", ourModel.get());
            std::unique_ptr<animation::Animator> animator = std::make_unique<animation::Animator>(
                    std::move(danceAnimation));

            auto characterTransform = Transform{};
            characterTransform.position = pos;

            // Extract collision data before moving
            PhysicsComponent physicsComp(numEntities);

            physicsComp.addCollisionMeshesFromModel(ourModel->getMeshVertices(), ourModel->getMeshIndices());

            // Now move the model
            gameObjects[numEntities] = std::make_unique<AnimationModelObject>(numEntities, std::move(ourModel),
                                                                              std::move(ourShader), characterTransform,
                                                                              std::move(animator));

            ecsManager->addPhysicsComponent(numEntities, physicsComp);
            numEntities++;
        }

    public:

        World() {
            grid = std::make_shared<Grid>();
            ecsManager = std::make_shared<ECSManager>();
        }

        std::shared_ptr<Player> getPlayer() {
            return player;
        }


        void initialise(GLFWwindow *window) {
            this->window = window;

            createVoxels();
            initialisePlayer({30, 30, 30});
//            createEntity({20, 20, 20});
        }

        void update(double timeScale) {

            player->processInput(window, timeScale);
            player->updateCamera();

            ecsManager->update();

            physicsSystem->step(ecsManager->getPhysicsComponentsDense(), ecsManager->getPhysicsComponentsAmount(),
                                gameObjects, grid, timeScale * FIXED_TIMESTEP);
        }

        void render(double timeScale) {

            if (player) {
                RenderContext renderContext = player->getRenderContext();

                // Draw to screen.
                for (int i = 0; i < numEntities; i++) {
                    gameObjects[i]->draw(renderContext);
                }
            }
        }


    };
}

#endif //QUESTFARERGAMEENGINE_WORLD_H
