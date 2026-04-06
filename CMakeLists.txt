cmake_minimum_required(VERSION 3.25)
project(GVK LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- SDL3 ---
add_subdirectory(${CMAKE_SOURCE_DIR}/include/SDL)

# --- GLM ----
add_subdirectory(${CMAKE_SOURCE_DIR}/include/glm)

# --- Vulkan ---
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/include/vulkan")
find_package(Vulkan REQUIRED)
include_directories(${Vulkan_INCLUDE_DIRS})

# --- IMGUI ---
set(IMGUI_PATH "${CMAKE_SOURCE_DIR}/include/imgui/")
file(GLOB IMGUI_SOURCES ${IMGUI_PATH}/*.cpp)
add_library(ImGui STATIC ${IMGUI_SOURCES})
target_include_directories(ImGui PUBLIC ${IMGUI_PATH})

add_executable(GVK
        main.cpp
)

target_link_libraries(GVK
    PRIVATE
        SDL3::SDL3
        Vulkan::Vulkan
        ImGui
)

target_include_directories(GVK
    PRIVATE
        ${Vulkan_INCLUDE_DIRS}
        ${IMGUI_PATH}
)