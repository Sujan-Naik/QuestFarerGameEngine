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

class World {

private:

    std::shared_ptr<Grid> grid = std::make_shared<Grid>();
    std::shared_ptr<Player> player;
    std::unique_ptr<LightingRenderer>  lighting;

    PhysicsSystem physicsSystem;

    // ECS State
    std::unique_ptr<GameObject> gameObjects[MAX_ENTITIES];
    int numEntities = 0;

    PhysicsComponent physicsComponentsDense[MAX_ENTITIES];
    int physicsComponentsSparse[MAX_ENTITIES];
    int physicsComponentsAmount = 0;

    AIComponent aiComponentsDense[MAX_ENTITIES];
    int aiComponentsSparse[MAX_ENTITIES];
    int aiComponentsAmount = 0;

    std::shared_ptr<Logger> logger = std::make_shared<Logger>("world_log.txt");

    GLFWwindow* window;


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


        auto voxelRenderer = std::make_unique<VoxelRenderer>(logger, std::move(voxelShader),grid);
        voxelRenderer->setup();

        gameObjects[numEntities] = std::make_unique<CustomMeshObject>(numEntities, std::move(voxelRenderer),
                                                                      Transform{});

        numEntities++;
    }


    void addAIComponent(int entityID, const AIComponent& component) {
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


    void addPhysicsComponent(int entityID, const PhysicsComponent& component) {
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



    void initialisePlayer(){

        std::unique_ptr<animation::Model> ourModel = std::make_unique<animation::Model>("resources/objects/animation/vampire/dancing_vampire.dae");
        auto ourShader = std::make_unique<Shader>(
                "../shader/vertex/anim_model.vs",
                "../shader/fragment/anim_model.fs"
        );

        std::unique_ptr<animation::Animation> danceAnimation = std::make_unique<animation::Animation>("resources/objects/animation/vampire/dancing_vampire.dae", ourModel.get());
        std::unique_ptr<animation::Animator> animator = std::make_unique<animation::Animator>(std::move(danceAnimation));

        gameObjects[numEntities] = std::make_unique<AnimationModelObject>(numEntities, std::move(ourModel), std::move(ourShader),Transform{}, std::move(animator));

        addPhysicsComponent(numEntities, *new PhysicsComponent(numEntities));
        numEntities++;

        player = std::make_shared<Player>(grid);
        glfwSetWindowUserPointer(window, player.get());

    }

public:

    World() {
        grid = std::make_shared<Grid>();
    }

    std::shared_ptr<Player> getPlayer(){
        return player;
}


    void initialise(GLFWwindow* window){
        this->window = window;

        createVoxels();
        initialisePlayer();

    }

    void update(double timeScale){

        player -> processInput(window, timeScale);


        // Process AI.
        logger->log(DEBUG, "AI Component updates begin: " + std::to_string(aiComponentsAmount));

        for (int i = 0; i < aiComponentsAmount; i++)
        {
            aiComponentsDense[i].update( gameObjects[aiComponentsDense[i].getEntityId()].get());
        }

        logger->log(DEBUG, "Physics Component updates begin: " + std::to_string(physicsComponentsAmount));


        // Update physics.
        for (int i = 0; i < physicsComponentsAmount; i++)
        {
            physicsComponentsDense[i].update(gameObjects[physicsComponentsDense[i].getEntityId()].get());
        }
    }

    void render(double timeScale){

        RenderContext renderContext = player->getRenderContext();

        // Draw to screen.
        for (int i = 0; i < numEntities; i++)
        {
            gameObjects[i]->draw(renderContext);
        }
    }





};

#endif //QUESTFARERGAMEENGINE_WORLD_H
