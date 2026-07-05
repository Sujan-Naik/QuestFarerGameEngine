#ifndef QUESTFARERGAMEENGINE_ANIMATION_H
#define QUESTFARERGAMEENGINE_ANIMATION_H

#include <vector>
#include <map>
#include <glm/glm.hpp>
#include <assimp/scene.h>
#include <functional>
#include "animdata.h"
#include "Bone.h"
#include "../rendering/model/ModelAnimation.h"

using namespace rendering::model;
namespace animation {

    struct AssimpNodeData {
        glm::mat4 transformation;
        std::string name;
        int childrenCount;
        std::vector<AssimpNodeData> children;
    };

    class Animation {
    public:
        Animation() = default;

        Animation(const std::string &animationPath, ModelAnimation *model) {
            Assimp::Importer importer;
            const aiScene *scene = importer.ReadFile(animationPath, aiProcess_Triangulate);

            if (!scene || !scene->mRootNode) {
                throw std::runtime_error("Animation file failed to load: " + animationPath);
            }

            auto animation = scene->mAnimations[0];
            m_Duration = animation->mDuration;
            m_TicksPerSecond = animation->mTicksPerSecond;
            ReadHierarchyData(m_RootNode, scene->mRootNode);
            ReadMissingBones(animation, *model);
        }

        Animation(const std::string &path, const aiScene* scene, unsigned int animationIndex, ModelAnimation* model) {
            if (!scene || animationIndex >= scene->mNumAnimations) {
                throw std::runtime_error("Invalid scene or animation index for: " + path);
            }

            auto animation = scene->mAnimations[animationIndex];
            m_Duration = animation->mDuration;
            m_TicksPerSecond = animation->mTicksPerSecond;

            ReadHierarchyData(m_RootNode, scene->mRootNode);
            ReadMissingBones(animation, *model);
        }

        Animation(const std::string &animationPath, unsigned int animationIndex = 0) {
            Assimp::Importer importer;
            const aiScene *scene = importer.ReadFile(animationPath, aiProcess_Triangulate);

            if (!scene || !scene->mRootNode) {
                throw std::runtime_error("Animation file failed to load: " + animationPath);
            }

            if (animationIndex >= scene->mNumAnimations) {
                throw std::runtime_error("Animation index out of range: " + animationPath);
            }

            auto animation = scene->mAnimations[animationIndex];
            m_Duration = animation->mDuration;
            m_TicksPerSecond = animation->mTicksPerSecond;

            ReadHierarchyData(m_RootNode, scene->mRootNode);
            ReadBonesWithoutModel(animation);
        }

        ~Animation() {}

        Bone *FindBone(const std::string &name) {
            auto iter = std::find_if(m_Bones.begin(), m_Bones.end(),
                                     [&](const Bone &Bone) {
                                         return Bone.GetBoneName() == name;
                                     }
            );
            if (iter == m_Bones.end()) return nullptr;
            else return &(*iter);
        }

        // Link bones to model after separate loading
        void LinkBonesWithModel(ModelAnimation& model) {
            auto &boneInfoMap = model.GetBoneInfoMap();
            int &boneCount = model.GetBoneCount();

            for (auto& bone : m_Bones) {
                std::string boneName = bone.GetBoneName();
                if (boneInfoMap.find(boneName) == boneInfoMap.end()) {
                    boneInfoMap[boneName].id = boneCount;
                    boneCount++;
                }
                bone.SetBoneId(boneInfoMap[boneName].id);
            }
            m_BoneInfoMap = boneInfoMap;
        }

        void ApplyCoordinateSystemConversion() {
            glm::mat4 conversion = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            ApplyConversionToNode(m_RootNode, conversion);
        }

        inline float GetTicksPerSecond() { return m_TicksPerSecond; }
        inline float GetDuration() { return m_Duration; }
        inline const AssimpNodeData &GetRootNode() { return m_RootNode; }
        inline const std::map<std::string, BoneInfo> &GetBoneIDMap() { return m_BoneInfoMap; }
        inline int GetBoneCount() const { return m_Bones.size(); }

    private:
        void ReadMissingBones(const aiAnimation *animation, ModelAnimation &model) {
            int size = animation->mNumChannels;
            auto &boneInfoMap = model.GetBoneInfoMap();
            int &boneCount = model.GetBoneCount();

            for (int i = 0; i < size; i++) {
                auto channel = animation->mChannels[i];
                std::string boneName = channel->mNodeName.data;
                if (boneName == "root") {
                    std::cout << "FOUND ROOT NODE - Channel Index: " << i << std::endl;
                }
                if (boneInfoMap.find(boneName) == boneInfoMap.end()) {
                    boneInfoMap[boneName].id = boneCount;
                    boneCount++;
                }
                m_Bones.push_back(Bone(channel->mNodeName.data,
                                       boneInfoMap[channel->mNodeName.data].id, channel));
            }
            m_BoneInfoMap = boneInfoMap;
        }

        void ReadBonesWithoutModel(const aiAnimation *animation) {
            int size = animation->mNumChannels;
            int tempBoneCounter = 0;

            for (int i = 0; i < size; i++) {
                auto channel = animation->mChannels[i];
                std::string boneName = channel->mNodeName.data;
                if (boneName == "root") {
                    std::cout << "FOUND ROOT NODE - Channel Index: " << i << std::endl;
                }
                m_Bones.push_back(Bone(boneName, tempBoneCounter, channel));
                m_BoneInfoMap[boneName].id = tempBoneCounter;
                tempBoneCounter++;
            }
        }

        void ReadHierarchyData(AssimpNodeData &dest, const aiNode *src) {
            assert(src);
            dest.name = src->mName.data;
            dest.transformation = AssimpGLMHelpers::ConvertMatrixToGLMFormat(src->mTransformation);
            dest.childrenCount = src->mNumChildren;

            for (int i = 0; i < src->mNumChildren; i++) {
                AssimpNodeData newData;
                ReadHierarchyData(newData, src->mChildren[i]);
                dest.children.push_back(newData);
            }
        }

        void ApplyConversionToNode(AssimpNodeData &node, const glm::mat4 &conversion) {
            node.transformation = conversion * node.transformation;

            for (auto &child : node.children) {
                ApplyConversionToNode(child, conversion);
            }
        }

        float m_Duration;
        int m_TicksPerSecond;
        std::vector<Bone> m_Bones;
        AssimpNodeData m_RootNode;
        std::map<std::string, BoneInfo> m_BoneInfoMap;
    };
}

#endif