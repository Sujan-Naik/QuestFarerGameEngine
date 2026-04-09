#include "../include/Camera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

Camera::Camera() = default;

void Camera::processKeyboard(CameraMovement direction, double deltaTime)
{
    const float speed = DEFAULT_SPEED * deltaTime;
    const glm::vec3 right = glm::normalize(glm::cross(front, up));

    switch (direction)
    {
        case CameraMovement::FORWARD:  position += front * speed; break;
        case CameraMovement::BACKWARD: position -= front * speed; break;
        case CameraMovement::LEFT:     position -= right * speed; break;
        case CameraMovement::RIGHT:    position += right * speed; break;
    }
}

void Camera::processMouseMovement(float xpos, float ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    yaw   += (xpos - lastX) * DEFAULT_SENSITIVITY;
    pitch  = glm::clamp(pitch + (lastY - ypos) * DEFAULT_SENSITIVITY, -89.0f, 89.0f);
    lastX  = xpos;
    lastY  = ypos;

    updateCameraVectors();
}

void Camera::processScroll(float yoffset)
{
    fov = glm::clamp(fov - yoffset, 1.0f, 70.0f);
}

glm::mat4 Camera::getViewMatrix() const
{
    return glm::lookAt(position, position + front, up);
}

glm::mat4 Camera::getProjectionMatrix(float aspectRatio) const
{
    return glm::perspective(glm::radians(fov), aspectRatio, 0.1f, 10000.0f);
}

const glm::vec3& Camera::getPosition() const
{
    return position;
}

void Camera::updateCameraVectors()
{
    front = glm::normalize(glm::vec3{
            cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
            sin(glm::radians(pitch)),
            sin(glm::radians(yaw)) * cos(glm::radians(pitch))
    });
}

const glm::vec3& Camera::getFront() const { return front; }
