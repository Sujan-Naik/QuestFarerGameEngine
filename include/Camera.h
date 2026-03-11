/**
 * Camera Implementation inspired by https://learnopengl.com/Getting-started/Camera
 */
#ifndef QUESTFARERGAMEENGINE_CAMERA_H
#define QUESTFARERGAMEENGINE_CAMERA_H


#include "glm/vec3.hpp"

class Camera {
private:
    glm::vec3 cameraPos   = glm::vec3(0.0f, 0.0f,  3.0f);
    glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f);

public:

    Camera();

    [[nodiscard]] const glm::vec3 &getCameraPos() const;

    void setCameraPos(const glm::vec3 &cameraPos);

    [[nodiscard]] const glm::vec3 &getCameraFront() const;

    void setCameraFront(const glm::vec3 &cameraFront);

    [[nodiscard]] const glm::vec3 &getCameraUp() const;

    void setCameraUp(const glm::vec3 &cameraUp);

};


#endif //QUESTFARERGAMEENGINE_CAMERA_H
