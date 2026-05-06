#ifndef QUESTFARERGAMEENGINE_ANIMATIONMODEL_H
#define QUESTFARERGAMEENGINE_ANIMATIONMODEL_H

#include "Model.h"
#include "../../animation/animdata.h"
#include "../../animation/assimp_glm_helpers.h"
#include "../mesh/MeshAnimation.h"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb/stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
#include <assert.h>

using namespace std;
using namespace rendering::mesh;
using namespace rendering;
using namespace animation;

class ModelAnimation
{
public:
    vector<Texture> textures_loaded;
    vector<MeshAnimation> meshes;
    string directory;
    bool gammaCorrection;

    ModelAnimation(string const &path, bool gamma = false) : gammaCorrection(gamma)
    {
        loadModel(path);
    }

    const aiScene* GetScene() const
    {
        return m_Scene;
    }

    void Draw(Shader &shader)
    {
        for(unsigned int i = 0; i < meshes.size(); i++)
            meshes[i].Draw(shader);
    }

    auto& GetBoneInfoMap() { return m_BoneInfoMap; }
    int& GetBoneCount() { return m_BoneCounter; }

    std::vector<glm::vec3> getMeshVerticesCollapsed() const {
        std::vector<glm::vec3> allVertices;
        for (const auto& mesh : meshes) {
            for (const auto& vertex : mesh.vertices) {
                allVertices.push_back(vertex.Position);
            }
        }
        return allVertices;
    }

    std::vector<std::vector<glm::vec3>> getMeshVertices() const {
        std::vector<std::vector<glm::vec3>> meshVertices;
        for (const auto& mesh : meshes) {
            std::vector<glm::vec3> positions;
            for (const auto& vertex : mesh.vertices) {
                positions.push_back(vertex.Position);
            }
            meshVertices.push_back(positions);
        }
        return meshVertices;
    }

    std::vector<std::vector<unsigned int>> getMeshIndices() const {
        std::vector<std::vector<unsigned int>> meshIndices;
        for (const auto& mesh : meshes) {
            meshIndices.push_back(mesh.indices);
        }
        return meshIndices;
    }

    unsigned int TextureFromFile(const char* path, const string& directory, bool gamma = false)
    {
        string filename = string(path);
        filename = directory + '/' + filename;

        unsigned int textureID;
        glGenTextures(1, &textureID);

        int width, height, nrComponents;
        unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
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
            stbi_image_free(data);
        }
        return textureID;
    }

private:
    Assimp::Importer m_Importer;
    const aiScene* m_Scene;
    std::map<string, BoneInfo> m_BoneInfoMap;
    int m_BoneCounter = 0;

    void loadModel(string const &path)
    {
        m_Scene = m_Importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace | aiProcess_LimitBoneWeights | aiProcess_PopulateArmatureData );

        if (!m_Scene) {
            cout << "ERROR::ASSIMP::Scene is null" << endl;
            return;
        }

        if (m_Scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
            cout << "WARNING::ASSIMP::Scene incomplete - missing some resources" << endl;
        }

        if (!m_Scene->mRootNode) {
            cout << "ERROR::ASSIMP::Root node is null" << endl;
            return;
        }

        directory = path.substr(0, path.find_last_of('/'));

        processNode(m_Scene->mRootNode, m_Scene);
    }

    void processNode(aiNode *node, const aiScene *scene)
    {
        for(unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }
        for(unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene);
        }
    }

    void SetVertexBoneDataToDefault(AnimVertex& vertex)
    {
        for (int i = 0; i < MAX_BONE_INFLUENCE; i++)
        {
            vertex.m_BoneIDs[i] = -1;
            vertex.m_Weights[i] = 0.0f;
        }
    }

    MeshAnimation processMesh(aiMesh* mesh, const aiScene* scene)
    {
        vector<AnimVertex> vertices;
        vector<unsigned int> indices;
        vector<Texture> textures;

        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            AnimVertex vertex;
            SetVertexBoneDataToDefault(vertex);
            vertex.Position = AssimpGLMHelpers::GetGLMVec(mesh->mVertices[i]);
            vertex.Normal = AssimpGLMHelpers::GetGLMVec(mesh->mNormals[i]);

            if (mesh->mTextureCoords[0])
            {
                vertex.TexCoords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
            }
            else
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);

            vertices.push_back(vertex);
        }
        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
        std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
        std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
        textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

        ExtractBoneWeightForVertices(vertices, mesh, scene);

        return MeshAnimation(vertices, indices, textures);
    }

    void SetVertexBoneData(AnimVertex& vertex, int boneID, float weight)
    {
        for (int i = 0; i < MAX_BONE_INFLUENCE; ++i)
        {
            if (vertex.m_BoneIDs[i] < 0)
            {
                vertex.m_Weights[i] = weight;
                vertex.m_BoneIDs[i] = boneID;
                break;
            }
        }
    }

    void ExtractBoneWeightForVertices(std::vector<AnimVertex>& vertices, aiMesh* mesh, const aiScene* scene)
    {
        for (int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
        {
            int boneID = -1;
            std::string boneName = mesh->mBones[boneIndex]->mName.C_Str();
            if (m_BoneInfoMap.find(boneName) == m_BoneInfoMap.end())
            {
                BoneInfo newBoneInfo;
                newBoneInfo.id = m_BoneCounter;
                newBoneInfo.offset = AssimpGLMHelpers::ConvertMatrixToGLMFormat(mesh->mBones[boneIndex]->mOffsetMatrix);
                m_BoneInfoMap[boneName] = newBoneInfo;
                boneID = m_BoneCounter;
                m_BoneCounter++;
            }
            else
            {
                boneID = m_BoneInfoMap[boneName].id;
            }
            assert(boneID != -1);
            auto weights = mesh->mBones[boneIndex]->mWeights;
            int numWeights = mesh->mBones[boneIndex]->mNumWeights;

            for (int weightIndex = 0; weightIndex < numWeights; ++weightIndex)
            {
                int vertexId = weights[weightIndex].mVertexId;
                float weight = weights[weightIndex].mWeight;
                assert(vertexId < vertices.size());
                SetVertexBoneData(vertices[vertexId], boneID, weight);
            }
        }
    }



    vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, string typeName)
    {
        vector<Texture> textures;
        for(unsigned int i = 0; i < mat->GetTextureCount(type); i++)
        {
            aiString str;
            mat->GetTexture(type, i, &str);
            bool skip = false;
            for(unsigned int j = 0; j < textures_loaded.size(); j++)
            {
                if(std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
                {
                    textures.push_back(textures_loaded[j]);
                    skip = true;
                    break;
                }
            }
            if(!skip)
            {
                Texture texture;
                texture.id = TextureFromFile(str.C_Str(), this->directory);
                texture.type = typeName;
                texture.path = str.C_Str();
                textures.push_back(texture);
                textures_loaded.push_back(texture);
            }
        }
        return textures;
    }
};

#endif