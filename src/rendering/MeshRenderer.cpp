
#include "../../include/rendering/MeshRenderer.h"
#include "../../include/logger/Logger.h"

using namespace logger;
using namespace rendering;

MeshRenderer::MeshRenderer(std::shared_ptr<Logger> logger, std::unique_ptr<Shader>shader)
        : logger(std::move(logger)), shader(std::move(shader)) {

}
