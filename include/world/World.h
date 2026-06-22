#ifndef QUESTFARERGAMEENGINE_WORLD_H
#define QUESTFARERGAMEENGINE_WORLD_H

#include "../scene/objects/GameObject.h"
#include "../voxel/Grid.h"
#include "../scene/systems/PhysicsSystem.h"
#include "../scene/systems/CharacterControllerSystem.h"
#include "../scene/systems/AISystem.h"
#include "../player/Player.h"
#include "../rendering/LightingRenderer.h"
#include "../scene/components/PhysicsComponent.h"
#include "../scene/components/AIComponent.h"
#include "../rendering/VoxelRenderer.h"
#include "../scene/objects/CustomMeshObject.h"
#include "../scene/objects/AnimationModelObject.h"
#include "../scene/components/ECSManager.h"
#include "../animation/AnimationAssetLibrary.h"
#include "ProceduralTerrainGenerator.h"
#include "../scene/components/fsm/ConcreteStates.h"
#include "../scene/components/fsm/ConcreteTransitions.h"
#include <set>
#include <future>
#include <queue>
#include <mutex>

using namespace logger;
using namespace player;
using namespace physics;
using namespace scene::components;

namespace world {
    class World {
    private:
        std::shared_ptr<voxel::Grid> grid = std::make_shared<voxel::Grid>();
        std::shared_ptr<PhysicsSystem> physicsSystem = std::make_shared<PhysicsSystem>();
        std::shared_ptr<CharacterControllerSystem> characterControllerSystem = std::make_shared<CharacterControllerSystem>();
        std::shared_ptr<AISystem> aiSystem = std::make_shared<AISystem>();
        std::shared_ptr<Player> player;
        std::unique_ptr<LightingRenderer> lighting;
        std::shared_ptr<Logger> logger = std::make_shared<Logger>("world_log.txt");

        GLFWwindow *window;
        std::unique_ptr<GameObject> gameObjects[MAX_ENTITIES];
        int numEntities = 0;

        std::shared_ptr<ECSManager> ecsManager;
        std::shared_ptr<ProceduralTerrainGenerator> terrainGenerator;

        struct ChunkPos {
            int x, z;
            bool operator<(const ChunkPos& other) const {
                if (x != other.x) return x < other.x;
                return z < other.z;
            }
        };

        struct PendingChunk {
            ChunkPos pos;
            std::unique_ptr<voxel::Chunk> chunkData;
            std::vector<rendering::mesh::Vertex> vertices;
            std::vector<unsigned int> indices;
        };

        std::set<ChunkPos> activeChunks;
        std::set<ChunkPos> processingChunks;
        std::queue<std::future<PendingChunk>> generationFutures;

        rendering::VoxelRenderer* voxelRendererPtr = nullptr;
        const int RENDER_DISTANCE = 6;
        const int UNLOAD_MARGIN = 2;

        void createVoxels() {
            auto voxelShader = std::make_unique<Shader>(
                    "../shader/vertex/voxel-shader.vs",
                    "../shader/fragment/voxel-shader.fs"
            );
            auto vRenderer = std::make_unique<rendering::VoxelRenderer>(logger, std::move(voxelShader), grid);
            vRenderer->loadTextureAtlas("resources/textures/terrain_atlas.png");
            voxelRendererPtr = vRenderer.get();

            gameObjects[numEntities] = std::make_unique<CustomMeshObject>(
                    numEntities,
                    std::move(vRenderer),
                    Transform{}
            );
            numEntities++;
        }

