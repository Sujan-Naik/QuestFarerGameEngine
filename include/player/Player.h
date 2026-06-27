#ifndef QUESTFARERGAMEENGINE_PLAYER_H
#define QUESTFARERGAMEENGINE_PLAYER_H

#include <memory>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "../../include/Camera.h"
#include "../../include/rendering/RenderContext.h"
#include "../../include/globals.h"
#include "../../include/voxel/Grid.h"
#include "../scene/objects/GameObject.h"
#include "../scene/components/CharacterControllerComponent.h"
#include "../scene/components/ECSManager.h"

using namespace voxel;

namespace player {
    struct RaycastResult {
        bool hit = false;
        glm::ivec3 voxelPos;
        glm::ivec3 normal;
    };

    class Player {
    private:
        std::unique_ptr<Camera> camera = std::make_unique<Camera>();
        int avatarEntityId; // Changed from raw pointer to ID
        std::shared_ptr<Grid> grid;

        RaycastResult raycast(float maxDistance);

        const float aspectRatio = static_cast<float>(SCR_WIDTH) / static_cast<float>(SCR_HEIGHT);
        bool cursorEnabled = false;

        void handleRightClick();
        double actionTimer = 0.0;

        std::shared_ptr<scene::components::ECSManager> ecsManager;

    public:
        Player(std::shared_ptr<Grid> gridPtr, int entityId,  std::shared_ptr<scene::components::ECSManager> ecsManager);

        void updateCamera(scene::components::ECSManager& ecs); // Needs ECS access

        [[nodiscard]] const rendering::RenderContext getRenderContext() const;

        void processInput(GLFWwindow *window, double elapsed, scene::components::ECSManager& ecs); // Needs ECS access

        static void mouse_callback(GLFWwindow *, double xpos, double ypos);
        static void mouse_button_callback(GLFWwindow *window, int button, int action, int);
        static void scroll_callback(GLFWwindow *, double, double yoffset);

        void processMouseMovement(float xpos, float ypos);
        void processScroll(float yoffset);

        void setAvatarId(int entityId);
        int getAvatarId() const;

        glm::vec3 getPosition(scene::components::ECSManager& ecs); // Needs ECS access
    };
}

#endif