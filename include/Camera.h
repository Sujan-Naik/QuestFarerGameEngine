#ifndef QUESTFARERGAMEENGINE_CAMERA_H
#define QUESTFARERGAMEENGINE_CAMERA_H

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

enum class CameraMovement { FORWARD, BACKWARD, LEFT, RIGHT };

class Camera
{
public:
    static constexpr float DEFAULT_SPEED       = 2.5f;
    static constexpr float DEFAULT_SENSITIVITY = 0.1f;
    static constexpr float DEFAULT_FOV         = 45.0f;

    Camera();

    void processKeyboard(CameraMovement direction, float deltaTime);
    void processMouseMovement(float xpos, float ypos);
    void processScroll(float yoffset);

    [[nodiscard]] glm::mat4 getViewMatrix() const;
    [[nodiscard]] glm::mat4 getProjectionMatrix(float aspectRatio) const;

    [[nodiscard]] const glm::vec3& getPosition() const;

private:
    glm::vec3 position = glm::vec3(0.0f, 0.0f,  3.0f);
    glm::vec3 front    = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 up       = glm::vec3(0.0f, 1.0f,  0.0f);

    float yaw   = -90.0f;
    float pitch =   0.0f;
    float fov   = DEFAULT_FOV;

    bool  firstMouse = true;
    float lastX      = 0.0f;
    float lastY      = 0.0f;

    void updateCameraVectors();
};

#endif //QUESTFARERGAMEENGINE_CAMERA_H