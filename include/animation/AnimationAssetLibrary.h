#ifndef QUESTFARERGAMEENGINE_ANIMATIONASSETLIBRARY_H
#define QUESTFARERGAMEENGINE_ANIMATIONASSETLIBRARY_H

#include <map>
#include <string>
#include <memory>
#include <vector>
#include <iostream>
#include <algorithm>
#include "../rendering/model/ModelAnimation.h"
#include "Animation.h"

using namespace rendering::mesh;

namespace animation {
    struct AnimationAssetLibrary {

        std::shared_ptr<ModelAnimation> model;

        std::vector<std::shared_ptr<ModelAnimation>> sourceFiles;

        std::map<std::string, std::shared_ptr<Animation>> animations;

        // Helper method to strip unwanted Blender prefixes and suffixes
        std::string sanitizeAnimationName(std::string animName, unsigned int defaultIndex = 0) const {
            // Remove any prefix ending in '|' (e.g., "Armature|", "Character|")
            size_t pipePos = animName.rfind('|');
            if (pipePos != std::string::npos) {
                animName = animName.substr(pipePos + 1);
            }

            // Remove trailing Blender instance numbers like ".001", ".002"
            size_t dotPos = animName.rfind('.');
            if (dotPos != std::string::npos && dotPos + 4 == animName.length()) {
                std::string suffix = animName.substr(dotPos + 1);
                if (suffix.length() == 3 && std::all_of(suffix.begin(), suffix.end(), ::isdigit)) {
                    animName = animName.substr(0, dotPos);
                }
            }

            // Fallback if the name ends up empty
            if (animName.empty()) {
                animName = "Animation_" + std::to_string(defaultIndex);
            }

            return animName;
        }

        void loadFromGLB(const std::string &path) {
            auto newSource = std::make_shared<ModelAnimation>(path);
            const aiScene *scene = newSource->GetScene();

            if (!scene) {
                std::cerr << "Failed to load GLB: " << path << std::endl;
                return;
            }

            if (!model) {
                model = newSource;
            }

            sourceFiles.push_back(newSource);

            for (unsigned int i = 0; i < scene->mNumAnimations; i++) {
                auto anim = std::make_shared<Animation>(path, scene, i, model.get());

                anim->LinkBonesWithModel(*model);

                std::string animName = sanitizeAnimationName(scene->mAnimations[i]->mName.C_Str(), i);
                animations[animName] = anim;

                std::cout << "[Library] Registered Animation: " << animName
                          << " (from " << path << ")" << std::endl;
            }
        }

        void loadFromFBX(const std::string &path, const std::string &animationName) {
            auto newSource = std::make_shared<ModelAnimation>(path);

            if (!newSource || !newSource->GetScene()) {
                std::cerr << "Failed to load FBX: " << path << std::endl;
                return;
            }

            if (!model) {
                model = newSource;
            }

            sourceFiles.push_back(newSource);

            const aiScene *scene = newSource->GetScene();

            for (unsigned int i = 0; i < scene->mNumAnimations; i++) {
                auto anim = std::make_shared<Animation>(path, scene, i, model.get());

                anim->LinkBonesWithModel(*model);

                anim->ApplyCoordinateSystemConversion();

                std::string cleanName = sanitizeAnimationName(animationName, i);
                animations[cleanName] = anim;

                std::cout << "[Library] Registered Animation: " << cleanName
                          << " (from " << path << ")" << std::endl;
            }
        }

        void loadFromFBX(const std::string &path) {
            auto newSource = std::make_shared<ModelAnimation>(path);

            if (!newSource || !newSource->GetScene()) {
                std::cerr << "Failed to load FBX: " << path << std::endl;
                return;
            }

            if (!model) {
                model = newSource;
                model->InitializeFootBones();
            }

            sourceFiles.push_back(newSource);

            const aiScene *scene = newSource->GetScene();

            if (scene->mNumMeshes == 0) {
                std::cerr << "[WARNING] No meshes found in FBX: " << path << std::endl;
            } else {
                std::cout << "[INFO] Found " << scene->mNumMeshes << " mesh(es)" << std::endl;
            }

            for (unsigned int i = 0; i < scene->mNumAnimations; i++) {
                auto anim = std::make_shared<Animation>(path, scene, i, model.get());

                anim->LinkBonesWithModel(*model);

                std::string animName = sanitizeAnimationName(scene->mAnimations[i]->mName.C_Str(), i);

                animations[animName] = anim;

                std::cout << "[Library] Registered Animation: " << animName
                          << " (from " << path << ")" << std::endl;
            }
        }

        std::shared_ptr<Animation> get(const std::string &name) {
            auto it = animations.find(name);
            if (it != animations.end()) return it->second;

            std::cerr << "WARNING: Animation '" << name << "' not found in library!" << std::endl;
            return nullptr;
        }
    };
}
#endif