#ifndef QUESTFARERGAMEENGINE_MESHRENDERER_H
#define QUESTFARERGAMEENGINE_MESHRENDERER_H


#include <memory>
#include <utility>
#include "glm/vec3.hpp"
#include "../mesh/Mesh.h"
#include "../logger/Logger.h"
#include "RenderContext.h"

/**
 * @class MeshRenderer
 * An abstract superclass to form a basis for the requirements to generate new content in OpenGL
 */
class MeshRenderer {

protected:

    std::unique_ptr<Mesh> mesh;

    std::shared_ptr<Logger> logger;

    std::unique_ptr<Shader> shader;

public:

    /**
     * @brief Constructor - creates a new renderer
     * @param logger A shared pointer to the debug log file
     * @param shader A pointer to the shader used specifically for this renderer
     */
    MeshRenderer(std::shared_ptr<Logger> logger, std::unique_ptr<Shader>shader );

    virtual ~MeshRenderer() = default;

    /**
     * @brief A virtual setup method
     */
    virtual void setup() = 0;

    /**
     * A virtual draw method the handle the rendering
     * @param cameraPosition The position of the camera currently
     * @param projection A projection matrix for perspective rendering
     * @param view View matrix for camera transformation
     * @param offset An offset used for some arbitrary manipulation of a rendered object within the engine
     */
    virtual void draw(const RenderContext& ctx, glm::mat4 modelMatrix) = 0;
};



#endif //QUESTFARERGAMEENGINE_MESHRENDERER_H

