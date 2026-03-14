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
        src/generator/Generator.cpp
        include/generator/Generator.h
        include/generator/Grid.h
        src/utils/Utils.cpp
        include/utils/Utils.h
        include/generator/Square.h
        src/rendering/LightingRenderer.cpp
        include/rendering/LightingRenderer.h
        include/mesh/Mesh.h
        include/rendering/MeshRenderer.h
        src/rendering/MeshRenderer.cpp
        src/generator/Square.cpp
        include/globals.h
        external/MC/MC.h
        external/MC/noise.h
        src/rendering/TerrainMCRenderer.cpp
        include/rendering/TerrainMCRenderer.h
        include/model/Model.h
        include/components/Component.h
        include/components/ContainerObject.h
        include/components/GameObject.h
        include/components/AIComponent.h
        include/components/PhysicsComponent.h
        include/components/RenderComponent.h
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