        void updateTerrainStreaming() {
            if (!player) return;

            int uploadsThisFrame = 0;
            const int MAX_UPLOADS_PER_FRAME = 1;

            while (!generationFutures.empty() && uploadsThisFrame < MAX_UPLOADS_PER_FRAME) {
                auto& fut = generationFutures.front();
                if (fut.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                    PendingChunk result = fut.get();
                    grid->chunks[glm::ivec2(result.pos.x, result.pos.z)] = std::move(result.chunkData);

                    if (voxelRendererPtr && !result.vertices.empty()) {
                        voxelRendererPtr->uploadManualMesh(result.pos.x, result.pos.z, result.vertices, result.indices);
                        uploadsThisFrame++;
                    }

                    activeChunks.insert(result.pos);
                    processingChunks.erase(result.pos);
                    generationFutures.pop();
                } else {
                    break;
                }
            }

            glm::vec3 pPos = player->getPosition(*ecsManager);
            int pChunkX = static_cast<int>(pPos.x) >> 4;
            int pChunkZ = static_cast<int>(pPos.z) >> 4;

            std::vector<ChunkPos> targets;
            for (int x = -RENDER_DISTANCE; x <= RENDER_DISTANCE; ++x) {
                for (int z = -RENDER_DISTANCE; z <= RENDER_DISTANCE; ++z) {
                    ChunkPos pos{pChunkX + x, pChunkZ + z};
                    if (activeChunks.find(pos) == activeChunks.end() && processingChunks.find(pos) == processingChunks.end()) {
                        targets.push_back(pos);
                    }
                }
            }

            std::sort(targets.begin(), targets.end(), [&](const ChunkPos& a, const ChunkPos& b) {
                return (std::abs(a.x - pChunkX) + std::abs(a.z - pChunkZ)) <
                       (std::abs(b.x - pChunkX) + std::abs(b.z - pChunkZ));
            });

            const int MAX_CONCURRENT_THREADS = 2;
            for (const auto& pos : targets) {
                if (generationFutures.size() >= MAX_CONCURRENT_THREADS) break;

                processingChunks.insert(pos);
                generationFutures.push(std::async(std::launch::async, [this, pos]() {
                    auto newChunk = std::make_unique<voxel::Chunk>();
                    terrainGenerator->generateChunkData(newChunk.get(), pos.x, pos.z);

                    std::vector<rendering::mesh::Vertex> v;
                    std::vector<unsigned int> i;
                    if (voxelRendererPtr) {
                        voxelRendererPtr->greedyMeshSpecificChunk(newChunk.get(), v, i);
                    }

                    return PendingChunk{pos, std::move(newChunk), std::move(v), std::move(i)};
                }));
            }

            int unloadDist = RENDER_DISTANCE + UNLOAD_MARGIN;
            for (auto it = activeChunks.begin(); it != activeChunks.end(); ) {
                int dx = std::abs(it->x - pChunkX);
                int dz = std::abs(it->z - pChunkZ);
                if (dx > unloadDist || dz > unloadDist) {
                    grid->chunks.erase({it->x, it->z});
                    if (voxelRendererPtr) voxelRendererPtr->unloadChunkMesh(it->x, it->z);
                    it = activeChunks.erase(it);
                } else {
                    ++it;
                }
            }
        }

