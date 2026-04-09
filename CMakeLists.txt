cmake_minimum_required(VERSION 3.26)
project(QuestFarerGameEngine)

set(CMAKE_CXX_STANDARD 23)



add_executable(QuestFarerGameEngine src/main.cpp
        external/KHR/khrplatform.h
        external/glad/glad.h
        external/glad/glad.c
        include/rendering/Shader.h
        src/rendering/Shader.cpp
        external/stb/stb_image.h
        external/stb/stb_image.cpp
        src/Camera.cpp
        include/Camera.h
        include/logger/Logger.h
        src/utils/Utils.cpp
        include/utils/Utils.h
        src/rendering/LightingRenderer.cpp
        include/rendering/LightingRenderer.h
        include/mesh/Mesh.h
        include/rendering/MeshRenderer.h
        src/rendering/MeshRenderer.cpp
        include/globals.h
        external/MC/MC.h
        external/MC/noise.h
        src/rendering/TerrainMCRenderer.cpp
        include/rendering/TerrainMCRenderer.h
        include/model/Model.h
        include/scene/components/Component.h
        include/scene/components/ContainerObject.h
        include/scene/objects/GameObject.h
        include/scene/components/AIComponent.h
        include/scene/components/PhysicsComponent.h
        include/scene/objects/ModelObject.h
        include/scene/objects/CustomMeshObject.h
        include/rendering/RenderContext.h
        include/scene/geometry/Transform.h
        include/voxel/VoxelType.h
        src/voxel/Chunk.cpp
        include/voxel/Chunk.h
        src/voxel/Grid.cpp
        include/voxel/Grid.h
        src/voxel/VoxelQuery.cpp
        include/voxel/VoxelQuery.h
        src/rendering/VoxelRenderer.cpp
        include/rendering/VoxelRenderer.h
        src/player/Player.cpp
        include/player/Player.h
        include/animation/Animator.h
        include/animation/Bone.h
        include/animation/assimp_glm_helpers.h
        include/animation/model_animation.h
        include/animation/animdata.h
        include/animation/MeshAnimation.h
        include/animation/Animation.h
        include/scene/objects/AnimationModelObject.h
        src/physics/PhysicsSystem.cpp
        include/physics/PhysicsSystem.h
        include/world/World.cpp
        include/world/World.h
)

add_custom_command(
        TARGET QuestFarerGameEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/resources
        ${CMAKE_BINARY_DIR}/resources
        COMMENT "Copying resources..."
)

find_package(assimp REQUIRED)
target_link_libraries(QuestFarerGameEngine assimp)

find_package(glfw3 3.3 REQUIRED)
target_link_libraries(QuestFarerGameEngine glfw)
target_include_directories(QuestFarerGameEngine PUBLIC
        external)