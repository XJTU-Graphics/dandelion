# This file contains Platform-specific flags for compilers and linkers

# Apple macOS has platform-specific libraries (frameworks) which need to be linked
if (APPLE)
    find_package(OpenGL REQUIRED)
    target_include_directories(${current_target} PRIVATE ${OPENGL_INCLUDE_DIR})
    target_link_libraries(${current_target}
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
    # Disable C4996 for std::fopen
    # Disable C4458 because we usually declare local variables with the same name
    # as member variables just as short for the members.
    target_compile_options(${current_target}
        PRIVATE /W4 /WX /utf-8 /wd4996 /wd4458
    )
    # LNK4099 is missing pdb
    # We don't provide pdb files for static libraries, so disable this warning
    target_link_options(${current_target}
        PRIVATE /WX /IGNORE:4099
    )
    target_compile_definitions(${current_target}
        PRIVATE NOMINMAX
    )
else()
    target_compile_options(${current_target}
        PRIVATE -Wall -Wextra -Werror
    )
endif()
# ignore dangling reference warning with assimp and fmt with GCC 13 and onward
# ignore maybe uninitialized warning from copy constructor of Eigen::Matrix with GCC 11 and onward (only occur in release)
if(${CMAKE_C_COMPILER_ID} STREQUAL "GNU" AND ${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER_EQUAL 11)
    if (${current_target} STREQUAL ${PROJECT_NAME})
        target_compile_options(assimp
            PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-dangling-reference -Wno-maybe-uninitialized>
        )
    endif()
    target_compile_options(${current_target}
        PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-maybe-uninitialized>
    )
endif()
# workaround for MINGW which will produce relocation error due to high image base
# the specific reason is not known
if (MINGW)
    target_link_options(${current_target}
        PRIVATE -Wl,--default-image-base-low
    )
endif ()
