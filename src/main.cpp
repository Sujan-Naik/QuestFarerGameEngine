#include <iostream>
#include <glad/glad.h>
#include "../include/rendering/Shader.h"
#include "../include/Camera.h"
#include "../include/logger/Logger.h"
#include "../include/generator/Generator.h"
#include "../include/rendering/TerrainDSRenderer.h"
#include "../include/rendering/TerrainMCRenderer.h"
#include "../include/rendering/LightingRenderer.h"
#include "../include/cloud/CloudRenderer.h"
#include "../include/globals.h"

#include <GLFW/glfw3.h>
#include <cmath>
#include <stb/stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>

#define GLM_ENABLE_EXPERIMENTAL

#define TERRAIN_MC
// #define TERRAIN_DS

std::unique_ptr<Camera> camera = std::make_unique<Camera>();
std::shared_ptr<Logger> logger = std::make_shared<Logger>("debug.txt");

constexpr float TERRAIN_SIMULATION_CD = 1.0f;
float lastTerrainSimulateTime;
std::shared_ptr<Generator> generator = std::make_shared<Generator>(SIZE, HEIGHT_LOWER_BOUND, HEIGHT_UPPER_BOUND);

#ifdef TERRAIN_DS
std::unique_ptr<TerrainDSRenderer> terrainRenderer;
#endif
#ifdef TERRAIN_MC
std::unique_ptr<TerrainMCRenderer> terrainRenderer;
#endif

std::shared_ptr<CloudSimulator> cloudSimulator = std::make_shared<CloudSimulator>();
std::unique_ptr<CloudRenderer> cloudRenderer;
std::unique_ptr<LightingRenderer> lighting;

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

bool firstMouse = true;
float yaw   = -90.0f;
float pitch =  0.0f;
float lastX =  800.0f / 2.0;
float lastY =  600.0f / 2.0;
float fov   =  45.0f;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void handleRendering(GLFWwindow* window);
glm::mat4 getViewMatrix();
glm::mat4 getProjectionMatrix();

void createTexture(unsigned int& texture, const char* path)
{
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
    if (data)
    {
        GLint textureFormat;
        switch (nrChannels) {
            case 1:  textureFormat = GL_RED;  break;
            case 3:  textureFormat = GL_RGB;  break;
            case 4:  textureFormat = GL_RGBA; break;
            default: textureFormat = GL_RGB;  break;
        }
        glTexImage2D(GL_TEXTURE_2D, 0, textureFormat, width, height, 0, textureFormat, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        throw std::runtime_error("Failed to load texture");
    }
    stbi_image_free(data);
}

int main()
{
    camera->setCameraPos({EFFECTIVE_SIDE_LENGTH / 2, HEIGHT_UPPER_BOUND * 1.5, EFFECTIVE_SIDE_LENGTH / 2});
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "QuestFarer Dynamic World Generation Demo", nullptr, nullptr);
    if (window == nullptr)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    std::unique_ptr<Shader> terrainShader =
            std::make_unique<Shader>("../shader/vertex/terrain-shader.vs",
                                     "../shader/fragment/terrain-shader.fs");
    terrainShader->use();
    terrainShader->setInt("texture1", 0);

    unsigned int texture1;
    createTexture(texture1, "grass.png");

    unsigned int texture2;
    createTexture(texture2, "cloud.png");


#ifdef TERRAIN_DS
    generator->generateGrid();
    terrainRenderer = std::make_unique<TerrainDSRenderer>(logger, std::move(terrainShader));
    terrainRenderer->initTerrainData(generator);
    terrainRenderer->setup();
    lastTerrainSimulateTime = (float)glfwGetTime();
#endif
#ifdef TERRAIN_MC


    terrainRenderer = std::make_unique<TerrainMCRenderer>(logger, std::move(terrainShader));
    terrainRenderer->initTerrainData(generator);
    terrainRenderer->setTexture(texture1);  // <-- before setup
    terrainRenderer->setup();
#endif
    cloudRenderer = std::make_unique<CloudRenderer>(logger, cloudSimulator);
    lighting = std::make_unique<LightingRenderer>(logger);

    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    cloudRenderer->setupVertexData();
    lighting->setupLighting();

    while (!glfwWindowShouldClose(window))
    {
        auto currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);
        handleRendering(window);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture1);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, texture2);

        terrainRenderer->draw(camera->getCameraPos(), getProjectionMatrix(), getViewMatrix(), {0, 0, 0});

        lighting->drawLighting(camera->getCameraPos(), getProjectionMatrix(), getViewMatrix());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

glm::mat4 getViewMatrix()
{
    return glm::lookAt(camera->getCameraPos(),
                       camera->getCameraPos() + camera->getCameraFront(),
                       camera->getCameraUp());
}

glm::mat4 getProjectionMatrix()
{
    return glm::perspective(glm::radians(fov), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 10000.0f);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

bool cursorEnabled = false;

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        if (!cursorEnabled) {
            cursorEnabled = true;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
    float cameraSpeed = static_cast<float>(fov * 2.5 * deltaTime);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera->setCameraPos(camera->getCameraPos() + cameraSpeed * camera->getCameraFront());
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera->setCameraPos(camera->getCameraPos() - cameraSpeed * camera->getCameraFront());
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera->setCameraPos(camera->getCameraPos() - glm::normalize(glm::cross(camera->getCameraFront(), camera->getCameraUp())) * cameraSpeed);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera->setCameraPos(camera->getCameraPos() + glm::normalize(glm::cross(camera->getCameraFront(), camera->getCameraUp())) * cameraSpeed);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    auto xpos = static_cast<float>(xposIn);
    auto ypos = static_cast<float>(yposIn);

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw   += xoffset;
    pitch += yoffset;

    if (pitch >  89.0f) pitch =  89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    camera->setCameraFront(glm::normalize(front));
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        if (cursorEnabled) {
            cursorEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    fov -= (float)yoffset;
    if (fov < 1.0f)  fov = 1.0f;
    if (fov > 70.0f) fov = 70.0f;
}

void handleRendering(GLFWwindow* window)
{
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}