#include <assimp/anim.h>
#include "../../include/animation/Animation.h"
#include "../../include/rendering/model/ModelAnimation.h"

namespace animation {

    Animation::Animation(const std::string &animationPath, ModelAnimation *model) {
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

    Animation::Animation(const std::string &path, const aiScene* scene, unsigned int animationIndex, ModelAnimation* model) {
        if (!scene || animationIndex >= scene->mNumAnimations) {
            throw std::runtime_error("Invalid scene or animation index for: " + path);
        }

        auto animation = scene->mAnimations[animationIndex];
        m_Duration = animation->mDuration;
        m_TicksPerSecond = animation->mTicksPerSecond;

        ReadHierarchyData(m_RootNode, scene->mRootNode);
        ReadMissingBones(animation, *model);
    }

    Animation::Animation(const std::string &animationPath, unsigned int animationIndex) {
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

    void Animation::LinkBonesWithModel(ModelAnimation& model) {
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

    void Animation::ReadMissingBones(const aiAnimation *animation, ModelAnimation &model) {
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
}