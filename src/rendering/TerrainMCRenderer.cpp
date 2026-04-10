#define MC_IMPLEM_ENABLE
#include "MC/noise.h"
#include "MC/MC.h"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <stdexcept>

#include "../../include/rendering/TerrainMCRenderer.h"
#include "../../include/globals.h"
using namespace logger;
using namespace rendering;
using namespace rendering::mesh;

TerrainMCRenderer::TerrainMCRenderer(std::shared_ptr<Logger> logger,
                                     std::unique_ptr<Shader> shader)
        : MeshRenderer(std::move(logger), std::move(shader))
{
}

void TerrainMCRenderer::setTexture(unsigned int glTextureId)
{
    textureId = glTextureId;
}

void TerrainMCRenderer::setup()
{
    float* field = new float[MC_GRID_X * MC_GRID_Y * MC_GRID_Z];

    for (int i = 0; i < MC_GRID_X; i++)
        for (int j = 0; j < MC_GRID_Y; j++)
            for (int k = 0; k < MC_GRID_Z; k++)
            {
                bool boundary = (i == 0 || i == MC_GRID_X - 1 ||
                                 j == 0 || j == MC_GRID_Y - 1 ||
                                 k == 0 || k == MC_GRID_Z - 1);

                field[(k * MC_GRID_Y + j) * MC_GRID_X + i] = boundary
                                                             ? -1.0f
                                                             : PerlinNoise::GetValue(i * MC_NOISE_FREQ,
                                                                                     j * MC_NOISE_FREQ,
                                                                                     k * MC_NOISE_FREQ);
            }

    MC::mcMesh mcMeshData;
    MC::marching_cube(field, MC_GRID_X, MC_GRID_Y, MC_GRID_Z, mcMeshData);
    delete[] field;

    if (mcMeshData.vertices.empty())
        throw std::runtime_error("TerrainMCRenderer: marching_cube produced no vertices");

    std::vector<Vertex> vertices;
    vertices.reserve(mcMeshData.vertices.size());

    for (size_t i = 0; i < mcMeshData.vertices.size(); i++)
    {
        Vertex vert;
        vert.Position  = { mcMeshData.vertices[i].x,
                           mcMeshData.vertices[i].y,
                           mcMeshData.vertices[i].z };
        vert.Normal    = { mcMeshData.normals[i].x,
                           mcMeshData.normals[i].y,
                           mcMeshData.normals[i].z };
        vert.TexCoords = { mcMeshData.vertices[i].x / (float)MC_GRID_X,
                           mcMeshData.vertices[i].z / (float)MC_GRID_Z };
        vertices.push_back(vert);
    }

    Texture tex;
    tex.id   = textureId;
    tex.type = "texture_diffuse";

    mesh.emplace(vertices, mcMeshData.indices, std::vector<Texture>{ tex }); // <-- was mesh->Draw(*shader)
}

void TerrainMCRenderer::draw(const RenderContext& ctx, glm::mat4 modelMatrix)
{
    if (!mesh.has_value()) return;

    shader->use();
    shader->setMat4("projection", ctx.projection);
    shader->setMat4("view", ctx.view);

    // Lighting uniforms — without these the fragment shader outputs black
    shader->setVec3("viewPos",    ctx.cameraPosition);
    shader->setVec3("lightPos",   glm::vec3(EFFECTIVE_SIDE_LENGTH / 2.0f,
                                            HEIGHT_UPPER_BOUND,
                                            EFFECTIVE_SIDE_LENGTH / 2.0f));
    shader->setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));


//    modelMatrix = glm::translate(model, glm::vec3(
//            EFFECTIVE_SIDE_LENGTH / 2.0f - (MC_GRID_X * MC_WORLD_SCALE) / 2.0f,
//            0.0f,
//            EFFECTIVE_SIDE_LENGTH / 2.0f - (MC_GRID_Z * MC_WORLD_SCALE) / 2.0f
//    ));
//    model = glm::scale(model, glm::vec3(MC_WORLD_SCALE));


    shader->setMat4("model", modelMatrix);


    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);
    mesh->Draw(*shader);
    glBindTexture(GL_TEXTURE_2D, 0);
}