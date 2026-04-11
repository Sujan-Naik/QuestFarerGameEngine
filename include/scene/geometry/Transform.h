#ifndef QUESTFARERGAMEENGINE_TRANSFORM_H
#define QUESTFARERGAMEENGINE_TRANSFORM_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

struct Transform {
    glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);

    glm::vec3 minBounds = glm::vec3(0.0f);
    glm::vec3 maxBounds = glm::vec3(0.0f);

    // Model's forward convention: which axis points "forward" in the model file
    // e.g., glm::vec3(1.0f, 0.0f, 0.0f) for +X (Mixamo), glm::vec3(0.0f, 0.0f, 1.0f) for +Z (engine default)
    glm::vec3 modelForward = glm::vec3(0.0f, 0.0f, 1.0f);

    Transform() = default;

    explicit Transform(std::vector<glm::vec3> vertices){
        calculateBounds(vertices);
    }

    Transform(glm::vec3 pos, std::vector<glm::vec3> vertices)
            : position(pos) {
        calculateBounds(vertices);
    }

    // Constructor with custom model forward convention
    Transform(glm::vec3 pos, glm::vec3 modelFwd, std::vector<glm::vec3> vertices)
            : position(pos), modelForward(modelFwd) {
        calculateBounds(vertices);
    }

    void calculateBounds(const std::vector<glm::vec3>& vertices) {
        if (vertices.empty()) {
            minBounds = glm::vec3(0.0f);
            maxBounds = glm::vec3(0.0f);
            return;
        }

        minBounds = vertices[0];
        maxBounds = vertices[0];

        for (const auto& vertex : vertices) {
            minBounds = glm::min(minBounds, vertex);
            maxBounds = glm::max(maxBounds, vertex);
        }
    }

    glm::vec3 getSize() const {
        return (maxBounds - minBounds) * scale;
    }

    glm::vec3 getTop() const {
        return position + glm::vec3(0.0f, maxBounds.y * scale.y, 0.0f);
    }

    glm::vec3 getBottom() const {
        return position + glm::vec3(0.0f, minBounds.y * scale.y, 0.0f);
    }

    glm::mat4 matrix() const {
        return glm::translate(glm::mat4(1.0f), position)
               * glm::mat4_cast(rotation)
               * glm::scale(glm::mat4(1.0f), scale);
    }

    const glm::vec3 &getScale() const {
        return scale;
    }

    glm::vec3 getForward() const {
        return glm::normalize(rotation * modelForward);  // LHS: rotation * vector
            }

    glm::vec3 getRight() const {
        return glm::normalize(glm::vec3(1.0f, 0.0f, 0.0f) * rotation);
    }

    glm::vec3 getLeft() const {
        return -getRight();
    }

    glm::vec3 getUp() const {
        return glm::normalize(glm::vec3(0.0f, 1.0f, 0.0f) * rotation);
    }

    glm::vec3 getDown() const {
        return -getUp();
    }

    glm::vec3 getBack() const {
        return -getForward();
    }
};

#endif //QUESTFARERGAMEENGINE_TRANSFORM_H