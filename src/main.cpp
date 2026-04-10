#define GLM_ENABLE_EXPERIMENTAL

#include <iostream>
#include <glad/glad.h>
#include "../include/rendering/Shader.h"
#include "../include/Camera.h"
#include "../include/logger/Logger.h"
#include "../include/rendering/TerrainMCRenderer.h"
#include "../include/rendering/LightingRenderer.h"
#include "../include/globals.h"
#include "../include/scene/components/AIComponent.h"
#include "../include/scene/components/PhysicsComponent.h"
#include "../include/scene/objects/CustomMeshObject.h"
#include "../include/rendering/VoxelRenderer.h"
#include "../include/player/Player.h"
#include "../include/animation/Animation.h"
#include "../include/animation/Animator.h"
#include "../include/scene/objects/AnimationModelObject.h"
#include "../include/world/World.h"

#include <GLFW/glfw3.h>
#include <stb/stb_image.h>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>


using namespace world;
std::shared_ptr<logger::Logger> logFile = std::make_shared<logger::Logger>("debug.txt");

std::shared_ptr<World> voxelWorld = std::make_shared<World>();


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

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "QuestFarer (In Development)", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return nullptr;
    }

    glfwMakeContextCurrent(window);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, Player::Player::mouse_callback);
    glfwSetMouseButtonCallback(window, Player::Player::mouse_button_callback);
    glfwSetScrollCallback(window, Player::Player::scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);


    return window;
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

    voxelWorld->initialise(window);

    double previous = glfwGetTime();
    double lag = 0.0;

    while (!glfwWindowShouldClose(window))
    {
        double current = glfwGetTime();
        double elapsed = current - previous;
        previous = current;
        lag += elapsed;


        clearScreen();

        while (lag >= FIXED_TIMESTEP)
        {
            voxelWorld->update(lag / FIXED_TIMESTEP);
            lag -= FIXED_TIMESTEP;
        }
        voxelWorld->render(lag / FIXED_TIMESTEP);


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