        void initialisePlayer(glm::vec3 pos) {
            animation::AnimationAssetLibrary playerAssets;
            playerAssets.loadFromFBX("resources/objects/humanoid/character.fbx");
            if (!playerAssets.model) return;

            auto ourShader = std::make_unique<Shader>(
                    "../shader/vertex/anim_model.vs",
                    "../shader/fragment/anim_model.fs"
            );

            auto characterTransform = Transform{playerAssets.model->getMeshVerticesCollapsed()};
            characterTransform.position = pos;
            characterTransform.scale = {0.05f, 0.05f, 0.05f};

            int entityId = numEntities;
            auto sharedFSM = std::make_shared<AnimationFSM>();

            auto idleClip = std::make_unique<ClipNode>(playerAssets.get("HumanM@Idle01"));
            auto idleAnimState = std::make_shared<AnimationState>(std::move(idleClip));
            idleAnimState->rootBoneNames = {"B-root"};

            auto multiDirectionalTree = std::make_unique<DirectionalBlendTree2D>();
            multiDirectionalTree->AddNode(std::make_unique<ClipNode>(playerAssets.get("HumanM@Walk01_Forward [RM]")),      glm::vec2(0.0f, 0.5f));
            multiDirectionalTree->AddNode(std::make_unique<ClipNode>(playerAssets.get("HumanM@Run01_Forward [RM]")),       glm::vec2(0.0f, 1.0f));
            multiDirectionalTree->AddNode(std::make_unique<ClipNode>(playerAssets.get("HumanM@Walk01_Backward [RM]")),     glm::vec2(0.0f, -0.5f));
            multiDirectionalTree->AddNode(std::make_unique<ClipNode>(playerAssets.get("HumanM@Run01_Backward [RM]")),      glm::vec2(0.0f, -1.0f));
            multiDirectionalTree->AddNode(std::make_unique<ClipNode>(playerAssets.get("HumanM@Walk01_Left [RM]")),         glm::vec2(-0.5f, 0.0f));
            multiDirectionalTree->AddNode(std::make_unique<ClipNode>(playerAssets.get("HumanM@Run01_Left [RM]")),          glm::vec2(-1.0f, 0.0f));
            multiDirectionalTree->AddNode(std::make_unique<ClipNode>(playerAssets.get("HumanM@Walk01_Right [RM]")),        glm::vec2(0.5f, 0.0f));
            multiDirectionalTree->AddNode(std::make_unique<ClipNode>(playerAssets.get("HumanM@Run01_Right [RM]")),         glm::vec2(1.0f, 0.0f));
            multiDirectionalTree->AddNode(std::make_unique<ClipNode>(playerAssets.get("HumanM@Walk01_ForwardLeft [RM]")),  glm::vec2(-0.5f, 0.5f));
            multiDirectionalTree->AddNode(std::make_unique<ClipNode>(playerAssets.get("HumanM@Walk01_ForwardRight [RM]")), glm::vec2(0.5f, 0.5f));
            multiDirectionalTree->AddNode(std::make_unique<ClipNode>(playerAssets.get("HumanM@Walk01_BackwardLeft [RM]")), glm::vec2(-0.5f, -0.5f));
            multiDirectionalTree->AddNode(std::make_unique<ClipNode>(playerAssets.get("HumanM@Walk01_BackwardRight [RM]")),glm::vec2(0.5f, -0.5f));
            multiDirectionalTree->AddNode(std::make_unique<ClipNode>(playerAssets.get("HumanM@Run01_ForwardLeft [RM]")),   glm::vec2(-0.707f, 0.707f));
            multiDirectionalTree->AddNode(std::make_unique<ClipNode>(playerAssets.get("HumanM@Run01_ForwardRight [RM]")),  glm::vec2(0.707f, 0.707f));
            multiDirectionalTree->AddNode(std::make_unique<ClipNode>(playerAssets.get("HumanM@Run01_BackwardLeft [RM]")),  glm::vec2(-0.707f, -0.707f));
            multiDirectionalTree->AddNode(std::make_unique<ClipNode>(playerAssets.get("HumanM@Run01_BackwardRight [RM]")), glm::vec2(0.707f, -0.707f));

            auto locAnimState = std::make_shared<AnimationState>(std::move(multiDirectionalTree));
            locAnimState->rootBoneNames = {"B-root"};

            auto jumpClip = std::make_unique<ClipNode>(playerAssets.get("HumanM@Jump01 [RM]"));
            auto jumpAnimState = std::make_shared<AnimationState>(std::move(jumpClip));
            jumpAnimState->rootBoneNames = {"B-root"};

            gameObjects[entityId] = std::make_unique<AnimationModelObject>(
                    entityId, playerAssets.model, std::move(ourShader), characterTransform, sharedFSM
            );

            // --- CRITICAL FIX: Add component to registry before passing context addresses ---
            CharacterControllerComponent cc(entityId, &gameObjects[entityId]->transform, sharedFSM);
            cc.skeleton = playerAssets.model;
            ecsManager->addCharacterControllerComponent(entityId, cc);

            // Fetch permanent stable heap location pointer
            CharacterControllerComponent* permanentCcPtr = &ecsManager->getCharacterControllerComponentFromSparse(entityId);

            auto stateIdle = std::make_shared<fsm::IdleState>(permanentCcPtr, idleAnimState);
            auto stateLocomotion = std::make_shared<fsm::LocomotionState>(permanentCcPtr, locAnimState);
            auto stateJump = std::make_shared<fsm::JumpState>(permanentCcPtr, jumpAnimState);

            fsm::AnimationFSM::TransitionMap transitions;

            transitions[stateIdle].push_back({stateLocomotion, std::make_shared<fsm::IdleToLocomotionTransition>(permanentCcPtr)});
            transitions[stateIdle].push_back({stateJump, std::make_shared<fsm::AnyToJumpTransition>(permanentCcPtr, *ecsManager)});

            transitions[stateLocomotion].push_back({stateIdle, std::make_shared<fsm::LocomotionToIdleTransition>(permanentCcPtr)});
            transitions[stateLocomotion].push_back({stateJump, std::make_shared<fsm::AnyToJumpTransition>(permanentCcPtr, *ecsManager)});

            transitions[stateJump].push_back({stateLocomotion, std::make_shared<fsm::JumpToFallbackTransition>(permanentCcPtr, stateJump)});
            transitions[stateJump].push_back({stateIdle, std::make_shared<fsm::JumpToFallbackTransition>(permanentCcPtr, stateJump)});

            sharedFSM->Initialize(transitions, stateIdle);

            player = std::make_shared<Player>(grid, entityId);
            glfwSetWindowUserPointer(window, player.get());

            PhysicsComponent pc(entityId);
            pc.addModel(playerAssets.model);
            ecsManager->addPhysicsComponent(entityId, pc);
            numEntities++;
        }

