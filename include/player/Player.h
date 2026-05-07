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

using namespace voxel;
namespace player {
    struct RaycastResult {
        bool hit = false;
        glm::ivec3 voxelPos; // The actual voxel hit
        glm::ivec3 normal;   // Which face was hit (useful for placing blocks)
    };

    class Player {

    private:
        std::unique_ptr<Camera> camera = std::make_unique<Camera>();

        scene::components::CharacterControllerComponent* avatar;

        std::shared_ptr<Grid> grid;

        RaycastResult raycast(float maxDistance);

        const float aspectRatio = static_cast<float>(SCR_WIDTH) / static_cast<float>(SCR_HEIGHT);
        bool cursorEnabled = false;

        void handleRightClick();

        double actionTimer;

    public:

        Player(std::shared_ptr<Grid> gridPtr, scene::components::CharacterControllerComponent* avatar);

        void updateCamera();

            [[nodiscard]] const rendering::RenderContext getRenderContext() const;

        void processInput(GLFWwindow *window, double elapsed);


        static void mouse_callback(GLFWwindow *, double xpos, double ypos);

        static void mouse_button_callback(GLFWwindow *window, int button, int action, int);

        static void scroll_callback(GLFWwindow *, double, double yoffset);

        void processMouseMovement(float xpos, float ypos);

        void processScroll(float yoffset);

        void setAvatar( scene::components::CharacterControllerComponent* newAvatar);

        scene::components::CharacterControllerComponent * getAvatar();

        const glm::vec3 &getGetPosition() const;

        glm::vec3 getPosition();
    };
}

#endif //QUESTFARERGAMEENGINE_PLAYER_H
