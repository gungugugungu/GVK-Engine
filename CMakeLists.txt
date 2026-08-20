cmake_minimum_required(VERSION 3.25)
project(GVK LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_language(C)

# ================ Dependencies ================

# --- SDL3 ---
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/include/SDL)

# --- GLM ----
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/include/glm)

# --- Vulkan and Vk-Bootstrap ---
include(FetchContent)
FetchContent_Declare(
        VulkanHeaders
        GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
        GIT_TAG v1.4.357
)
FetchContent_MakeAvailable(VulkanHeaders)

find_package(Vulkan REQUIRED)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/include/vk-bootstrap)
target_include_directories(vk-bootstrap PUBLIC ${vulkanheaders_SOURCE_DIR}/include)

# --- IMGUI ---
set(IMGUI_PATH "${CMAKE_CURRENT_SOURCE_DIR}/include/imgui/")
set(IMGUI_BACKENDS_PATH "${IMGUI_PATH}/backends")

file(GLOB IMGUI_SOURCES ${IMGUI_PATH}/*.cpp)

add_library(ImGui STATIC
        ${IMGUI_SOURCES}
        ${IMGUI_BACKENDS_PATH}/imgui_impl_vulkan.cpp
        ${IMGUI_BACKENDS_PATH}/imgui_impl_sdl3.cpp
)

target_include_directories(ImGui PUBLIC
        ${IMGUI_PATH}
        ${IMGUI_BACKENDS_PATH}
)

target_link_libraries(ImGui PUBLIC
        Vulkan::Vulkan
        SDL3::SDL3
)

# --- FMT ---
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/include/fmt)

# --- TINYGLTF ---
set(TINYGLTF_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/tinygltf")

add_library(tinygltf STATIC
        ${TINYGLTF_DIR}/tiny_gltf_v3.c
)

target_include_directories(tinygltf PUBLIC
        ${TINYGLTF_DIR}
)

target_include_directories(tinygltf PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/stb/)
target_compile_definitions(tinygltf PUBLIC
        TINYGLTF3_ENABLE_FS
        TINYGLTF3_ENABLE_STB_IMAGE
)

set_target_properties(tinygltf PROPERTIES
        C_STANDARD 11
        C_STANDARD_REQUIRED ON
        C_EXTENSIONS OFF
)

# --- GVK LIBRARY ---

add_library(gvk INTERFACE)
target_compile_definitions(gvk INTERFACE VK_ENABLE_BETA_EXTENSIONS)
add_library(gvk::gvk ALIAS gvk)

target_include_directories(gvk INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include/glm
        ${Vulkan_INCLUDE_DIRS}
        ${IMGUI_PATH}
        ${IMGUI_BACKENDS_PATH}
        ${TINYGLTF_DIR}
)

target_link_libraries(gvk INTERFACE
        SDL3::SDL3
        Vulkan::Vulkan
        fmt
        tinygltf
        ImGui
        vk-bootstrap::vk-bootstrap
        ${CMAKE_DL_LIBS}
)

if (PROJECT_IS_TOP_LEVEL)
    add_executable(GVK main.cpp)
    target_link_libraries(GVK PRIVATE gvk::gvk)
endif()