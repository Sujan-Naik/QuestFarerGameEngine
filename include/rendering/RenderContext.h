#ifndef QUESTFARERGAMEENGINE_RENDERCONTEXT_H
#define QUESTFARERGAMEENGINE_RENDERCONTEXT_H

#include "glm/detail/type_mat4x4.hpp"

namespace rendering {

/**
* @struct Defines necessary information for rendering
* @param cameraPosition The position of the camera currently
* @param projection A projection matrix for perspective rendering
* @param view View matrix for camera transformation
*/
    struct RenderContext {

        glm::vec3 cameraPosition;
        glm::mat4 projection;
        glm::mat4 view;
    };
}

#endif //QUESTFARERGAMEENGINE_RENDERCONTEXT_H
