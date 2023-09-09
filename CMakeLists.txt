cmake_minimum_required(VERSION 3.5)
project(dandelion VERSION 0.4)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release")
if (CMAKE_BUILD_TYPE MATCHES "(Debug)|(Release)")
    message("Current build type: ${CMAKE_BUILD_TYPE}")
else()
    message("Set default buile type to Debug.")
    set(CMAKE_BUILD_TYPE "Debug")
endif()
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

add_subdirectory(deps/glfw)

set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_COLLADA_IMPORTER TRUE)
set(ASSIMP_BUILD_OBJ_IMPORTER TRUE)
set(ASSIMP_BUILD_FBX_IMPORTER TRUE)
add_subdirectory(deps/assimp)

set(GLAD_SOURCES
    deps/glad/src/glad.c
)
set(IMGUI_SOURCES
    deps/imgui/imgui.cpp
    deps/imgui/imgui_demo.cpp
    deps/imgui/imgui_draw.cpp
    deps/imgui/imgui_tables.cpp
    deps/imgui/imgui_widgets.cpp
    deps/imgui/imgui_impl_glfw.cpp
    deps/imgui/imgui_impl_opengl3.cpp
)

set(DANDELION_PLATFORM_SOURCES
    src/platform/platform.cpp
    src/platform/gl.cpp
    src/platform/shader.cpp
)
set(DANDELION_UI_SOURCES
    src/ui/controller.cpp
    src/ui/menubar.cpp
    src/ui/toolbar.cpp
)
set(DANDELION_SCENE_SOURCES
    src/scene/scene.cpp
    src/scene/group.cpp
    src/scene/object.cpp
    src/scene/camera.cpp
    src/scene/light.cpp
)
set(DANDELION_UTILS_SOURCES
    src/utils/stb_image_wrapper.cpp
    # src/utils/ray.cpp
    # src/utils/aabb.cpp
    # src/utils/bvh.cpp
    src/utils/kinetic_state.cpp
    src/utils/logger.cpp
)
set(DANDELION_RENDER_SOURCES
    src/render/shader.cpp
    src/render/rasterizer.cpp
    src/render/rasterizer_mt.cpp
    src/render/rasterizer_renderer.cpp
    src/render/rasterizer_renderer_mt.cpp
    src/render/whitted_renderer.cpp
    src/render/render_engine.cpp
    src/render/triangle.cpp
)
set(DANDELION_GEOMETRY_SOURCES
    src/geometry/halfedge_mesh.cpp
    src/geometry/meshedit.cpp
    src/geometry/halfedge.cpp
    src/geometry/vertex.cpp
    src/geometry/edge.cpp
    src/geometry/face.cpp
)
set(DANDELION_SIMULATION_SOURCES
    src/simulation/solver.cpp
)

set(SOURCES
    src/main.cpp
    ${DANDELION_PLATFORM_SOURCES}
    ${DANDELION_UI_SOURCES}
    ${DANDELION_SCENE_SOURCES}
    ${DANDELION_UTILS_SOURCES}
    ${DANDELION_RENDER_SOURCES}
    ${DANDELION_GEOMETRY_SOURCES}
    ${DANDELION_SIMULATION_SOURCES}
    ${GLAD_SOURCES}
    ${IMGUI_SOURCES}
)

add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME}
    PRIVATE deps
    PRIVATE deps/glad/include
)
target_link_directories(${PROJECT_NAME} PRIVATE deps)
target_link_libraries(${PROJECT_NAME}
    glfw
    assimp
    debug dandelion-ray-debug
    optimized dandelion-ray
    debug dandelion-bvh-debug
    optimized dandelion-bvh
)
target_compile_definitions(${PROJECT_NAME}
    PRIVATE $<$<CONFIG:Debug>:DEBUG>
    PRIVATE SPDLOG_FMT_EXTERNAL
    PRIVATE FMT_HEADER_ONLY
)
# Apple macOS has platform-specific libraries (frameworks) which need to be linked
if (APPLE)
    find_package(OpenGL REQUIRED)
    target_include_directories(${PROJECT_NAME} PRIVATE ${OPENGL_INCLUDE_DIR})
    target_link_libraries(${PROJECT_NAME}
        "-framework Cocoa"
        "-framework OpenGL"
        "-framework IOKit"
        ${OPENGL_gl_LIBRARY}
    )
endif()

# Disable the deprecated-non-prototype warning for zlib
if (${CMAKE_C_COMPILER_ID} MATCHES "Clang")
    target_compile_options(zlibstatic
        PRIVATE $<$<COMPILE_LANGUAGE:C>:-Wno-deprecated-non-prototype>
    )
endif()
if (MSVC)
    # Disable C4127 for Eigen
    # Disable C4996 for std::fopen
    # Disable C4458 because we usually declare local variables with the same name
    # as member variables just as short for the members.
    target_compile_options(${PROJECT_NAME}
        PRIVATE /W4 /utf-8 /wd4127 /wd4996 /wd4458
    )
    target_compile_definitions(${PROJECT_NAME}
        PRIVATE NOMINMAX
    )
else()
    target_compile_options(${PROJECT_NAME}
        PRIVATE -Wall -Wextra -Werror
    )
endif()

file(COPY ${PROJECT_SOURCE_DIR}/resources DESTINATION ${PROJECT_BINARY_DIR})
