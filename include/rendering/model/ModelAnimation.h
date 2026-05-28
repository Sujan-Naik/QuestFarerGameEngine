#ifndef QUESTFARERGAMEENGINE_ANIMATIONMODEL_H
#define QUESTFARERGAMEENGINE_ANIMATIONMODEL_H

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include "Model.h"
#include "../../animation/animdata.h"
#include "../../animation/assimp_glm_helpers.h"
#include "../mesh/MeshAnimation.h"
#include "glm/gtx/quaternion.hpp"

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

    // Foot bone tracking
    std::vector<std::string> footBoneNames = {"B-foot.L", "B-foot.R"};
    std::vector<int> footBoneIndices;
    bool footBonesInitialized = false;


    /**
     * @brief Store temporary foot adjustments (applied only for rendering this frame).
     * Called by physics system after collision resolution.
     */
    void SetFootAdjustments(const std::vector<glm::mat4>& adjustedBones) {
        footAdjustmentMatrices = adjustedBones;
    }

    /**
     * @brief Get bone matrices with foot adjustments applied.
     * Used during rendering to apply terrain-fitted feet.
     */
    std::vector<glm::mat4> GetAdjustedBoneMatrices() const {
        if (footAdjustmentMatrices.empty()) {
            return finalBoneMatrices;
        }
        return footAdjustmentMatrices;
    }

    /**
     * @brief Clear foot adjustments (call at start of frame before physics).
     */
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

    auto& GetBoneInfoMap() { return m_BoneInfoMap; }
    int& GetBoneCount() { return m_BoneCounter; }

    /**
     * @brief Initialize foot bone indices from bone info map.
     * Call once after model is loaded.
     */
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

    /**
     * @brief Get world-space foot positions given character position and bone matrices.
     */
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
        int upperBoneIdx;  // thigh
        int lowerBoneIdx;  // shin
        int footBoneIdx;   // foot (target)
        float upperLength; // thigh length
        float lowerLength; // shin length
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

        result.upper = glm::mat4_cast(upperSwing) * upperMatrix;
        result.upper[3] = glm::vec4(upperPos, 1.0f);

        glm::mat4 inheritedLowerMatrix = glm::mat4_cast(upperSwing) * lowerMatrix;
        glm::vec3 inheritedShinDir = upperSwing * glm::normalize(animFootPos - animKneePos);
        glm::vec3 finalShinDir     = glm::normalize(targetPos - finalKneePos);
        glm::quat lowerSwing       = safeRotation(inheritedShinDir, finalShinDir);

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
            std::function<float(glm::vec3)> getTerrainHeight) const
    {
        static uint32_t diagnosticLogCounter = 0;
        bool shouldLog = (diagnosticLogCounter++ % 60 == 0);

        if (originalBoneMatrices.empty() || !footBonesInitialized) {
            return originalBoneMatrices;
        }

        const std::vector<glm::mat4> inputLocalBones = originalBoneMatrices;
        auto adjustedMatrices = originalBoneMatrices;

        struct LegBones { std::string thigh, shin, foot; int thighIdx, shinIdx, footIdx; };
        std::vector<LegBones> legs = {
                {"B-thigh.L", "B-shin.L", "B-foot.L", -1, -1, -1},
                {"B-thigh.R", "B-shin.R", "B-foot.R", -1, -1, -1}
        };

        float maxDropNeededLocal = 0.0f;
        std::vector<glm::vec3> footLocalPositions(legs.size());
        std::vector<float> targetHeightsLocal(legs.size());
        std::vector<float> individualHipOffsetsY(legs.size(), 0.0f);

        auto fixMatrixBasis = [](glm::mat4& m) {
            glm::vec3 x = glm::normalize(glm::vec3(m[0]));
            glm::vec3 y = glm::normalize(glm::vec3(m[1]));
            glm::vec3 z = glm::normalize(glm::cross(x, y));
            x = glm::cross(y, z);
            m[0] = glm::vec4(x, 0.0f);
            m[1] = glm::vec4(y, 0.0f);
            m[2] = glm::vec4(z, 0.0f);
        };

        for (size_t i = 0; i < legs.size(); ++i) {
            legs[i].thighIdx = GetBoneIndex(legs[i].thigh);
            legs[i].shinIdx  = GetBoneIndex(legs[i].shin);
            legs[i].footIdx  = GetBoneIndex(legs[i].foot);

            if (legs[i].thighIdx < 0 || legs[i].shinIdx < 0 || legs[i].footIdx < 0) {
                return originalBoneMatrices;
            }

            glm::mat4 localThighMat = inputLocalBones[legs[i].thighIdx];
            glm::mat4 localShinMat  = localThighMat * inputLocalBones[legs[i].shinIdx];
            glm::mat4 localFootMat  = localShinMat  * inputLocalBones[legs[i].footIdx];

            glm::vec3 footWorldPos = glm::vec3(characterWorldMatrix * localFootMat[3]);
            float worldTerrainHeight = getTerrainHeight(footWorldPos) + 0.05f;

            float worldDropDistance = footWorldPos.y - worldTerrainHeight;
            footLocalPositions[i] = glm::vec3(localFootMat[3]);

            targetHeightsLocal[i] = footLocalPositions[i].y - worldDropDistance;
            individualHipOffsetsY[i] = localThighMat[3].y;

            if (worldDropDistance > maxDropNeededLocal) {
                maxDropNeededLocal = worldDropDistance;
            }
        }

        float dynamicPelvisDropLocal = glm::clamp(maxDropNeededLocal, 0.0f, maxStepHeight);

        // Calculate structural variance between left and right animation hips
        float structuralHipDeltaY = individualHipOffsetsY[0] - individualHipOffsetsY[1];

        for (size_t i = 0; i < legs.size(); ++i) {
            const auto& leg = legs[i];

            glm::mat4 origThigh = inputLocalBones[leg.thighIdx];
            glm::mat4 origShin  = origThigh * inputLocalBones[leg.shinIdx];
            glm::mat4 origFoot  = origShin  * inputLocalBones[leg.footIdx];

            glm::vec3 localShinOffsetFromThigh = glm::vec3(inputLocalBones[leg.shinIdx][3]);
            glm::vec3 localFootOffsetFromShin  = glm::vec3(inputLocalBones[leg.footIdx][3]);

            glm::mat4 upperLocalMat = origThigh;
            glm::mat4 lowerLocalMat = origShin;
            glm::mat4 footLocalMat  = origFoot;

            fixMatrixBasis(upperLocalMat);
            fixMatrixBasis(lowerLocalMat);
            fixMatrixBasis(footLocalMat);

            glm::vec3 stableHipJointLocalRoot = glm::vec3(inputLocalBones[leg.thighIdx][3]);

            upperLocalMat[3].y -= dynamicPelvisDropLocal;
            lowerLocalMat[3].y -= dynamicPelvisDropLocal;
            footLocalMat[3].y  -= dynamicPelvisDropLocal;

            float upperLen = glm::distance(glm::vec3(lowerLocalMat[3]), glm::vec3(upperLocalMat[3]));
            float lowerLen = glm::distance(glm::vec3(footLocalMat[3]), glm::vec3(lowerLocalMat[3]));

            if (upperLen < 0.001f || lowerLen < 0.001f) continue;

            // Balance target heights dynamically using the computed structural variance factor
            float asymmetryCompensation = (i == 0) ? (structuralHipDeltaY * 0.5f) : 0.0f;

            glm::vec3 targetFootLocalPos = glm::vec3(
                    footLocalPositions[i].x,
                    targetHeightsLocal[i] + (dynamicPelvisDropLocal * 0.95f) + asymmetryCompensation,
                    footLocalPositions[i].z
            );

            glm::vec3 stablePoleVectorLocal = glm::vec3(lowerLocalMat[3]);
            stablePoleVectorLocal.z += 5.0f;

            auto legResult = SolveIK(upperLocalMat, lowerLocalMat, footLocalMat,
                                     targetFootLocalPos, stablePoleVectorLocal, upperLen, lowerLen);

            adjustedMatrices[leg.thighIdx] = legResult.upper;
            adjustedMatrices[leg.thighIdx][3] = glm::vec4(stableHipJointLocalRoot, 1.0f);

            glm::mat4 localShinTransform = glm::inverse(legResult.upper) * legResult.lower;
            localShinTransform[3] = glm::vec4(localShinOffsetFromThigh, 1.0f);
            adjustedMatrices[leg.shinIdx] = localShinTransform;

            glm::mat4 localFootTransform = glm::inverse(legResult.lower) * legResult.foot;
            localFootTransform[3] = glm::vec4(localFootOffsetFromShin, 1.0f);
            adjustedMatrices[leg.footIdx] = localFootTransform;

            if (shouldLog) {
                glm::vec3 finalThighPos = glm::vec3(adjustedMatrices[leg.thighIdx][3]);
                std::cout << "  [Leg " << i << " (" << (i == 0 ? "Left" : "Right") << ")] "
                          << "Original Hip Pos: (" << stableHipJointLocalRoot.x << ", " << stableHipJointLocalRoot.y << ", " << stableHipJointLocalRoot.z << ") | "
                          << "Output Hip Pos: (" << finalThighPos.x << ", " << finalThighPos.y << ", " << finalThighPos.z << ")\n";
            }
        }

        return adjustedMatrices;
    }



    int GetBoneIndex(const std::string& boneName) const {
        auto it = m_BoneInfoMap.find(boneName);
        return it != m_BoneInfoMap.end() ? it->second.id : -1;
    }



    /**
   * @brief Check if foot adjustment would be necessary (for early collision detection).
   */
    bool WouldFeetHitTerrain(
            const glm::mat4& characterWorldMatrix, // Changed to mat4 to match
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

            // Convert Model Space foot position to World Space correctly using the matrix
            glm::vec4 modelFootPos = boneMatrices[boneIdx][3];
            glm::vec3 footWorldPos = glm::vec3(characterWorldMatrix * modelFootPos);

            float terrainHeight = getTerrainHeight(footWorldPos);
            float stepRequired = terrainHeight - footWorldPos.y;

            if (stepRequired > maxStepHeight) {
                return true;  // Blocked
            }
        }

        return false;  // Clear
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

    std::vector<glm::mat4> footAdjustmentMatrices;
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

            // DEBUG
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