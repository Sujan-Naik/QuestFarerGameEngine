#include <utility>
#include <iostream>

#include "../../include/player/Player.h"

using namespace player;

Player::Player(std::shared_ptr<Grid> gridPtr) : grid(std::move(gridPtr)) {}




const rendering::RenderContext Player::getRenderContext() const {
    const glm::mat4 view       = camera->getViewMatrix();
    const glm::mat4 projection = camera->getProjectionMatrix(aspectRatio);


    return rendering::RenderContext{camera->getPosition(), projection, view};
}

void Player::processInput(GLFWwindow* window, double timeScale)
{
    std::cout << timeScale << std::endl;
    actionTimer -= FIXED_TIMESTEP * timeScale;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS && !cursorEnabled)
    {
        cursorEnabled = true;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera->processKeyboard(CameraMovement::FORWARD, timeScale);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera->processKeyboard(CameraMovement::BACKWARD, timeScale);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera->processKeyboard(CameraMovement::LEFT, timeScale);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera->processKeyboard(CameraMovement::RIGHT, timeScale);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camera->processKeyboard(CameraMovement::UP, timeScale);
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        camera->processKeyboard(CameraMovement::DOWN, timeScale);


    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        if (!cursorEnabled && actionTimer<=0.0f)
        {
            RaycastResult result = raycast(100);

            if (result.hit){
                grid->setVoxel(result.voxelPos.x,result.voxelPos.y, result.voxelPos.z, VoxelType::AIR );
                actionTimer = ACTION_COOLDOWN;
            }
        }
    }
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        if (actionTimer<=0.0f) {
            handleRightClick();
            actionTimer = ACTION_COOLDOWN;
        }
    }

}


void Player::mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    // 1. Get the "this" pointer back from GLFW
    Player* player = static_cast<Player*>(glfwGetWindowUserPointer(window));

    // 2. Use the instance to access private variables/methods
    if (player && !player->cursorEnabled)
    {
        player->processMouseMovement(static_cast<float>(xpos), static_cast<float>(ypos));
    }
}

void Player::handleRightClick() {
    RaycastResult result = raycast(100.0f);

    if (result.hit) {
        // The magic formula: Hit Position + Normal = Adjacent Space
        glm::ivec3 placePos = result.voxelPos + result.normal;

        // Ensure we don't place a block inside our own head
        glm::ivec3 playerPos = glm::floor(camera->getPosition());
        if (placePos != playerPos) {
            grid->setVoxel(placePos.x, placePos.y, placePos.z, VoxelType::DIRT);
        }
    }
}


void Player::mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    Player* player = static_cast<Player*>(glfwGetWindowUserPointer(window));
    if (!player) return;

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        if (player->cursorEnabled)
        {
            player->cursorEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }

    }
}


void Player::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    Player* player = static_cast<Player*>(glfwGetWindowUserPointer(window));

    if (player)
    {
        player->processScroll(static_cast<float>(yoffset));
    }
}


void Player::processMouseMovement(float xpos, float ypos)
{
    camera->processMouseMovement(xpos, ypos);
}

void Player::processScroll(float yoffset)
{
    camera->processScroll(yoffset);
}




RaycastResult Player::raycast(float maxDistance) {
    glm::vec3 rayOrigin = camera->getPosition();
    glm::vec3 rayDir = camera->getFront();

    // Current voxel coordinates (int)
    glm::ivec3 voxelPos = glm::floor(rayOrigin);

    // How far to travel in ray units to move 1 full unit in X, Y, or Z
    glm::vec3 deltaDist = glm::abs(glm::vec3(1.0f) / rayDir);

    // Which direction to step in (+1 or -1)
    glm::ivec3 step = glm::sign(rayDir);

    // Distance to the next voxel boundary
    glm::vec3 sideDist;
    sideDist.x = (step.x > 0) ? (voxelPos.x + 1.0f - rayOrigin.x) * deltaDist.x : (rayOrigin.x - voxelPos.x) * deltaDist.x;
    sideDist.y = (step.y > 0) ? (voxelPos.y + 1.0f - rayOrigin.y) * deltaDist.y : (rayOrigin.y - voxelPos.y) * deltaDist.y;
    sideDist.z = (step.z > 0) ? (voxelPos.z + 1.0f - rayOrigin.z) * deltaDist.z : (rayOrigin.z - voxelPos.z) * deltaDist.z;

    float traveled = 0;
    glm::ivec3 lastNormal(0);

    while (traveled < maxDistance) {
        // Check if current voxelPos is solid in your Grid
        if (grid->isSolid(voxelPos.x, voxelPos.y, voxelPos.z)) {
            return {true, voxelPos, lastNormal};
        }

        // Jump to next voxel boundary
        if (sideDist.x < sideDist.y && sideDist.x < sideDist.z) {
            traveled = sideDist.x;
            sideDist.x += deltaDist.x;
            voxelPos.x += step.x;
            lastNormal = glm::ivec3(-step.x, 0, 0);
        } else if (sideDist.y < sideDist.z) {
            traveled = sideDist.y;
            sideDist.y += deltaDist.y;
            voxelPos.y += step.y;
            lastNormal = glm::ivec3(0, -step.y, 0);
        } else {
            traveled = sideDist.z;
            sideDist.z += deltaDist.z;
            voxelPos.z += step.z;
            lastNormal = glm::ivec3(0, 0, -step.z);
        }
    }
    return {false};
}

