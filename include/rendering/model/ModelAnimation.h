#ifndef QUESTFARERGAMEENGINE_ANIMATIONMODEL_H
#define QUESTFARERGAMEENGINE_ANIMATIONMODEL_H

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include "Model.h"
#include "../../animation/animdata.h"
#include "../../animation/assimp_glm_helpers.h"
#include "../mesh/MeshAnimation.h"
#include "glm/gtx/quaternion.hpp"
#include "../../animation/Animation.h"

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
#include <utility>
#include <vector>
#include <assert.h>
#include <functional>
#include <chrono>

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
    std::vector<glm::mat4> finalBoneMatrices;

    std::vector<std::string> footBoneNames = {"B-foot.L", "B-foot.R"};
    std::vector<int> footBoneIndices;
    bool footBonesInitialized = false;

    int GetParentBoneId(int childBoneId) const
    {
        std::string childName = "";
        for (const auto& [name, info] : m_BoneInfoMap) {
            if (info.id == childBoneId) {
                childName = name;
                break;
            }
        }

        if (childName.empty() || !m_Scene || !m_Scene->mRootNode) {
            return -1;
        }

        aiNode* childNode = m_Scene->mRootNode->FindNode(childName.c_str());
        if (!childNode || !childNode->mParent) {
            return -1;
        }

        aiNode* parentNode = childNode->mParent;
        while (parentNode) {
            std::string parentName = parentNode->mName.C_Str();
            auto it = m_BoneInfoMap.find(parentName);
            if (it != m_BoneInfoMap.end()) {
                return it->second.id;
            }
            parentNode = parentNode->mParent;
        }

        return -1;
    }

    void SetFootAdjustments(const std::vector<glm::mat4>& adjustedBones) {
        footAdjustmentMatrices = adjustedBones;
    }

    std::vector<glm::mat4> GetAdjustedBoneMatrices() const {
        if (footAdjustmentMatrices.empty()) {
            return finalBoneMatrices;
        }
        return footAdjustmentMatrices;
    }

    void ClearFootAdjustments() {
        footAdjustmentMatrices.clear();
    }

    std::vector<glm::mat4> GetFinalBoneMatrices() const{
        return finalBoneMatrices;
    }

    void SetFinalBoneMatrices(std::vector<glm::mat4> newFinalBoneMatrices){
        finalBoneMatrices = std::move(newFinalBoneMatrices);
    }

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

    const auto& GetBoneInfoMap() const { return m_BoneInfoMap; }
    auto& GetBoneInfoMap() { return m_BoneInfoMap; }

    int& GetBoneCount() { return m_BoneCounter; }
    int GetBoneCount() const { return m_BoneCounter; }

    const AssimpNodeData* GetRootNode() const {
        return &m_RootNode;
    }

    void InitializeFootBones()
    {
        footBoneIndices.clear();
        for (const auto& footName : footBoneNames) {
            auto it = m_BoneInfoMap.find(footName);
            if (it != m_BoneInfoMap.end()) {
                footBoneIndices.push_back(it->second.id);
            }
        }
        footBonesInitialized = !footBoneIndices.empty();
    }

    std::vector<glm::vec3> GetFootPositions(const glm::vec3& characterPos,
                                            const std::vector<glm::mat4>& boneMatrices) const
    {
        std::vector<glm::vec3> footPositions;

        for (int boneIdx : footBoneIndices) {
            if (boneIdx >= 0 && boneIdx < static_cast<int>(boneMatrices.size())) {
                glm::vec3 footLocalPos = glm::vec3(boneMatrices[boneIdx][3]);
                glm::vec3 footWorldPos = characterPos + footLocalPos;
                footPositions.push_back(footWorldPos);
            }
        }

        return footPositions;
    }

    struct IKChain {
        int upperBoneIdx;
        int lowerBoneIdx;
        int footBoneIdx;
        float upperLength;
        float lowerLength;
    };

    struct AdjustedLeg {
        glm::mat4 upper;
        glm::mat4 lower;
        glm::mat4 foot;
    };

    static AdjustedLeg SolveIK(const glm::mat4& upperMatrix, const glm::mat4& lowerMatrix, const glm::mat4& footMatrix,
                               glm::vec3 targetPos, glm::vec3 poleVector, float upperLen, float lowerLen)
    {
        auto safeRotation = [](const glm::vec3& from, const glm::vec3& to) -> glm::quat {
            const float d = glm::dot(from, to);
            if (d >= 1.0f - 1e-6f)  return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
            if (d <= -1.0f + 1e-6f) {
                glm::vec3 perp = (glm::abs(from.x) < 0.9f) ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0);
                return glm::angleAxis(glm::pi<float>(), glm::normalize(glm::cross(from, perp)));
            }
            return glm::rotation(from, to);
        };

        AdjustedLeg result = {upperMatrix, lowerMatrix, footMatrix};

        glm::vec3 upperPos    = glm::vec3(upperMatrix[3]);
        glm::vec3 animKneePos = glm::vec3(lowerMatrix[3]);
        glm::vec3 animFootPos = glm::vec3(footMatrix[3]);

        glm::vec3 toTarget = targetPos - upperPos;
        float dist = glm::length(toTarget);
        const float EPSILON = 0.005f;

        if (dist < EPSILON) return result;

        float maxDist = (upperLen + lowerLen);
        float softMaxDist = maxDist * 0.95f;
        float minDist = glm::abs(upperLen - lowerLen) + EPSILON;

        if (dist > softMaxDist) {
            float scale = softMaxDist + (maxDist - softMaxDist) * (1.0f - glm::exp(-(dist - softMaxDist) / (maxDist - softMaxDist)));
            targetPos = upperPos + (toTarget / dist) * scale;
            toTarget = targetPos - upperPos;
            dist = scale;
        } else if (dist < minDist) {
            targetPos = upperPos + (toTarget / dist) * minDist;
            toTarget = targetPos - upperPos;
            dist = minDist;
        }

        glm::vec3 targetDir = glm::normalize(toTarget);
        glm::vec3 localHingeAxis = glm::normalize(glm::vec3(upperMatrix[0]));

        glm::vec3 poleOffset  = poleVector - upperPos;
        glm::vec3 poleBendDir = poleOffset - targetDir * glm::dot(poleOffset, targetDir);

        glm::vec3 bendDir;
        if (glm::length(poleBendDir) > 0.001f) {
            bendDir = glm::normalize(poleBendDir);
        } else {
            bendDir = glm::normalize(glm::cross(localHingeAxis, targetDir));
            if (glm::length(bendDir) < 0.001f) {
                glm::vec3 alt = (glm::abs(targetDir.y) < 0.9f) ? glm::vec3(0, 1, 0) : glm::vec3(0, 0, 1);
                bendDir = glm::normalize(glm::cross(alt, targetDir));
            }
        }

        float cosUpperJoint = (upperLen * upperLen + dist * dist - lowerLen * lowerLen) / (2.0f * upperLen * dist);
        cosUpperJoint = glm::clamp(cosUpperJoint, -1.0f, 1.0f);
        float angle = glm::acos(cosUpperJoint);

        float d = upperLen * cosUpperJoint;
        float r = upperLen * glm::sin(angle);

        glm::vec3 finalKneePos = upperPos + (targetDir * d) + (bendDir * r);

        glm::vec3 animThighDir  = glm::normalize(animKneePos - upperPos);
        glm::vec3 finalThighDir = glm::normalize(finalKneePos - upperPos);
        glm::quat upperSwing    = safeRotation(animThighDir, finalThighDir);

        glm::vec3 upperTwistAxis = glm::normalize(glm::vec3(upperMatrix[1]));
        glm::vec3 upperProj = glm::dot(glm::vec3(upperSwing.x, upperSwing.y, upperSwing.z), upperTwistAxis) * upperTwistAxis;
        glm::quat upperTwist = glm::normalize(glm::quat(upperSwing.w, upperProj.x, upperProj.y, upperProj.z));
        upperSwing = upperSwing * glm::inverse(upperTwist);

        result.upper = glm::mat4_cast(upperSwing) * upperMatrix;
        result.upper[3] = glm::vec4(upperPos, 1.0f);

        glm::mat4 inheritedLowerMatrix = glm::mat4_cast(upperSwing) * lowerMatrix;
        glm::vec3 inheritedShinDir = upperSwing * glm::normalize(animFootPos - animKneePos);
        glm::vec3 finalShinDir     = glm::normalize(targetPos - finalKneePos);
        glm::quat lowerSwing       = safeRotation(inheritedShinDir, finalShinDir);

        glm::vec3 lowerTwistAxis = glm::normalize(glm::vec3(inheritedLowerMatrix[1]));
        glm::vec3 lowerProj = glm::dot(glm::vec3(lowerSwing.x, lowerSwing.y, lowerSwing.z), lowerTwistAxis) * lowerTwistAxis;
        glm::quat lowerTwist = glm::normalize(glm::quat(lowerSwing.w, lowerProj.x, lowerProj.y, lowerProj.z));
        lowerSwing = lowerSwing * glm::inverse(lowerTwist);

        result.lower = glm::mat4_cast(lowerSwing) * inheritedLowerMatrix;
        result.lower[3] = glm::vec4(finalKneePos, 1.0f);

        glm::quat combinedRotation = lowerSwing * upperSwing;
        result.foot = glm::mat4_cast(combinedRotation) * footMatrix;
        result.foot[3] = glm::vec4(targetPos, 1.0f);

        return result;
    }

    std::vector<glm::mat4> AdjustBonesForTerrainCollisionIK(
            const std::vector<glm::mat4>& originalBoneMatrices,
            const glm::mat4& characterWorldMatrix,
            float maxStepHeight,
            const std::function<float(glm::vec3)>& getTerrainHeight) const
    {
        auto cleanGlobalMatrices = originalBoneMatrices;
        for (const auto& [name, boneInfo] : m_BoneInfoMap) {
            cleanGlobalMatrices[boneInfo.id] = originalBoneMatrices[boneInfo.id] * glm::inverse(boneInfo.offset);
        }

        auto adjustedGlobals = cleanGlobalMatrices;
        glm::mat4 inverseCharacterMatrix = glm::inverse(characterWorldMatrix);

        struct LegBones {
            std::string thigh, shin, foot, toe;
            int thighIdx, shinIdx, footIdx, toeIdx;
        };

        std::vector<LegBones> legs = {
                {"B-thigh.L", "B-shin.L", "B-foot.L", "B-toe.L", -1, -1, -1, -1},
                {"B-thigh.R", "B-shin.R", "B-foot.R", "B-toe.R", -1, -1, -1, -1}
        };

        std::vector<glm::vec3> footWorldPositions(legs.size());
        std::vector<float> worldDropDistances(legs.size());
        std::vector<bool> needsGrounding(legs.size(), false);
        float maxDropNeededWorld = 0.0f;

        for (size_t i = 0; i < legs.size(); ++i) {
            legs[i].thighIdx = GetBoneIndex(legs[i].thigh);
            legs[i].shinIdx  = GetBoneIndex(legs[i].shin);
            legs[i].footIdx  = GetBoneIndex(legs[i].foot);
            legs[i].toeIdx   = GetBoneIndex(legs[i].toe);

            glm::mat4 modelFootMat = cleanGlobalMatrices[legs[i].footIdx];
            glm::vec3 footWorldPos = glm::vec3(characterWorldMatrix * modelFootMat[3]);
            float worldTerrainHeight = getTerrainHeight(footWorldPos);

            footWorldPositions[i] = footWorldPos;

            if (footWorldPos.y < worldTerrainHeight + maxStepHeight) {
                float diff = worldTerrainHeight - footWorldPos.y;
                if (diff > 0.0f) {
                    worldDropDistances[i] = diff;
                    needsGrounding[i] = true;
                    if (diff > maxDropNeededWorld) {
                        maxDropNeededWorld = diff;
                    }
                }
            }
        }

        float dynamicPelvisDropWorld = glm::clamp(maxDropNeededWorld, 0.0f, maxStepHeight);

        int pelvisIdx = GetBoneIndex("B-root");
        if (pelvisIdx != -1 && dynamicPelvisDropWorld > 0.001f) {
            glm::vec3 pelvisWorldPos = glm::vec3(characterWorldMatrix * cleanGlobalMatrices[pelvisIdx][3]);
            pelvisWorldPos.y -= dynamicPelvisDropWorld;
            adjustedGlobals[pelvisIdx][3] = inverseCharacterMatrix * glm::vec4(pelvisWorldPos, 1.0f);
        }

        glm::vec3 modelDropVec = glm::vec3(inverseCharacterMatrix * glm::vec4(0.0f, dynamicPelvisDropWorld, 0.0f, 0.0f));

        for (size_t i = 0; i < legs.size(); ++i) {
            const auto& leg = legs[i];

            glm::mat4 modelThighMat = cleanGlobalMatrices[leg.thighIdx];
            glm::mat4 modelShinMat  = cleanGlobalMatrices[leg.shinIdx];
            glm::mat4 modelFootMat  = cleanGlobalMatrices[leg.footIdx];

            modelThighMat[3] -= glm::vec4(modelDropVec, 0.0f);
            modelShinMat[3]  -= glm::vec4(modelDropVec, 0.0f);
            modelFootMat[3]  -= glm::vec4(modelDropVec, 0.0f);

            if (!needsGrounding[i] && dynamicPelvisDropWorld <= 0.001f) {
                continue;
            }

            float upperLen = glm::distance(glm::vec3(modelShinMat[3]), glm::vec3(modelThighMat[3]));
            float lowerLen = glm::distance(glm::vec3(modelFootMat[3]), glm::vec3(modelShinMat[3]));

            glm::vec3 targetFootWorldPos = footWorldPositions[i];
            if (needsGrounding[i]) {
                targetFootWorldPos.y = footWorldPositions[i].y + worldDropDistances[i] + 1;
            } else {
                targetFootWorldPos.y = footWorldPositions[i].y;
            }

            glm::vec3 targetFootModelPos = glm::vec3(inverseCharacterMatrix * glm::vec4(targetFootWorldPos, 1.0f));

            glm::vec3 stablePoleVectorModel = glm::vec3(modelShinMat[3]);
            stablePoleVectorModel.z -= 1.0f;

            auto legResult = SolveIK(modelThighMat, modelShinMat, modelFootMat,
                                     targetFootModelPos, stablePoleVectorModel, upperLen, lowerLen);

            adjustedGlobals[leg.thighIdx] = legResult.upper;
            adjustedGlobals[leg.shinIdx]  = legResult.lower;
            adjustedGlobals[leg.footIdx]  = legResult.foot;

            glm::mat4 modelToeMat = cleanGlobalMatrices[leg.toeIdx];
            modelToeMat[3] -= glm::vec4(modelDropVec, 0.0f);

            glm::mat4 localToeMat = glm::inverse(modelFootMat) * modelToeMat;
            adjustedGlobals[leg.toeIdx] = legResult.foot * localToeMat;
        }

        auto finalSkinningMatrices = originalBoneMatrices;
        for (const auto& [name, boneInfo] : m_BoneInfoMap) {
            finalSkinningMatrices[boneInfo.id] = adjustedGlobals[boneInfo.id] * boneInfo.offset;
        }

        return finalSkinningMatrices;
    }

    int GetBoneIndex(const std::string& boneName) const {
        auto it = m_BoneInfoMap.find(boneName);
        return it != m_BoneInfoMap.end() ? it->second.id : -1;
    }

    bool WouldFeetHitTerrain(
            const glm::mat4& characterWorldMatrix,
            const std::vector<glm::mat4>& boneMatrices,
            float maxStepHeight,
            std::function<float(glm::vec3)> getTerrainHeight) const
    {
        if (!footBonesInitialized || footBoneIndices.empty()) {
            return false;
        }

        for (int boneIdx : footBoneIndices) {
            if (boneIdx < 0 || boneIdx >= static_cast<int>(boneMatrices.size())) {
                continue;
            }

            glm::vec4 modelFootPos = boneMatrices[boneIdx][3];
            glm::vec3 footWorldPos = glm::vec3(characterWorldMatrix * modelFootPos);

            float terrainHeight = getTerrainHeight(footWorldPos);
            float stepRequired = terrainHeight - footWorldPos.y;

            if (stepRequired > maxStepHeight) {
                return true;
            }
        }

        return false;
    }

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

    std::vector<std::vector<glm::vec3>> getAnimatedMeshVertices(const std::vector<glm::mat4> &boneMatrices) const {
        std::vector<std::vector<glm::vec3>> animatedVertices;

        for (const auto& mesh : meshes) {
            std::vector<glm::vec3> positions;
            for (const auto& vertex : mesh.vertices) {
                glm::vec3 finalPos(0.0f);
                for (int i = 0; i < MAX_BONE_INFLUENCE; i++) {
                    if (vertex.m_BoneIDs[i] < 0) break;
                    finalPos += glm::vec3(boneMatrices[vertex.m_BoneIDs[i]] * glm::vec4(vertex.Position, 1.0f)) * vertex.m_Weights[i];
                }
                positions.push_back(finalPos);
            }
            animatedVertices.push_back(positions);
        }
        return animatedVertices;
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

        std::replace(filename.begin(), filename.end(), '\\', '/');

        size_t lastSlash = filename.find_last_of('/');
        if (lastSlash != string::npos)
        {
            filename = filename.substr(lastSlash + 1);
        }

        filename = directory + '/' + filename;

        int width, height, nrComponents;
        unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);

        if (!data)
        {
            std::cout << "TEXTURE FAILED TO LOAD AT PATH: " << filename << std::endl;
            return 0;
        }

        unsigned int textureID;
        glGenTextures(1, &textureID);

        GLenum format = GL_RGB;
        if (nrComponents == 1) format = GL_RED;
        else if (nrComponents == 3) format = GL_RGB;
        else if (nrComponents == 4) format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
        return textureID;
    }