        void initialiseNPC(glm::vec3 pos) {
            animation::AnimationAssetLibrary playerAssets;
            playerAssets.loadFromFBX("resources/objects/humanoid/character.fbx");
            if (!playerAssets.model) return;

            auto ourShader = std::make_unique<Shader>(
                    "../shader/vertex/anim_model.vs",
                    "../shader/fragment/anim_model.fs"
            );

            auto characterTransform = Transform{playerAssets.model->getMeshVerticesCollapsed()};
            characterTransform.position = pos;
            characterTransform.scale = {0.05f, 0.05f, 0.05f};

            int entityId = numEntities;
            auto sharedFSM = std::make_shared<AnimationFSM>();

            auto idleClip = std::make_unique<ClipNode>(playerAssets.get("HumanM@Idle01"));
            auto idleAnimState = std::make_shared<AnimationState>(std::move(idleClip));
            idleAnimState->rootBoneNames = {"B-root"};

            auto multiDirectionalTree = std::make_unique<DirectionalBlendTree2D>();
            multiDirectionalTree->AddNode(std::make_unique<ClipNode>(playerAssets.get("HumanM@Walk01_Forward [RM]")),      glm::vec2(0.0f, 0.5f));
            multiDirectionalTree->AddNode(std::make_unique<ClipNode>(playerAssets.get("HumanM@Run01_Forward [RM]")),       glm::vec2(0.0f, 1.0f));
            multiDirectionalTree->AddNode(std::make_unique<ClipNode>(playerAssets.get("HumanM@Walk01_Backward [RM]")),     glm::vec2(0.0f, -0.5f));
            multiDirectionalTree->AddNode(std::make_unique<ClipNode>(playerAssets.get("HumanM@Run01_Backward [RM]")),      glm::vec2(0.0f, -1.0f));
            multiDirectionalTree->AddNode(std::make_unique<ClipNode>(playerAssets.get("HumanM@Walk01_Left [RM]")),         glm::vec2(-0.5f, 0.0f));
            multiDirectionalTree->AddNode(std::make_unique<ClipNode>(playerAssets.get("HumanM@Run01_Left [RM]")),          glm::vec2(-1.0f, 0.0f));
            multiDirectionalTree->AddNode(std::make_unique<ClipNode>(playerAssets.get("HumanM@Walk01_Right [RM]")),        glm::vec2(0.5f, 0.0f));
            multiDirectionalTree->AddNode(std::make_unique<ClipNode>(playerAssets.get("HumanM@Run01_Right [RM]")),         glm::vec2(1.0f, 0.0f));
            multiDirectionalTree->AddNode(std::make_unique<ClipNode>(playerAssets.get("HumanM@Walk01_ForwardLeft [RM]")),  glm::vec2(-0.5f, 0.5f));
            multiDirectionalTree->AddNode(std::make_unique<ClipNode>(playerAssets.get("HumanM@Walk01_ForwardRight [RM]")), glm::vec2(0.5f, 0.5f));
            multiDirectionalTree->AddNode(std::make_unique<ClipNode>(playerAssets.get("HumanM@Walk01_BackwardLeft [RM]")), glm::vec2(-0.5f, -0.5f));
            multiDirectionalTree->AddNode(std::make_unique<ClipNode>(playerAssets.get("HumanM@Walk01_BackwardRight [RM]")),glm::vec2(0.5f, -0.5f));
            multiDirectionalTree->AddNode(std::make_unique<ClipNode>(playerAssets.get("HumanM@Run01_ForwardLeft [RM]")),   glm::vec2(-0.707f, 0.707f));
            multiDirectionalTree->AddNode(std::make_unique<ClipNode>(playerAssets.get("HumanM@Run01_ForwardRight [RM]")),  glm::vec2(0.707f, 0.707f));
            multiDirectionalTree->AddNode(std::make_unique<ClipNode>(playerAssets.get("HumanM@Run01_BackwardLeft [RM]")),  glm::vec2(-0.707f, -0.707f));
            multiDirectionalTree->AddNode(std::make_unique<ClipNode>(playerAssets.get("HumanM@Run01_BackwardRight [RM]")), glm::vec2(0.707f, -0.707f));

            auto locAnimState = std::make_shared<AnimationState>(std::move(multiDirectionalTree));
            locAnimState->rootBoneNames = {"B-root"};

            auto jumpClip = std::make_unique<ClipNode>(playerAssets.get("HumanM@Jump01 [RM]"));
            auto jumpAnimState = std::make_shared<AnimationState>(std::move(jumpClip));
            jumpAnimState->rootBoneNames = {"B-root"};

            gameObjects[entityId] = std::make_unique<AnimationModelObject>(
                    entityId, playerAssets.model, std::move(ourShader), characterTransform, sharedFSM
            );

            // --- CRITICAL FIX: Add component to registry before passing context addresses ---
            CharacterControllerComponent cc(entityId, &gameObjects[entityId]->transform, sharedFSM);
            cc.skeleton = playerAssets.model;
            ecsManager->addCharacterControllerComponent(entityId, cc);

            // Fetch permanent stable heap location pointer
            CharacterControllerComponent* permanentCcPtr = &ecsManager->getCharacterControllerComponentFromSparse(entityId);

            auto stateIdle = std::make_shared<fsm::IdleState>(permanentCcPtr, idleAnimState);
            auto stateLocomotion = std::make_shared<fsm::LocomotionState>(permanentCcPtr, locAnimState);
            auto stateJump = std::make_shared<fsm::JumpState>(permanentCcPtr, jumpAnimState);

            fsm::AnimationFSM::TransitionMap transitions;

            transitions[stateIdle].push_back({stateLocomotion, std::make_shared<fsm::IdleToLocomotionTransition>(permanentCcPtr)});
            transitions[stateIdle].push_back({stateJump, std::make_shared<fsm::AnyToJumpTransition>(permanentCcPtr, *ecsManager)});

            transitions[stateLocomotion].push_back({stateIdle, std::make_shared<fsm::LocomotionToIdleTransition>(permanentCcPtr)});
            transitions[stateLocomotion].push_back({stateJump, std::make_shared<fsm::AnyToJumpTransition>(permanentCcPtr, *ecsManager)});

            transitions[stateJump].push_back({stateLocomotion, std::make_shared<fsm::JumpToFallbackTransition>(permanentCcPtr, stateJump)});
            transitions[stateJump].push_back({stateIdle, std::make_shared<fsm::JumpToFallbackTransition>(permanentCcPtr, stateJump)});

            sharedFSM->Initialize(transitions, stateIdle);

            PhysicsComponent pc(entityId);
            pc.addModel(playerAssets.model);
            ecsManager->addPhysicsComponent(entityId, pc);

            AIComponent ac(entityId);
            ecsManager->addAIComponent(entityId, ac);

            numEntities++;
        }

