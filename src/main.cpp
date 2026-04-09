#define GLM_ENABLE_EXPERIMENTAL

#include <iostream>
#include <glad/glad.h>
#include "../include/rendering/Shader.h"
#include "../include/Camera.h"
#include "../include/logger/Logger.h"
#include "../include/rendering/TerrainMCRenderer.h"
#include "../include/rendering/LightingRenderer.h"
#include "../include/globals.h"
#include "../include/model/Model.h"
#include "../include/scene/components/AIComponent.h"
#include "../include/scene/components/PhysicsComponent.h"
#include "../include/scene/objects/GameObject.h"
#include "../include/scene/objects/CustomMeshObject.h"
#include "../include/scene/objects/ModelObject.h"
#include "../include/rendering/VoxelRenderer.h"
#include "../include/player/Player.h"
#include "../include/animation/model_animation.h"
#include "../include/animation/Animation.h"
#include "../include/animation/Animator.h"
#include "../include/scene/objects/AnimationModelObject.h"

#include <GLFW/glfw3.h>
#include <stb/stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>



std::shared_ptr<Logger> logger = std::make_shared<Logger>("debug.txt");
std::shared_ptr<Grid> grid = std::make_shared<Grid>();
std::shared_ptr<Player> player = std::make_shared<Player>(grid);


std::unique_ptr<LightingRenderer>  lighting;



void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void clearScreen();

void createTexture(unsigned int& texture, const char* path)
{
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, nrChannels;
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
    if (!data)
        throw std::runtime_error("Failed to load texture");

    GLint textureFormat;
    switch (nrChannels) {
        case 1:  textureFormat = GL_RED;  break;
        case 3:  textureFormat = GL_RGB;  break;
        case 4:  textureFormat = GL_RGBA; break;
        default: textureFormat = GL_RGB;  break;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, textureFormat, width, height, 0, textureFormat, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);
}

GLFWwindow* initialiseGLFW()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "QuestFarer Dynamic World Generation Demo", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return nullptr;
    }

    glfwMakeContextCurrent(window);
    glfwSetWindowUserPointer(window, player.get());

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, Player::mouse_callback);
    glfwSetMouseButtonCallback(window, Player::mouse_button_callback);
    glfwSetScrollCallback(window, Player::scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);


    return window;
}


const int MAX_ENTITIES = 100;
int aiComponentsSparse[MAX_ENTITIES];
AIComponent* aiComponentsDense =
        new AIComponent[MAX_ENTITIES];

int aiComponentsAmount = 0;

int physicsComponentsSparse[MAX_ENTITIES];
PhysicsComponent* physicsComponentsDense =
        new PhysicsComponent[MAX_ENTITIES];

int physicsComponentsAmount = 0;


std::unique_ptr<GameObject> gameObjects[MAX_ENTITIES];
int numEntities = 0;


void render(double timeScale){

    RenderContext renderContext = player->getRenderContext();

    // Draw to screen.
    for (int i = 0; i < numEntities; i++)
    {
        gameObjects[i]->draw(renderContext);
    }
}


void update(){

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

    gameObjects[numEntities] = std::make_unique<AnimationModelObject>(numEntities, std::move(ourModel), std::move(ourShader),glm::mat4(1.0), std::move(animator));

    numEntities++;
}



void initialiseGameObjects(){
    auto terrainShader = std::make_unique<Shader>(
            "../shader/vertex/terrain-shader.vs",
            "../shader/fragment/terrain-shader.fs"
    );
    terrainShader->use();
    terrainShader->setInt("texture1", 0);

    unsigned int texture1;
    createTexture(texture1, "resources/images/grass.png");

    auto terrainRenderer = std::make_unique<TerrainMCRenderer>(logger, std::move(terrainShader));
    terrainRenderer->setTexture(texture1);
    terrainRenderer->setup();

    gameObjects[numEntities] = std::make_unique<CustomMeshObject>(numEntities, std::move(terrainRenderer), glm::mat4(1.0));

    numEntities++;


    auto ourModel = std::make_unique<Model>("resources/objects/backpack/backpack.obj");
    auto ourShader = std::make_unique<Shader>(
            "../shader/vertex/model-loading.vs",
            "../shader/fragment/model-loading.fs"
    );

    gameObjects[numEntities] = std::make_unique<ModelObject>(numEntities, std::move(ourModel), std::move(ourShader),glm::mat4(1.0));
    addPhysicsComponent(numEntities, *new PhysicsComponent(numEntities));

    numEntities++;


//    lighting = std::make_unique<LightingRenderer>(logger);
//    lighting->setupLighting()

}

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

    gameObjects[numEntities] = std::make_unique<CustomMeshObject>(numEntities, std::move(voxelRenderer), glm::mat4(1.0));

    numEntities++;
}


int main()
{
    GLFWwindow* window = initialiseGLFW();
    if (!window)
        return -1;


    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    stbi_set_flip_vertically_on_load(true);

//    initialiseGameObjects();

    initialisePlayer();
    createVoxels();

    double previous = glfwGetTime();
    double lag = 0.0;

    while (!glfwWindowShouldClose(window))
    {
        double current = glfwGetTime();
        double elapsed = current - previous;
        previous = current;
        lag += elapsed;

        player -> processInput(window, elapsed);

        clearScreen();

        while (lag >= FIXED_TIMESTEP)
        {
            update();
            lag -= FIXED_TIMESTEP;
        }
        render(lag / FIXED_TIMESTEP);


        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow*, int width, int height)
{
    glViewport(0, 0, width, height);
}


void clearScreen()
{
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}