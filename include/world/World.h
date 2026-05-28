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
#include "ProceduralTerrainGenerator.h"
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

            glm::vec3 pPos = player->getPosition();
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
            auto idleState = std::make_shared<SimpleAnimationState>();
            idleState->animation = playerAssets.get("HumanM@Idle01");
            idleState->rootBoneNames = {"B-root"};

            auto locState = std::make_shared<LocomotionBlendState>();
            locState->walk = playerAssets.get("HumanM@Walk01_Forward [RM]");
            locState->run = playerAssets.get("HumanM@Run01_Forward [RM]");
            locState->rootBoneNames = {"B-root"};

            sharedFSM->RegisterState("Idle", idleState);
            sharedFSM->RegisterState("Locomotion", locState);
            sharedFSM->TransitionTo("Idle");

            gameObjects[entityId] = std::make_unique<AnimationModelObject>(
                    entityId, playerAssets.model, std::move(ourShader), characterTransform, sharedFSM
            );

            CharacterControllerComponent cc(entityId, &gameObjects[entityId]->transform, sharedFSM);
            cc.initialize(playerAssets.model);
            cc.setLocomotionSpeed(0.0f);
            cc.setECSManager(ecsManager);
            ecsManager->addCharacterControllerComponent(entityId, cc);

            player = std::make_shared<Player>(grid, &ecsManager->getCharacterControllerComponentFromSparse(entityId));
            glfwSetWindowUserPointer(window, player.get());

            PhysicsComponent pc(entityId);
            pc.addModel(playerAssets.model);
            ecsManager->addPhysicsComponent(entityId, pc);
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
            createVoxels();
            updateTerrainStreaming();
        }

        void update(double timeScale) {
            player->processInput(window, timeScale);
            ecsManager->update();
            updateTerrainStreaming();
            physicsSystem->step(
                    ecsManager->getPhysicsComponentsDense(),
                    ecsManager->getPhysicsComponentsAmount(),
                    gameObjects, grid, timeScale * FIXED_TIMESTEP
            );
            player->updateCamera();

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