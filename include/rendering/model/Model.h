#ifndef QUESTFARERGAMEENGINE_MODEL_H
#define QUESTFARERGAMEENGINE_MODEL_H

#include "glad/glad.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "stb/stb_image.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <string>
#include <iostream>
#include <map>
#include <vector>
#include "../mesh/Mesh.h"

using namespace std;

    namespace rendering::model {
        unsigned int TextureFromFile(const char *path, const string &directory, bool gamma)
        {
            string filename = string(path);
            filename = directory + '/' + filename;

            unsigned int textureID;
            glGenTextures(1, &textureID);

            int width, height, nrComponents;
            unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
            if (data)
            {
                GLenum format;
                if (nrComponents == 1)
                    format = GL_RED;
                else if (nrComponents == 3)
                    format = GL_RGB;
                else if (nrComponents == 4)
                    format = GL_RGBA;

                glBindTexture(GL_TEXTURE_2D, textureID);
                glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
                glGenerateMipmap(GL_TEXTURE_2D);

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                stbi_image_free(data);
            }
            else
            {
                std::cout << "Texture failed to load at path: " << path << std::endl;
                stbi_image_free(data);
            }

            return textureID;
        }

        class Model {
        public:
            vector <mesh::Texture> textures_loaded;
            string directory;
            bool gammaCorrection;

            Model(string const &path, bool gamma = false) : gammaCorrection(gamma) {
            }

            virtual ~Model() = default;

            virtual void Draw(Shader &shader) = 0;

        protected:
            void loadModel(string const &path) {
                Assimp::Importer importer;
                const aiScene *scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals |
                                                               aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

                if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
                    cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
                    return;
                }

                directory = path.substr(0, path.find_last_of('/'));
                processNode(scene->mRootNode, scene);
            }

            virtual void processNode(aiNode *node, const aiScene *scene) = 0;

            vector <mesh::Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, string typeName) {
                vector<mesh::Texture> textures;
                for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
                    aiString str;
                    mat->GetTexture(type, i, &str);

                    bool skip = false;
                    for (unsigned int j = 0; j < textures_loaded.size(); j++) {
                        if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0) {
                            textures.push_back(textures_loaded[j]);
                            skip = true;
                            break;
                        }
                    }

                    if (!skip) {
                        mesh::Texture texture;
                        texture.id = TextureFromFile(str.C_Str(), this->directory, false);
                        texture.type = typeName;
                        texture.path = str.C_Str();
                        textures.push_back(texture);
                        textures_loaded.push_back(texture);
                    }
                }
                return textures;
            }
        };

    }

#endif //QUESTFARERGAMEENGINE_MODEL_H