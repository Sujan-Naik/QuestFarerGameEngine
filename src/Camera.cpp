#include "../include/Camera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
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
        case CameraMovement::UP:       position += up * speed; break;
        case CameraMovement::DOWN:     position -= up * speed; break;
    }
}

float Camera::getAngleWithTransform( Transform target) const {
    // 1. Get the Model's forward vector from its rotation quaternion
    // In GLM, multiplying a quaternion by (0,0,1) gives the local forward in world space
    glm::vec3 modelForward = target.rotation * glm::vec3(0.0f, 0.0f, 1.0f);

    // 2. Ensure both are normalized (Camera::front usually is already)
    glm::vec3 camForward = glm::normalize(this->front);
    modelForward = glm::normalize(modelForward);

    // 3. Calculate Dot Product and clamp to avoid NaN with acos
    float dot = glm::dot(camForward, modelForward);
    dot = glm::clamp(dot, -1.0f, 1.0f);

    // 4. Return the angle in degrees
    return glm::degrees(std::acos(dot));
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



void Camera::setTransform(const Transform& transform)
{
    position = transform.position;
    updateYawPitchFromQuaternion(transform.rotation);
}

void Camera::movePosition(glm::vec3 posDelta)
{
    position += posDelta;
}

void Camera::updateYawPitchFromQuaternion(const glm::quat& rotation)
{
    // Rotate the default forward direction (Z+) by the quaternion
    front = glm::normalize(rotation * glm::vec3(0.0f, 0.0f, 1.0f));

    // Update up direction
    up = glm::normalize(rotation * glm::vec3(0.0f, 1.0f, 0.0f));

    // Extract yaw/pitch for consistency
    // yaw: rotation around Y axis (left-right)
    // pitch: rotation around X axis (up-down)
    pitch = glm::degrees(asin(-front.y));
    yaw = glm::degrees(atan2(front.x, front.z));
}

glm::mat4 Camera::getViewMatrix() const
{
    return glm::lookAt(position, position + front, up);
}

glm::mat4 Camera::getProjectionMatrix(float aspectRatio) const
{
    return glm::perspective(glm::radians(fov), aspectRatio, 0.1f, 10000.0f);
}

void Camera::setPosition(const glm::vec3 &position) {
    Camera::position = position;
}

const glm::vec3& Camera::getPosition() const
{
    return position;
}

void Camera::updateCameraVectors()
{
    front = glm::normalize(glm::vec3{
            sin(glm::radians(yaw)) * cos(glm::radians(pitch)),
            sin(glm::radians(pitch)),
            cos(glm::radians(yaw)) * cos(glm::radians(pitch))
    });
}

const glm::vec3& Camera::getFront() const { return front; }

const glm::vec3 &Camera::getUp() const {
    return up;
}

float Camera::getYaw() const {
    return yaw;
}
