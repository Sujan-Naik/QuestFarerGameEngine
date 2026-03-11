
#include "../../include/rendering/TerrainDSRenderer.h"

#include <utility>
#include "../include/globals.h"

void TerrainDSRenderer::draw(glm::vec3 cameraPosition, glm::mat4 projection, glm::mat4 view, glm::vec3 offset) {

    shader->use();
    shader->setVec3("lightColor", 1.0f, 1.0f, 1.0f);
    shader->setVec3("lightPos",     glm::vec3(10.0f, 10.0f, 10.0f));
    shader->setVec3("viewPos", cameraPosition);
    shader->setMat4("projection", projection);
    shader->setMat4("view", view);


    auto model = glm::scale(glm::mat4(1), {HORIZONTAL_SCALE,1,HORIZONTAL_SCALE});

    shader->setMat4("model", model);

    mesh->Draw(*shader);

}

void TerrainDSRenderer::initTerrainData(std::shared_ptr<Generator> newGenerator) {
    this->generator = std::move(newGenerator);
}

void TerrainDSRenderer::setup() {

    logger->log(DEBUG, "Attempting to generate vertex and index data...");
    const Grid& grid = generator->getGrid();
    std::vector<Vertex> vertices;
    vertices.reserve(generator->getSquares().size() * 4);
    std::vector<unsigned int> indices;
    indices.reserve(generator->getSquares().size() * 6);

    unsigned int count = 0;
    for (auto square: generator->getSquares()) {
        glm::vec3 edge1 = square.getBottomLeft(grid) - square.getTopLeft(grid);
        glm::vec3 edge2 = square.getBottomLeft(grid) - square.getBottomRight(grid);
        glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

        Vertex topRightVertex{}, bottomRightVertex{}, bottomLeftVertex{}, topLeftVertex{};

        topRightVertex.Position = square.getTopRight(grid);
        topRightVertex.Normal = normal;
        topRightVertex.TexCoords = glm::vec2(1.0f, 1.0f);

        bottomRightVertex.Position = square.getBottomRight(grid);
        bottomRightVertex.Normal = normal;
        bottomRightVertex.TexCoords = glm::vec2(1.0f, 0.0f);

        bottomLeftVertex.Position =square.getBottomLeft(grid);
        bottomLeftVertex.Normal = normal;
        bottomLeftVertex.TexCoords = glm::vec2(0.0f, 0.0f);

        topLeftVertex.Position = square.getTopLeft(grid);
        topLeftVertex.Normal = normal;
        topLeftVertex.TexCoords = glm::vec2(0.0f, 1.0f);

        vertices.push_back(topRightVertex);
        vertices.push_back(bottomRightVertex);
        vertices.push_back(bottomLeftVertex);
        vertices.push_back(topLeftVertex);

        indices.push_back(count);
        indices.push_back(count + 1);
        indices.push_back(count + 3);
        indices.push_back(count + 1);
        indices.push_back(count + 2);
        indices.push_back(count + 3);
        count += 4;
    }


    mesh = std::make_unique<Mesh>(vertices, indices, std::vector<Texture>{});
    logger->log(DEBUG, "Generated Mesh Successfully.");
}

TerrainDSRenderer::TerrainDSRenderer(std::shared_ptr<Logger> logger, std::unique_ptr<Shader> terrainShader) : MeshRenderer(
        std::move(logger), std::move(terrainShader)) {

}





