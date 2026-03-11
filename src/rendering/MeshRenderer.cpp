
#include "../../include/rendering/MeshRenderer.h"

MeshRenderer::MeshRenderer(std::shared_ptr<Logger> logger, std::unique_ptr<Shader>shader)
        : logger(std::move(logger)), shader(std::move(shader)) {

}
