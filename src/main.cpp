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

#include <GLFW/glfw3.h>
#include <stb/stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>

std::unique_ptr<Camera> camera = std::make_unique<Camera>();
std::shared_ptr<Logger> logger = std::make_shared<Logger>("debug.txt");

std::unique_ptr<LightingRenderer>  lighting;

const unsigned int SCR_WIDTH  = 800;
const unsigned int SCR_HEIGHT = 600;
const float aspectRatio = static_cast<float>(SCR_WIDTH) / static_cast<float>(SCR_HEIGHT);

bool  cursorEnabled = false;



void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window, double elapsed);
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
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);
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

    const glm::mat4 view       = camera->getViewMatrix();
    const glm::mat4 projection = camera->getProjectionMatrix(aspectRatio);

//    lighting->drawLighting(camera->getPosition(), projection, view);

    RenderContext renderContext{camera->getPosition(), projection, view};
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
//    lighting->setupLighting();




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

    initialiseGameObjects();


    double previous = glfwGetTime();
    double lag = 0.0;

    while (!glfwWindowShouldClose(window))
    {
        double current = glfwGetTime();
        double elapsed = current - previous;
        previous = current;
        lag += elapsed;

        processInput(window, elapsed);

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

void processInput(GLFWwindow* window, double elapsed)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS && !cursorEnabled)
    {
        cursorEnabled = true;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera->processKeyboard(CameraMovement::FORWARD, elapsed);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera->processKeyboard(CameraMovement::BACKWARD, elapsed);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera->processKeyboard(CameraMovement::LEFT, elapsed);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera->processKeyboard(CameraMovement::RIGHT, elapsed);
}

void mouse_callback(GLFWwindow*, double xpos, double ypos)
{
    if (!cursorEnabled)
        camera->processMouseMovement(static_cast<float>(xpos), static_cast<float>(ypos));
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && cursorEnabled)
    {
        cursorEnabled = false;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
}

void scroll_callback(GLFWwindow*, double, double yoffset)
{
    camera->processScroll(static_cast<float>(yoffset));
}

void clearScreen()
{
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}