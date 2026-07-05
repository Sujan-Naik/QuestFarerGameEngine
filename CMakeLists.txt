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
        include/rendering/mesh/Mesh.h
        include/rendering/MeshRenderer.h
        src/rendering/MeshRenderer.cpp
        include/globals.h
        external/MC/MC.h
        external/MC/noise.h
        src/rendering/TerrainMCRenderer.cpp
        include/rendering/TerrainMCRenderer.h
        include/rendering/model/StaticModel.h
        include/scene/components/Component.h
        include/scene/objects/GameObject.h
        include/scene/components/AIComponent.h
        include/scene/components/PhysicsComponent.h
        include/scene/objects/ModelObject.h
        include/scene/objects/CustomMeshObject.h
        include/rendering/RenderContext.h
        include/scene/geometry/Transform.h
        include/voxel/VoxelType.h
        include/voxel/Chunk.h
        include/voxel/Grid.h
        include/voxel/VoxelQuery.h
        src/rendering/VoxelRenderer.cpp
        include/rendering/VoxelRenderer.h
        src/player/Player.cpp
        include/player/Player.h
        include/animation/Bone.h
        include/animation/assimp_glm_helpers.h
        include/rendering/model/ModelAnimation.h
        include/animation/animdata.h
        include/rendering/mesh/MeshAnimation.h
        include/animation/Animation.h
        include/scene/objects/AnimationModelObject.h
        src/scene/systems/PhysicsSystem.cpp
        include/scene/systems/PhysicsSystem.h
        include/world/World.h
        src/scene/systems/PhysicsSystem.cpp
        include/character/CharacterController.cpp
        include/character/CharacterController.h
        include/rendering/model/Model.h
        include/scene/components/CharacterControllerComponent.h
        include/scene/components/ECSManager.h
        include/scene/components/fsm/AnimationFSM.h
        include/animation/AnimationAssetLibrary.h
        external/FastNoiseLite/FastNoiseLite.h
        include/world/ProceduralTerrainGenerator.h
        include/world/ProceduralTerrainGenerator.h
        include/world/ProceduralTerrainGenerator.h
        include/world/ProceduralTerrainGenerator.h
        src/scene/components/AIComponent.cpp
        src/scene/components/CharacterControllerComponent.cpp
        src/scene/components/PhysicsComponent.cpp
        src/scene/ECSManager.cpp
        src/scene/systems/System.cpp
        include/scene/systems/System.h
        include/scene/systems/System.h
        src/scene/systems/CharacterControllerSystem.cpp
        include/scene/systems/AISystem.h
        include/scene/systems/AISystem.h
        include/scene/systems/CharacterControllerSystem.h
        include/scene/systems/CharacterControllerSystem.h
        src/scene/systems/AISystem.cpp
        src/scene/systems/AISystem.cpp
        src/scene/systems/System.cpp
        src/scene/systems/CharacterControllerSystem.cpp
        include/scene/components/fsm/State.h
        include/scene/components/fsm/State.h
        include/scene/components/fsm/Transition.h
        include/scene/components/fsm/Transition.h
        include/scene/components/fsm/Transition.h
        include/scene/components/fsm/AnimationFSM.h
        src/scene/components/fsm/AnimationFSM.cpp
        src/scene/components/fsm/AnimationFSM.cpp
        src/scene/components/fsm/AnimationFSM.cpp
        include/scene/components/fsm/ConcreteStates.h
        src/scene/components/fsm/ConcreteStates.cpp
        include/scene/components/fsm/ConcreteTransitions.h
        src/scene/components/fsm/ConcreteTransitions.cpp
        include/scene/components/fsm/AnimationState.h
        src/scene/components/AttackComponent.cpp
        include/scene/components/AttackComponent.h
        src/scene/components/HealthComponent.cpp
        include/scene/components/HealthComponent.h
        src/scene/systems/AttackSystem.cpp
        include/scene/systems/AttackSystem.h
        include/scene/components/fsm/BoxingPunch.h
        src/scene/ECSManager.cpp
)

add_custom_target(ForceCopyResources
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/resources"
        "${CMAKE_BINARY_DIR}/resources"
        COMMENT "Forcefully syncing resources..."
)

add_dependencies(QuestFarerGameEngine ForceCopyResources)

find_package(assimp REQUIRED)
target_link_libraries(QuestFarerGameEngine assimp)

find_package(glfw3 3.3 REQUIRED)
target_link_libraries(QuestFarerGameEngine glfw)
target_include_directories(QuestFarerGameEngine PUBLIC
        external)