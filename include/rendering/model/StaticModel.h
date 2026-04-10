#ifndef QUESTFARERGAMEENGINE_STATICMODEL_H
#define QUESTFARERGAMEENGINE_STATICMODEL_H

#include "Model.h"

namespace rendering {
    namespace model {

        class StaticModel : public Model {
        public:
            vector<mesh::Mesh> meshes;

            StaticModel(string const &path, bool gamma = false) : Model(path, gamma) {loadModel(path);}

            void Draw(Shader &shader) override {
                for (unsigned int i = 0; i < meshes.size(); i++)
                    meshes[i].Draw(shader);
            }

        private:
            void processNode(aiNode *node, const aiScene *scene) override {
                for (unsigned int i = 0; i < node->mNumMeshes; i++) {
                    aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
                    meshes.push_back(processMesh(mesh, scene));
                }
                for (unsigned int i = 0; i < node->mNumChildren; i++) {
                    processNode(node->mChildren[i], scene);
                }
            }

            mesh::Mesh processMesh(aiMesh *mesh, const aiScene *scene) {
                vector<mesh::Vertex> vertices;
                vector<unsigned int> indices;
                vector<mesh::Texture> textures;

                for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
                    mesh::Vertex vertex;
                    glm::vec3 vector;

                    vector.x = mesh->mVertices[i].x;
                    vector.y = mesh->mVertices[i].y;
                    vector.z = mesh->mVertices[i].z;
                    vertex.Position = vector;

                    if (mesh->HasNormals()) {
                        vector.x = mesh->mNormals[i].x;
                        vector.y = mesh->mNormals[i].y;
                        vector.z = mesh->mNormals[i].z;
                        vertex.Normal = vector;
                    }

                    if (mesh->mTextureCoords[0]) {
                        glm::vec2 vec;
                        vec.x = mesh->mTextureCoords[0][i].x;
                        vec.y = mesh->mTextureCoords[0][i].y;
                        vertex.TexCoords = vec;

                        vector.x = mesh->mTangents[i].x;
                        vector.y = mesh->mTangents[i].y;
                        vector.z = mesh->mTangents[i].z;
                        vertex.Tangent = vector;

                        vector.x = mesh->mBitangents[i].x;
                        vector.y = mesh->mBitangents[i].y;
                        vector.z = mesh->mBitangents[i].z;
                        vertex.Bitangent = vector;
                    } else {
                        vertex.TexCoords = glm::vec2(0.0f, 0.0f);
                    }

                    vertices.push_back(vertex);
                }

                for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
                    aiFace face = mesh->mFaces[i];
                    for (unsigned int j = 0; j < face.mNumIndices; j++)
                        indices.push_back(face.mIndices[j]);
                }

                aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];

                vector<mesh::Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE,
                                                                         "texture_diffuse");
                textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
                vector<mesh::Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR,
                                                                          "texture_specular");
                textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
                vector<mesh::Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT,
                                                                        "texture_normal");
                textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
                vector<mesh::Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT,
                                                                        "texture_height");
                textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

                return mesh::Mesh(vertices, indices, textures);
            }
        };

    }
}

#endif //QUESTFARERGAMEENGINE_STATICMODEL_H