    public:
        World() {
            grid = std::make_shared<voxel::Grid>();
            ecsManager = std::make_shared<ECSManager>();
            terrainGenerator = std::make_shared<ProceduralTerrainGenerator>();
        }

        std::shared_ptr<Player> getPlayer() { return player; }

        void initialise(GLFWwindow *window) {
            this->window = window;
            initialisePlayer({30, 100, 30});
            initialiseNPC({30, 100, 40});
            createVoxels();
            updateTerrainStreaming();
        }

        void update(double timeScale) {
            float dt = static_cast<float>(timeScale * FIXED_TIMESTEP);
            player->processInput(window, timeScale, *ecsManager);
            updateTerrainStreaming();
            aiSystem->updateAI(*ecsManager, grid, dt);
            characterControllerSystem->update(*ecsManager, dt);
            physicsSystem->step(
                    ecsManager->getPhysicsComponentsDense(),
                    ecsManager->getPhysicsComponentsAmount(),
                    gameObjects, grid, dt
            );
            player->updateCamera(*ecsManager);
        }

        void render(double timeScale) {
            if (player) {
                RenderContext ctx = player->getRenderContext();
                for (int i = 0; i < numEntities; i++) {
                    gameObjects[i]->draw(ctx);
                }
            }
        }
    };
}

#endif