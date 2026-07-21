#include <utility>
#include <iostream>
#include "../../include/player/Player.h"
#include "../../include/scene/components/ECSManager.h"

using namespace player;

Player::Player(std::shared_ptr<Grid> gridPtr, int entityId, std::shared_ptr<scene::components::ECSManager> newEcsManager)
        : grid(std::move(gridPtr)), avatarEntityId(entityId), ecsManager(newEcsManager) {}

const rendering::RenderContext Player::getRenderContext() const {
    const glm::mat4 view       = camera->getViewMatrix();
    const glm::mat4 projection = camera->getProjectionMatrix(aspectRatio);
    return rendering::RenderContext{camera->getPosition(), projection, view};
}

void Player::updateCamera(scene::components::ECSManager& ecs){
    auto* avatar = &ecs.getCharacterControllerComponentFromSparse(avatarEntityId);
    if (avatar && avatar->transform){
        glm::quat yawRotation = glm::angleAxis(glm::radians(camera->getYaw()), glm::vec3(0.0f, 1.0f, 0.0f));
//        glm::quat yawRotation = glm::angleAxis(glm::radians(camera->getYaw() + 180), glm::vec3(0.0f, 1.0f, 0.0f));
        avatar->transform->rotation = yawRotation;

        glm::vec3 offset = (avatar->transform->getBack() * glm::vec3(2, 2, 2) + avatar->transform->getUp() * glm::vec3(0, 0.1f, 0)) *
                           avatar->transform->getSize().x;

//        glm::vec3 offset = (avatar->transform->getForward() * glm::vec3(2, 2, 2) + avatar->transform->getUp() * glm::vec3(0, 0.1f, 0)) *
//                           avatar->transform->getSize().x;

        glm::vec3 newCameraPosition = glm::mix(camera->getPosition(), avatar->transform->getTop() + offset, 0.99f);
        camera->setPosition(newCameraPosition);
//        camera->setPosition(avatar->transform->getTop() + offset);
    }
}

glm::vec3 Player::getPosition(scene::components::ECSManager& ecs) {
    auto* avatar = &ecs.getCharacterControllerComponentFromSparse(avatarEntityId);
    if (avatar && avatar->transform) {
        return avatar->transform->position;
    }
    return glm::vec3(0.0f);
}

void Player::processInput(GLFWwindow* window, double timeScale, scene::components::ECSManager& ecs)
{
    actionTimer -= FIXED_TIMESTEP * timeScale;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS && !cursorEnabled)
    {
        cursorEnabled = true;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    auto* liveAvatar = &ecs.getCharacterControllerComponentFromSparse(avatarEntityId);
    if (!liveAvatar || liveAvatar->getEntityId() == -1) return;

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        liveAvatar->triggerJump();
    }

    bool shiftPressed = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS);

    float forwardInput = 0.0f;
    float strafeInput  = 0.0f;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) forwardInput += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) forwardInput -= 1.0f;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) strafeInput  += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) strafeInput  -= 1.0f;

    bool isSprinting = shiftPressed && (liveAvatar->m_activePunchIntent == BoxingPunch::None);

    if (forwardInput != 0.0f || strafeInput != 0.0f) {
        float forwardMagnitude = forwardInput * (isSprinting ? 1.0f : 0.5f);
        float strafeMagnitude = strafeInput * (isSprinting ? 1.0f : 0.5f);

        liveAvatar->m_currentDirection = glm::vec2(strafeMagnitude, forwardMagnitude);
        liveAvatar->m_currentSpeedFactor = glm::length(liveAvatar->m_currentDirection);
    } else {
        liveAvatar->m_currentDirection = glm::vec2(0.0f);
        liveAvatar->m_currentSpeedFactor = 0.0f;
    }
}

void Player::mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    Player* player = static_cast<Player*>(glfwGetWindowUserPointer(window));
    if (!player) return;

    if (!player->cursorEnabled)
    {
        player->processMouseMovement(static_cast<float>(xpos), static_cast<float>(ypos));
    }
}

void Player::handleRightClick() {
    RaycastResult result = raycast(100.0f);
    if (result.hit) {
        glm::ivec3 placePos = result.voxelPos + result.normal;
        glm::ivec3 playerPos = glm::floor(camera->getPosition());
        if (placePos != playerPos) {
            grid->SetVoxel(placePos.x, placePos.y, placePos.z, VoxelType::DIRT);
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
            return;
        }
    }

    if (player->cursorEnabled) return;

    auto* liveAvatar = player->ecsManager->getCharacterControllerComponent(player->getAvatarId());
    if (!liveAvatar || liveAvatar->getEntityId() == -1) return;

    bool shiftPressed = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS);
    bool holdQ = (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS);
    bool holdE = (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS);

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        if (shiftPressed) {
            liveAvatar->m_activePunchIntent = BoxingPunch::LeftUppercut;
        } else if (holdQ) {
            liveAvatar->m_activePunchIntent = BoxingPunch::LeftHook;
        } else {
            liveAvatar->m_activePunchIntent = BoxingPunch::Jab;
        }
        liveAvatar->triggerPunch();
    }
    else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        if (holdE) {
            liveAvatar->m_activePunchIntent = BoxingPunch::RightHook;
        } else {
            liveAvatar->m_activePunchIntent = BoxingPunch::Cross;
        }
        liveAvatar->triggerPunch();
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
    glm::ivec3 voxelPos = glm::floor(rayOrigin);
    glm::vec3 deltaDist = glm::abs(glm::vec3(1.0f) / rayDir);
    glm::ivec3 step = glm::sign(rayDir);
    glm::vec3 sideDist;

    sideDist.x = (step.x > 0) ? (voxelPos.x + 1.0f - rayOrigin.x) * deltaDist.x : (rayOrigin.x - voxelPos.x) * deltaDist.x;
    sideDist.y = (step.y > 0) ? (voxelPos.y + 1.0f - rayOrigin.y) * deltaDist.y : (rayOrigin.y - voxelPos.y) * deltaDist.y;
    sideDist.z = (step.z > 0) ? (voxelPos.z + 1.0f - rayOrigin.z) * deltaDist.z : (rayOrigin.z - voxelPos.z) * deltaDist.z;

    float traveled = 0;
    glm::ivec3 lastNormal(0);

    while (traveled < maxDistance) {
        if (grid->IsSolid(voxelPos.x, voxelPos.y, voxelPos.z)) {
            return {true, voxelPos, lastNormal};
        }

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

void Player::setAvatarId(int entityId) {
    avatarEntityId = entityId;
}

int Player::getAvatarId() const {
    return avatarEntityId;
}