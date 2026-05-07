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

        std::set<ChunkPos> activeChunks;
        rendering::VoxelRenderer* voxelRendererPtr = nullptr;
        const int RENDER_DISTANCE = 8;
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

            glm::vec3 pPos = player->getPosition();
            int pChunkX = static_cast<int>(pPos.x) >> 4;
            int pChunkZ = static_cast<int>(pPos.z) >> 4;

            // 1. Gather all chunks within range that aren't loaded yet
            std::vector<ChunkPos> targets;
            for (int x = -RENDER_DISTANCE; x <= RENDER_DISTANCE; ++x) {
                for (int z = -RENDER_DISTANCE; z <= RENDER_DISTANCE; ++z) {
                    int tx = pChunkX + x;
                    int tz = pChunkZ + z;
                    if (activeChunks.find({tx, tz}) == activeChunks.end()) {
                        targets.push_back({tx, tz});
                    }
                }
            }

            // 2. Sort targets by distance to player (Closest First)
            std::sort(targets.begin(), targets.end(), [&](const ChunkPos& a, const ChunkPos& b) {
                int distA = std::abs(a.x - pChunkX) + std::abs(a.z - pChunkZ);
                int distB = std::abs(b.x - pChunkX) + std::abs(b.z - pChunkZ);
                return distA < distB;
            });

            // 3. Process a limited number of chunks per frame (prevents lag spikes)
            int chunksCreated = 0;
            const int MAX_GEN_PER_FRAME = 2;

            for (const auto& pos : targets) {
                if (chunksCreated >= MAX_GEN_PER_FRAME) break;

                terrainGenerator->generateChunk(grid, pos.x, pos.z);
                activeChunks.insert(pos);
                chunksCreated++;
            }

            // 4. Unload (same as before)
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
            characterTransform.scale = {0.1f, 0.1f, 0.1f};

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
            ecsManager->addCharacterControllerComponent(entityId, cc);

            player = std::make_shared<Player>(grid, &ecsManager->getCharacterControllerComponentFromSparse(entityId));
            glfwSetWindowUserPointer(window, player.get());

            PhysicsComponent pc(entityId);
            pc.addCollisionMeshesFromModel(playerAssets.model->getMeshVertices(), playerAssets.model->getMeshIndices());
            ecsManager->addPhysicsComponent(entityId, pc);
            numEntities++;
        }

    public:
        World() {
            grid = std::make_shared<Grid>();
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
            player->updateCamera();
            updateTerrainStreaming();
            physicsSystem->step(
                    ecsManager->getPhysicsComponentsDense(),
                    ecsManager->getPhysicsComponentsAmount(),
                    gameObjects, grid, timeScale * FIXED_TIMESTEP
            );
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