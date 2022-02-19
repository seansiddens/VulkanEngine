# Path variables need to be defined in .env.cmake
include(./.env.cmake OPTIONAL RESULT_VARIABLE LOCAL_ENV)
message(STATUS "Local .env.cmake: ${LOCAL_ENV}")

# C++ and CMake versions.
cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 17)

# Name of our executable and project versioning.
set(NAME VulkanEngine)
project(${NAME} VERSION 0.23.1)

# 1. Set VULKAN_SDK_PATH in .env.cmake to target specific vulkan version
if (DEFINED VULKAN_SDK_PATH)
    set(Vulkan_INCLUDE_DIRS "${VULKAN_SDK_PATH}/Include") # 1.1 Make sure this include path is correct
    set(Vulkan_LIBRARIES "${VULKAN_SDK_PATH}/Lib") # 1.2 Make sure lib path is correct
    set(Vulkan_FOUND "True")
else ()
    find_package(Vulkan REQUIRED) # throws error if could not find Vulkan
    message(STATUS "Found Vulkan: $ENV{VULKAN_SDK}")
endif ()
if (NOT Vulkan_FOUND)
    message(FATAL_ERROR "Could not find Vulkan library!")
else ()
    message(STATUS "Using vulkan lib at: ${Vulkan_LIBRARIES}")
endif ()

# 2. Set GLFW_PATH in .env.cmake to target specific glfw
if (DEFINED GLFW_PATH)
    message(STATUS "Using GLFW path specified in .env")
    set(GLFW_INCLUDE_DIRS "${GLFW_PATH}/include")
    if (MSVC)
        set(GLFW_LIB "${GLFW_PATH}/lib-vc2019") # 2.1 Update lib-vc2019 to use same version as your visual studio
    else ()
        message(STATUS "USING MINGW")
        set(GLFW_LIB "${GLFW_PATH}/lib-mingw-w64") # 2.1 make sure matches glfw mingw subdirectory
    endif ()
else ()
    set(glfw3_DIR ${GLFW_PATH})
    find_package(glfw3 3.3 REQUIRED)
    set(GLFW_LIB glfw)
    message(STATUS "Found GLFW")
endif ()
if (NOT GLFW_LIB)
    message(FATAL_ERROR "Could not find glfw library!")
else ()
    message(STATUS "Using glfw lib at: ${GLFW_LIB}")
endif ()

# If TINYOBJ_PATH not specified in .env.cmake, try fetching from local project folder
if (NOT TINYOBJ_PATH)
    message(STATUS "TINYOBJ_PATH not specified in .env.cmake, using external/tiny_obj_loader")
    set(TINYOBJ_PATH external/tiny_obj_loader/)
endif ()

# If STB_IMAGE_PATH not specified in .env.cmake, try fetching from local project folder
if (NOT STB_IMAGE_PATH)
    message(STATUS "STB_IMAGE_PATH not specified in .env.cmake, using external/stb_image")
    set(STB_IMAGE_PATH external/stb_image/)
endif ()

# Add Imgui library.
set(IMGUI_PATH external/imgui)
set(IMGUI_SOURCES
        ${IMGUI_PATH}/imgui.cpp
        ${IMGUI_PATH}/imgui_draw.cpp
        ${IMGUI_PATH}/imgui_demo.cpp
        ${IMGUI_PATH}/imgui_widgets.cpp
        ${IMGUI_PATH}/imgui_tables.cpp
        ${IMGUI_PATH}/imgui_impl_glfw.cpp
        ${IMGUI_PATH}/imgui_impl_vulkan.cpp)
#message(${IMGUI_SOURCES})
#add_library(imgui STATIC ${IMGUI_SOURCES})
#target_include_directories(imgui PUBLIC ${VULKAN_INCLUDE_DIRS} ${GLFW_INCLUDE_DIRS})

# Find all .cpp and  within the /src directory (including files within nested folders).
file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp)

# Add our executable which is built from all of the source files.
# TODO: Figure out how to add Imgui as a static library.
add_executable(${PROJECT_NAME} ${SOURCES} ${IMGUI_SOURCES})

# Use C++17 standard.
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)

set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/cmake-build-debug")

if (WIN32)
    message(STATUS "CREATING BUILD FOR WINDOWS")

    if (USE_MINGW)
        target_include_directories(${PROJECT_NAME} PUBLIC
                ${MINGW_PATH}/include
                )
        target_link_directories(${PROJECT_NAME} PUBLIC
                ${MINGW_PATH}/lib
                )
    endif ()

    target_include_directories(${PROJECT_NAME} PUBLIC
            ${PROJECT_SOURCE_DIR}/src
            ${Vulkan_INCLUDE_DIRS}
            ${TINYOBJ_PATH}
            ${GLFW_INCLUDE_DIRS}
            ${GLM_PATH}
            ${STB_IMAGE_PATH}
            ${IMGUI_PATH}
            )

    target_link_directories(${PROJECT_NAME} PUBLIC
            ${Vulkan_LIBRARIES}
            ${GLFW_LIB}
            )

    target_link_libraries(${PROJECT_NAME} glfw3 vulkan-1)
elseif (UNIX)
    message(STATUS "CREATING BUILD FOR UNIX")
    target_include_directories(${PROJECT_NAME} PUBLIC
            ${PROJECT_SOURCE_DIR}/src
            ${TINYOBJ_PATH}
            ${STB_IMAGE_PATH}
            ${IMGUI_PATH}
            )
    target_link_libraries(${PROJECT_NAME} glfw ${Vulkan_LIBRARIES})
endif ()
############## Build SHADERS #######################

# Find all vertex and fragment sources within shaders directory
# taken from VBlancos vulkan tutorial
# https://github.com/vblanco20-1/vulkan-guide/blob/all-chapters/CMakeLists.txt
find_program(GLSL_VALIDATOR glslangValidator HINTS
        ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE}
        /usr/bin
        /usr/local/bin
        ${VULKAN_SDK_PATH}/Bin
        ${VULKAN_SDK_PATH}/Bin32
        $ENV{VULKAN_SDK}/Bin/
        $ENV{VULKAN_SDK}/Bin32/
        )

# get all .vert and .frag files in shaders directory
file(GLOB_RECURSE GLSL_SOURCE_FILES
        "${PROJECT_SOURCE_DIR}/shaders/*.frag"
        "${PROJECT_SOURCE_DIR}/shaders/*.vert"
        )

foreach (GLSL ${GLSL_SOURCE_FILES})
    get_filename_component(FILE_NAME ${GLSL} NAME)
    set(SPIRV "${PROJECT_SOURCE_DIR}/shaders/${FILE_NAME}.spv")
    add_custom_command(
            OUTPUT ${SPIRV}
            COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
            DEPENDS ${GLSL})
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach (GLSL)

add_custom_target(
        Shaders
        DEPENDS ${SPIRV_BINARY_FILES}
)
