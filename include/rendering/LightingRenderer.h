

#ifndef QUESTFARERGAMEENGINE_LIGHTINGRENDERER_H
#define QUESTFARERGAMEENGINE_LIGHTINGRENDERER_H


#include <memory>
#include "../logger/Logger.h"
#include "Shader.h"

/**
 * @class LightingRenderer
 * Handles the setup and drawing of lighting
 */
class LightingRenderer {
private:

    unsigned int VAO, lightCubeVAO;

    std::shared_ptr<Logger> logger;

    std::unique_ptr<Shader> lightCubeShader;

public:

    /**
     * @brief Constructor - Creates a new light
     * @param logger a reference to a shared debug log
     */
    explicit LightingRenderer(std::shared_ptr<Logger> logger);

    /**
     * Handles setup for lighting
     */
    void setupLighting();

    /**
     * Handles the drawing for lighting
     * @param cameraPosition
     * @param projection
     * @param view
     */
    void drawLighting(glm::vec3 cameraPosition, glm::mat4 projection, glm::mat4 view);
};


#endif //QUESTFARERGAMEENGINE_LIGHTINGRENDERER_H
