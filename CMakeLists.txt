cmake_minimum_required(VERSION 3.25)
project(GVK LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ================ Dependencies ================

# --- SDL3 ---
add_subdirectory(${CMAKE_SOURCE_DIR}/include/SDL)

# --- GLM ----
add_subdirectory(${CMAKE_SOURCE_DIR}/include/glm)

# --- Vulkan and Vk-Bootstrap ---
find_package(Vulkan REQUIRED)
add_subdirectory(${CMAKE_SOURCE_DIR}/include/vk-bootstrap)

# --- IMGUI ---
set(IMGUI_PATH "${CMAKE_SOURCE_DIR}/include/imgui/")
file(GLOB IMGUI_SOURCES ${IMGUI_PATH}/*.cpp)
add_library(ImGui STATIC ${IMGUI_SOURCES})
target_include_directories(ImGui PUBLIC ${IMGUI_PATH})

# --- FMT ---
add_subdirectory(${CMAKE_SOURCE_DIR}/include/fmt)

# ================ Executable ================

add_executable(GVK
        main.cpp
)

target_link_libraries(GVK
    PRIVATE
        SDL3::SDL3
        Vulkan::Vulkan
        fmt
        vk-bootstrap::vk-bootstrap
        ImGui
        ${CMAKE_DL_LIBS}
)

target_include_directories(GVK
    PRIVATE
        ${Vulkan_INCLUDE_DIRS}
        ${IMGUI_PATH}
)