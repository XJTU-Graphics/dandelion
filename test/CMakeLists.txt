cmake_minimum_required(VERSION 3.5)
project(test VERSION 0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

add_subdirectory(../deps/glfw ${PROJECT_BINARY_DIR}/deps/glfw)

set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_COLLADA_IMPORTER TRUE)
set(ASSIMP_BUILD_OBJ_IMPORTER TRUE)
set(ASSIMP_BUILD_FBX_IMPORTER TRUE)

add_subdirectory(../deps/assimp ${PROJECT_BINARY_DIR}/deps/assimp)
add_subdirectory(../deps/fmt ${PROJECT_BINARY_DIR}/deps/fmt)

set(GLAD_SOURCES
    ../deps/glad/src/glad.c
)
set(CATCH2_SOURCES
    ../deps/catch2/catch_amalgamated.cpp
)

set(DANDELION_PLATFORM_SOURCES
    ../src/platform/gl.cpp
    ../src/platform/shader.cpp
)
set(DANDELION_SCENE_SOURCES
    ../src/scene/scene.cpp
    ../src/scene/group.cpp
    ../src/scene/object.cpp
    ../src/scene/camera.cpp
    ../src/scene/light.cpp
)
set(DANDELION_UTILS_SOURCES
    ../src/utils/stb_image_wrapper.cpp
    # ../src/utils/ray.cpp
    # ../src/utils/aabb.cpp
    # ../src/utils/bvh.cpp
    ../src/utils/kinetic_state.cpp
    ../src/utils/logger.cpp
)
set(DANDELION_RENDER_SOURCES
    ../src/render/shader.cpp
    ../src/render/rasterizer.cpp
    ../src/render/rasterizer_renderer.cpp
    ../src/render/whitted_renderer.cpp
    ../src/render/render_engine.cpp
    ../src/render/triangle.cpp
)
set(DANDELION_GEOMETRY_SOURCES
    ../src/geometry/halfedge_mesh.cpp
    ../src/geometry/meshedit.cpp
    ../src/geometry/halfedge.cpp
    ../src/geometry/vertex.cpp
    ../src/geometry/edge.cpp
    ../src/geometry/face.cpp
)
set(DANDELION_SIMULATION_SOURCES
    ../src/simulation/solver.cpp
)
set(TEST_SOURCES
    basic_tests.cpp
    geometry_tests.cpp
)

set(SOURCES
    main.cpp
    ${DANDELION_PLATFORM_SOURCES}
    ${DANDELION_SCENE_SOURCES}
    ${DANDELION_UTILS_SOURCES}
    ${DANDELION_RENDER_SOURCES}
    ${DANDELION_GEOMETRY_SOURCES}
    ${DANDELION_SIMULATION_SOURCES}
    ${TEST_SOURCES}
    ${GLAD_SOURCES}
    ${CATCH2_SOURCES}
)

add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME}
    PRIVATE ../deps
    PRIVATE ../deps/glad/include
)
target_link_directories(${PROJECT_NAME} PRIVATE ../deps)
target_link_libraries(${PROJECT_NAME}
    glfw
    assimp
    fmt::fmt
    debug dandelion-ray-debug
    optimized dandelion-ray
    debug dandelion-bvh-debug
    optimized dandelion-bvh
)
target_compile_definitions(${PROJECT_NAME}
    PRIVATE $<$<CONFIG:Debug>:DEBUG>
    PRIVATE SPDLOG_FMT_EXTERNAL
    PRIVATE CATCH_AMALGAMATED_CUSTOM_MAIN
)

set(current_target ${PROJECT_NAME})
include(${CMAKE_CURRENT_SOURCE_DIR}/../CompilerFlags.cmake)