private:
    AssimpNodeData m_RootNode;
    std::vector<glm::mat4> footAdjustmentMatrices;
    Assimp::Importer m_Importer;
    const aiScene* m_Scene;
    std::map<string, BoneInfo> m_BoneInfoMap;
    int m_BoneCounter = 0;

    void ReadNodeHierarchy(const aiNode* src, AssimpNodeData& dest) {
        dest.name = src->mName.data;
        dest.transformation = AssimpGLMHelpers::ConvertMatrixToGLMFormat(src->mTransformation);
        dest.childrenCount = src->mNumChildren;

        for (unsigned int i = 0; i < src->mNumChildren; i++) {
            AssimpNodeData childData;
            ReadNodeHierarchy(src->mChildren[i], childData);
            dest.children.push_back(childData);
        }
    }

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

        ReadNodeHierarchy(m_Scene->mRootNode, m_RootNode);

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

            int numWeights = mesh->mBones[boneIndex]->mNumWeights;
            std::cout << "[Bone] " << boneName << " has " << numWeights << " vertex weights" << std::endl;

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

            auto weights = mesh->mBones[boneIndex]->mWeights;
            for (int weightIndex = 0; weightIndex < numWeights; ++weightIndex)
            {
                int vertexId = weights[weightIndex].mVertexId;
                float weight = weights[weightIndex].mWeight;
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