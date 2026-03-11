#ifndef QUESTFARERGAMEENGINE_CLOUDRENDERER_H
#define QUESTFARERGAMEENGINE_CLOUDRENDERER_H


#include "CloudSimulator.h"

/**
 * @class CloudRenderer
 * @brief Handles the separation between the setup of vertex information and the drawing.
 *
 */
class CloudRenderer {

private:


    unsigned int VAO{};

    std::shared_ptr<Logger> logger;

    std::shared_ptr<CloudSimulator> simulator;

    std::unique_ptr<Shader> shader;

public:

    CloudRenderer(std::shared_ptr<Logger> logger, std::shared_ptr<CloudSimulator> simulator);

    void setupVertexData();

    void drawClouds(glm::vec3 cameraPosition, glm::mat4 projection, glm::mat4 view, glm::vec3 offset);

};



#endif //QUESTFARERGAMEENGINE_CLOUDRENDERER_H
