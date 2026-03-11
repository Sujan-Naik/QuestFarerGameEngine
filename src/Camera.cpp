/**
 * Camera Implementation inspired by https://learnopengl.com/Getting-started/Camera
 */

#include "../include/Camera.h"

const glm::vec3 &Camera::getCameraPos() const {
    return cameraPos;
}

void Camera::setCameraPos(const glm::vec3 &cameraPos) {
    Camera::cameraPos = cameraPos;
}

const glm::vec3 &Camera::getCameraFront() const {
    return cameraFront;
}

void Camera::setCameraFront(const glm::vec3 &cameraFront) {
    Camera::cameraFront = cameraFront;
}

const glm::vec3 &Camera::getCameraUp() const {
    return cameraUp;
}

void Camera::setCameraUp(const glm::vec3 &cameraUp) {
    Camera::cameraUp = cameraUp;
}

Camera::Camera() {

}
