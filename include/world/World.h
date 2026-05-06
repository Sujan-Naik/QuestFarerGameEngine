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
#include "../animation/AnimationAssetLibrary.h"

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
            // 1. Load the Asset Container (The "Mushed" .glb)
            // This loads the Mesh, Bones, and all Animation Clips in one pass.
            animation::AnimationAssetLibrary playerAssets;
//            playerAssets.loadFromGLB("resources/objects/UAL1_Standard.glb");
//            playerAssets.loadFromGLB("resources/objects/UAL2_Standard.glb");


//            playerAssets.loadFromFBX("resources/objects/humanoid/HumanM_Model.fbx", "Model");
//            playerAssets.loadFromFBX("resources/objects/humanoid/HumanM@Idle01.fbx", "Idle");
//            playerAssets.loadFromFBX("resources/objects/humanoid/HumanM@Run01_Forward [RM].fbx", "Run");


//            playerAssets.loadFromFBX("resources/objects/humanoid/HumanM@Walk01_Forward [RM].fbx", "Walk");


            playerAssets.loadFromFBX("resources/objects/humanoid/character.fbx");

            // Safety check: if the model didn't load, the rest will crash.
            if (!playerAssets.model) {
                std::cerr << "ERROR: Failed to load player assets from GLB!" << std::endl;
                return;
            }

            // 2. Setup Shader
            auto ourShader = std::make_unique<Shader>(
                    "../shader/vertex/anim_model.vs",
                    "../shader/fragment/anim_model.fs"
            );

            // 3. Setup World Transform
            // Use the model from the library to calculate initial bounds/transform
            auto characterTransform = Transform{playerAssets.model->getMeshVerticesCollapsed()};
            characterTransform.position = pos;
            characterTransform.scale = {0.1, 0.1, 0.1};

            int entityId = numEntities;
            auto sharedFSM = std::make_shared<AnimationFSM>();

            // 4. Setup Idle State
            auto idleState = std::make_shared<SimpleAnimationState>();
            idleState->animation = playerAssets.get("HumanM@Idle01");
            idleState->rootBoneNames = {"B-root"};

// 5. Setup the Locomotion State
            auto locState = std::make_shared<LocomotionBlendState>();
            locState->walk = playerAssets.get("HumanM@Walk01_Forward [RM]");
            locState->run = playerAssets.get("HumanM@Run01_Forward [RM]");
            locState->rootBoneNames = {"B-root"};

// Register both states
            sharedFSM->RegisterState("Idle", idleState);
            sharedFSM->RegisterState("Locomotion", locState);
            sharedFSM->TransitionTo("Idle");  // Start in idle


            // 5. Create the Game Object
            // We pass the model pointer directly from the library.
            gameObjects[entityId] = std::make_unique<AnimationModelObject>(
                    entityId,
                    playerAssets.model,
                    std::move(ourShader),
                    characterTransform,
                    sharedFSM
            );

            // 6. Setup Character Controller Component
            CharacterControllerComponent characterControllerComponent(
                    entityId,
                    &gameObjects[entityId]->transform,
                    sharedFSM
            );

            // Initialize with the model to sync bone maps
            characterControllerComponent.initialize(playerAssets.model);
            characterControllerComponent.setLocomotionSpeed(0.0f); // Default to Idle

            // Push to ECS
            ecsManager->addCharacterControllerComponent(entityId, characterControllerComponent);

            // 7. Setup Player Logic & Input
            player = std::make_shared<Player>(
                    grid,
                    &ecsManager->getCharacterControllerComponentFromSparse(entityId)
            );

            // Link GLFW input to the player class
            glfwSetWindowUserPointer(window, player.get());

            // 8. Physics Setup
            // Pull the raw mesh data from the GLB for the collision shapes
            PhysicsComponent physicsComp(entityId);
            physicsComp.addCollisionMeshesFromModel(
                    playerAssets.model->getMeshVertices(),
                    playerAssets.model->getMeshIndices()
            );

            ecsManager->addPhysicsComponent(entityId, physicsComp);

            numEntities++;

            std::cout << "Player initialized successfully with entity ID: " << entityId << std::endl;
        }

//        void createEntity(glm::vec3 pos) {
//            std::unique_ptr<ModelAnimation> ourModel = std::make_unique<ModelAnimation>(
//                    "resources/objects/animation/vampire/dancing_vampire.dae");
//            auto ourShader = std::make_unique<Shader>(
//                    "../shader/vertex/anim_model.vs",
//                    "../shader/fragment/anim_model.fs"
//            );
//
//            std::unique_ptr<animation::Animation> danceAnimation = std::make_unique<animation::Animation>(
//                    "resources/objects/animation/vampire/dancing_vampire.dae", ourModel.get());
//            std::unique_ptr<animation::Animator> animator = std::make_unique<animation::Animator>(
//                    std::move(danceAnimation));
//
//            auto characterTransform = Transform{};
//            characterTransform.position = pos;
//
//            // Extract collision data before moving
//            PhysicsComponent physicsComp(numEntities);
//
//            physicsComp.addCollisionMeshesFromModel(ourModel->getMeshVertices(), ourModel->getMeshIndices());
//
//            // Now move the model
//            gameObjects[numEntities] = std::make_unique<AnimationModelObject>(numEntities, std::move(ourModel),
//                                                                              std::move(ourShader), characterTransform,
//                                                                              std::move(animator));
//
//            ecsManager->addPhysicsComponent(numEntities, physicsComp);
//            numEntities++;
//        }

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
            initialisePlayer({30, 100, 30});
//            createEntity({20, 20, 20});
        }

        void update(double timeScale) {

            player->processInput(window, timeScale);

            ecsManager->update();
            player->updateCamera();


            physicsSystem->step(ecsManager->getPhysicsComponentsDense(), ecsManager->getPhysicsComponentsAmount(),
                                gameObjects, grid, timeScale * FIXED_TIMESTEP);
        }

        void render(double timeScale) {

            if (player) {
                RenderContext renderContext = player->getRenderContext();

                for (int i = 0; i < numEntities; i++) {
                    gameObjects[i]->draw(renderContext);
                }
            }
        }


    };
}

#endif //QUESTFARERGAMEENGINE_WORLD_H
