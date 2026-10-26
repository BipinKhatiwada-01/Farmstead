cmake_minimum_required(VERSION 3.20)

project(Farmstead LANGUAGES CXX)

find_package(SDL3 REQUIRED CONFIG)

add_executable(farmstead
    src/main.cpp
)

target_compile_features(farmstead PRIVATE cxx_std_20)

set_target_properties(farmstead PROPERTIES
    CXX_EXTENSIONS OFF
)

target_link_libraries(farmstead PRIVATE SDL3::SDL3)