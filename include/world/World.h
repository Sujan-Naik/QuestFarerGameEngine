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
#include "../scene/systems/AttackSystem.h"
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
        std::shared_ptr<AttackSystem> attackSystem = std::make_shared<AttackSystem>();
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

                    glm::ivec2 chunkPos2D(result.pos.x, result.pos.z);
                    grid->chunks[chunkPos2D] = std::move(result.chunkData);

                    int physicsCount = ecsManager->getPhysicsComponentsAmount();
                    const auto* physicsComponents = ecsManager->getPhysicsComponentsDense();

                    for (int e = 0; e < physicsCount; ++e) {
                        const auto& comp = physicsComponents[e];
                        if (comp.getEntityId() == -1) continue;

                        GameObject* obj = gameObjects[comp.getEntityId()].get();
                        if (!obj) continue;

                        glm::vec3 pos = obj->getPosition();
                        glm::ivec2 entityChunk = glm::ivec2(
                                static_cast<int>(std::floor(pos.x)) >> 4,
                                static_cast<int>(std::floor(pos.z)) >> 4
                        );

                        if (entityChunk == chunkPos2D) {
                            grid->AddEntityToChunk(comp.getEntityId(), chunkPos2D);
                        }
                    }

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

        int createBaseHumanoid(glm::vec3 pos, bool isPlayer) {
            animation::AnimationAssetLibrary assets;
            assets.loadFromFBX("resources/objects/humanoid/female_character.fbx");
            if (!assets.model) return -1;

            auto ourShader = std::make_unique<Shader>(
                    "../shader/vertex/anim_model.vs",
                    "../shader/fragment/anim_model.fs"
            );

            auto characterTransform = Transform{assets.model->getMeshVerticesCollapsed()};
            characterTransform.position = pos;
            characterTransform.scale = {0.05f, 0.05f, 0.05f};

            int entityId = numEntities;
            auto sharedFSM = std::make_shared<AnimationFSM>();

            // States common to both
            auto idleAnimState = std::make_shared<AnimationState>(std::make_unique<ClipNode>(assets.get("HumanM@Idle01")));
            idleAnimState->rootBoneNames = {"B-root"};

            auto multiDirectionalTree = std::make_unique<DirectionalBlendTree2D>();
            multiDirectionalTree->AddNode(std::make_unique<ClipNode>(assets.get("HumanM@Walk01_Forward [RM]")),      glm::vec2(0.0f, 0.5f));
            multiDirectionalTree->AddNode(std::make_unique<ClipNode>(assets.get("HumanM@Run01_Forward [RM]")),       glm::vec2(0.0f, 1.0f));
            multiDirectionalTree->AddNode(std::make_unique<ClipNode>(assets.get("HumanM@Walk01_Backward [RM]")),     glm::vec2(0.0f, -0.5f));
            multiDirectionalTree->AddNode(std::make_unique<ClipNode>(assets.get("HumanM@Run01_Backward [RM]")),      glm::vec2(0.0f, -1.0f));
            multiDirectionalTree->AddNode(std::make_unique<ClipNode>(assets.get("HumanM@Walk01_Left [RM]")),         glm::vec2(-0.5f, 0.0f));
            multiDirectionalTree->AddNode(std::make_unique<ClipNode>(assets.get("HumanM@Run01_Left [RM]")),          glm::vec2(-1.0f, 0.0f));
            multiDirectionalTree->AddNode(std::make_unique<ClipNode>(assets.get("HumanM@Walk01_Right [RM]")),        glm::vec2(0.5f, 0.0f));
            multiDirectionalTree->AddNode(std::make_unique<ClipNode>(assets.get("HumanM@Run01_Right [RM]")),         glm::vec2(1.0f, 0.0f));
            multiDirectionalTree->AddNode(std::make_unique<ClipNode>(assets.get("HumanM@Walk01_ForwardLeft [RM]")),  glm::vec2(-0.5f, 0.5f));
            multiDirectionalTree->AddNode(std::make_unique<ClipNode>(assets.get("HumanM@Walk01_ForwardRight [RM]")), glm::vec2(0.5f, 0.5f));
            multiDirectionalTree->AddNode(std::make_unique<ClipNode>(assets.get("HumanM@Walk01_BackwardLeft [RM]")), glm::vec2(-0.5f, -0.5f));
            multiDirectionalTree->AddNode(std::make_unique<ClipNode>(assets.get("HumanM@Walk01_BackwardRight [RM]")),glm::vec2(0.5f, -0.5f));
            multiDirectionalTree->AddNode(std::make_unique<ClipNode>(assets.get("HumanM@Run01_ForwardLeft [RM]")),   glm::vec2(-0.707f, 0.707f));
            multiDirectionalTree->AddNode(std::make_unique<ClipNode>(assets.get("HumanM@Run01_ForwardRight [RM]")),  glm::vec2(0.707f, 0.707f));
            multiDirectionalTree->AddNode(std::make_unique<ClipNode>(assets.get("HumanM@Run01_BackwardLeft [RM]")),  glm::vec2(-0.707f, -0.707f));
            multiDirectionalTree->AddNode(std::make_unique<ClipNode>(assets.get("HumanM@Run01_BackwardRight [RM]")), glm::vec2(0.707f, -0.707f));

            auto locAnimState = std::make_shared<AnimationState>(std::move(multiDirectionalTree));
            locAnimState->rootBoneNames = {"B-root"};

            auto jumpAnimState = std::make_shared<AnimationState>(std::make_unique<ClipNode>(assets.get("HumanM@Jump01 [RM]")));
            jumpAnimState->rootBoneNames = {"B-root"};

            gameObjects[entityId] = std::make_unique<AnimationModelObject>(
                    entityId, assets.model, std::move(ourShader), characterTransform, sharedFSM
            );

            CharacterControllerComponent cc(entityId, &gameObjects[entityId]->transform, sharedFSM);
            cc.skeleton = assets.model;
            ecsManager->addCharacterControllerComponent(entityId, cc);

            CharacterControllerComponent* permanentCcPtr = &ecsManager->getCharacterControllerComponentFromSparse(entityId);

            auto stateIdle = std::make_shared<fsm::IdleState>(permanentCcPtr, idleAnimState);
            auto stateLocomotion = std::make_shared<fsm::LocomotionState>(permanentCcPtr, locAnimState);
            auto stateJump = std::make_shared<fsm::JumpState>(permanentCcPtr, jumpAnimState);

            fsm::AnimationFSM::TransitionMap transitions;

            transitions[stateIdle].push_back({stateLocomotion, std::make_shared<fsm::IdleToLocomotionTransition>(permanentCcPtr)});
            transitions[stateIdle].push_back({stateJump, std::make_shared<fsm::AnyToJumpTransition>(permanentCcPtr, *ecsManager)});

            transitions[stateLocomotion].push_back({stateIdle, std::make_shared<fsm::LocomotionToIdleTransition>(permanentCcPtr)});
            transitions[stateLocomotion].push_back({stateJump, std::make_shared<fsm::AnyToJumpTransition>(permanentCcPtr, *ecsManager)});

            transitions[stateJump].push_back({stateLocomotion, std::make_shared<fsm::JumpToFallbackTransition>(permanentCcPtr, *ecsManager, stateJump)});
            transitions[stateJump].push_back({stateIdle, std::make_shared<fsm::JumpToFallbackTransition>(permanentCcPtr,*ecsManager, stateJump)});

            // Player Specific Combat States & Transitions
            if (isPlayer) {
                auto jabAnimState = std::make_shared<AnimationState>(std::make_unique<ClipNode>(assets.get("HumanM@Jab")));
                jabAnimState->rootBoneNames = {"B-root"};
                auto crossAnimState = std::make_shared<AnimationState>(std::make_unique<ClipNode>(assets.get("HumanM@Cross")));
                crossAnimState->rootBoneNames = {"B-root"};
                auto hookAnimState = std::make_shared<AnimationState>(std::make_unique<ClipNode>(assets.get("HumanM@LeftHook")));
                hookAnimState->rootBoneNames = {"B-root"};
                auto rightHookAnimState = std::make_shared<AnimationState>(std::make_unique<ClipNode>(assets.get("HumanM@RightHook")));
                rightHookAnimState->rootBoneNames = {"B-root"};
                auto uppercutAnimState = std::make_shared<AnimationState>(std::make_unique<ClipNode>(assets.get("HumanM@LeftUppercut")));
                uppercutAnimState->rootBoneNames = {"B-root"};

                auto stateJab = std::make_shared<fsm::PunchState>(permanentCcPtr, jabAnimState);
                auto stateCross = std::make_shared<fsm::PunchState>(permanentCcPtr, crossAnimState);
                auto stateHook = std::make_shared<fsm::PunchState>(permanentCcPtr, hookAnimState);
                auto stateRightHook = std::make_shared<fsm::PunchState>(permanentCcPtr, rightHookAnimState);
                auto stateUppercut = std::make_shared<fsm::PunchState>(permanentCcPtr, uppercutAnimState);

                std::vector<std::pair<std::shared_ptr<fsm::State>, BoxingPunch>> punchConfigs = {
                        {stateJab, BoxingPunch::Jab}, {stateCross, BoxingPunch::Cross},
                        {stateHook, BoxingPunch::LeftHook}, {stateRightHook, BoxingPunch::RightHook},
                        {stateUppercut, BoxingPunch::LeftUppercut}
                };

                for (const auto& config : punchConfigs) {
                    transitions[stateIdle].push_back({config.first, std::make_shared<fsm::AnyToPunchTransition>(permanentCcPtr, config.second)});
                    transitions[stateLocomotion].push_back({config.first, std::make_shared<fsm::AnyToPunchTransition>(permanentCcPtr, config.second)});
                }

                std::vector<std::shared_ptr<fsm::State>> punchStates = {stateJab, stateCross, stateHook, stateRightHook, stateUppercut};
                for (const auto& currentPunchState : punchStates) {
                    for (const auto& config : punchConfigs) {
                        transitions[currentPunchState].push_back({config.first, std::make_shared<fsm::AnyToPunchTransition>(permanentCcPtr, config.second)});
                    }
                    transitions[currentPunchState].push_back({stateLocomotion, std::make_shared<fsm::PunchToFallbackTransition>(permanentCcPtr, currentPunchState)});
                    transitions[currentPunchState].push_back({stateIdle, std::make_shared<fsm::PunchToFallbackTransition>(permanentCcPtr, currentPunchState)});
                }
            }

            sharedFSM->Initialize(transitions, stateIdle);

            PhysicsComponent pc(entityId);
            pc.addModel(assets.model);
            ecsManager->addPhysicsComponent(entityId, pc);

            HealthComponent hc(entityId);
            hc.currentHealth = 1000.0f;
            hc.maxHealth = 1000.0f;
            ecsManager->addHealthComponent(entityId, hc);

            // Player Specific Combat Component
            if (isPlayer) {
                AttackComponent atc(entityId);
                atc.attackRadius = 2.0f;
                const auto& boneInfoMap = assets.model->GetBoneInfoMap();
                auto it = boneInfoMap.find("B-hand.L");
                atc.damagingBoneIndex = (it != boneInfoMap.end()) ? it->second.id : -1;
                ecsManager->addAttackComponent(entityId, atc);
            }

            glm::ivec2 spawnChunkCoord = glm::ivec2(
                    static_cast<int>(std::floor(pos.x)) >> 4,
                    static_cast<int>(std::floor(pos.z)) >> 4
            );
            grid->AddEntityToChunk(entityId, spawnChunkCoord);

            numEntities++;
            return entityId;
        }

        void initialisePlayer(glm::vec3 pos) {
            int entityId = createBaseHumanoid(pos, true);
            if (entityId == -1) return;

            player = std::make_shared<Player>(grid, entityId, ecsManager);
            glfwSetWindowUserPointer(window, player.get());
        }

        void initialiseNPC(glm::vec3 pos) {
            int entityId = createBaseHumanoid(pos, false);
            if (entityId == -1) return;

            AIComponent ac(entityId);
            ecsManager->addAIComponent(entityId, ac);
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
            initialisePlayer({30, 30, 30});
//            initialiseNPC({35, 100, 30});

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

            attackSystem->update(*ecsManager, dt);



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