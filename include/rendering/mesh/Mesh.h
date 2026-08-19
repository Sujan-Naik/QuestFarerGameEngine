#ifndef QUESTFARERGAMEENGINE_MESH_H
#define QUESTFARERGAMEENGINE_MESH_H

#include "glad/glad.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <string>
#include <vector>
#include <cstddef>
#include "../Shader.h"

#define MAX_BONE_INFLUENCE 4

namespace rendering {
    namespace mesh {

        struct Vertex {
            glm::vec3 Position;
            glm::vec3 Normal;
            glm::vec2 TexCoords;
            glm::vec3 Tangent;
            glm::vec3 Bitangent;

            // Atlas tile index, in tile coordinates rather than normalized UVs.
            // This is passed as a separate flat attribute so greedy quads
            // can repeat one atlas tile without interpolating into another.
            glm::vec2 AtlasTile;
        };

        struct Texture {
            unsigned int id;
            std::string type;
            std::string path;
        };

        class Mesh {
        public:
            std::vector<Vertex> vertices;
            std::vector<unsigned int> indices;
            std::vector<Texture> textures;
            unsigned int VAO;

            Mesh(
                    std::vector<Vertex> vertices,
                    std::vector<unsigned int> indices,
                    std::vector<Texture> textures)
                    : vertices(std::move(vertices)),
                      indices(std::move(indices)),
                      textures(std::move(textures)) {
                setupMesh();
            }

            virtual ~Mesh() = default;

            virtual void Draw(Shader &shader) {
                bindTextures(shader);

                glBindVertexArray(VAO);

                glDrawElements(
                        GL_TRIANGLES,
                        static_cast<unsigned int>(indices.size()),
                        GL_UNSIGNED_INT,
                        0
                );

                glBindVertexArray(0);
                glActiveTexture(GL_TEXTURE0);
            }

        protected:
            unsigned int VBO = 0;
            unsigned int EBO = 0;

            void bindTextures(Shader &shader) {
                unsigned int diffuseNr = 1;
                unsigned int specularNr = 1;
                unsigned int normalNr = 1;
                unsigned int heightNr = 1;

                for (unsigned int i = 0; i < textures.size(); i++) {
                    glActiveTexture(GL_TEXTURE0 + i);

                    std::string number;
                    std::string name = textures[i].type;

                    if (name == "texture_diffuse") {
                        number = std::to_string(diffuseNr++);
                    }
                    else if (name == "texture_specular") {
                        number = std::to_string(specularNr++);
                    }
                    else if (name == "texture_normal") {
                        number = std::to_string(normalNr++);
                    }
                    else if (name == "texture_height") {
                        number = std::to_string(heightNr++);
                    }

                    glUniform1i(
                            glGetUniformLocation(
                                    shader.ID,
                                    (name + number).c_str()
                            ),
                            i
                    );

                    glBindTexture(
                            GL_TEXTURE_2D,
                            textures[i].id
                    );
                }
            }

            void setupMesh() {
                glGenVertexArrays(1, &VAO);
                glGenBuffers(1, &VBO);
                glGenBuffers(1, &EBO);

                glBindVertexArray(VAO);

                glBindBuffer(GL_ARRAY_BUFFER, VBO);
                glBufferData(
                        GL_ARRAY_BUFFER,
                        vertices.size() * sizeof(Vertex),
                        vertices.empty() ? nullptr : vertices.data(),
                        GL_STATIC_DRAW
                );

                glBindBuffer(
                        GL_ELEMENT_ARRAY_BUFFER,
                        EBO
                );

                glBufferData(
                        GL_ELEMENT_ARRAY_BUFFER,
                        indices.size() * sizeof(unsigned int),
                        indices.empty() ? nullptr : indices.data(),
                        GL_STATIC_DRAW
                );

                setupVertexAttributes();

                glBindVertexArray(0);
            }

            virtual void setupVertexAttributes() {
                // Position
                glEnableVertexAttribArray(0);
                glVertexAttribPointer(
                        0,
                        3,
                        GL_FLOAT,
                        GL_FALSE,
                        sizeof(Vertex),
                        (void*) offsetof(Vertex, Position)
                );

                // Normal
                glEnableVertexAttribArray(1);
                glVertexAttribPointer(
                        1,
                        3,
                        GL_FLOAT,
                        GL_FALSE,
                        sizeof(Vertex),
                        (void*) offsetof(Vertex, Normal)
                );

                // Texture coordinates / local repeating coordinates
                glEnableVertexAttribArray(2);
                glVertexAttribPointer(
                        2,
                        2,
                        GL_FLOAT,
                        GL_FALSE,
                        sizeof(Vertex),
                        (void*) offsetof(Vertex, TexCoords)
                );

                // Tangent
                glEnableVertexAttribArray(3);
                glVertexAttribPointer(
                        3,
                        3,
                        GL_FLOAT,
                        GL_FALSE,
                        sizeof(Vertex),
                        (void*) offsetof(Vertex, Tangent)
                );

                // Bitangent
                glEnableVertexAttribArray(4);
                glVertexAttribPointer(
                        4,
                        3,
                        GL_FLOAT,
                        GL_FALSE,
                        sizeof(Vertex),
                        (void*) offsetof(Vertex, Bitangent)
                );

                // Atlas tile.
                //
                // This is deliberately a normal vertex attribute at the
                // vertex stage. The voxel vertex shader declares the output
                // as 'flat', so all vertices of a greedy quad use exactly
                // the same atlas tile.
                glEnableVertexAttribArray(5);
                glVertexAttribPointer(
                        5,
                        2,
                        GL_FLOAT,
                        GL_FALSE,
                        sizeof(Vertex),
                        (void*) offsetof(Vertex, AtlasTile)
                );
            }
        };
    }
}

#endif // QUESTFARERGAMEENGINE_MESH_H