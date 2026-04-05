cmake_minimum_required(VERSION 3.25)
project(GVK)
set(CMAKE_CXX_STANDARD 20)

add_subdirectory(${CMAKE_SOURCE_DIR}/include/SDL)

add_executable(GVK
        main.cpp
)

target_link_libraries(GVK
    PRIVATE
    SDL3::SDL3
)