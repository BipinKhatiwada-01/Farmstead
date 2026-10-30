cmake_minimum_required(VERSION 3.20)

project(Farmstead LANGUAGES CXX)

find_package(SDL3 REQUIRED CONFIG)

add_library(
  farmstead-core
  src/game_model.cpp
  src/Crop.cpp
)

target_include_directories(
  farmstead-core
  PUBLIC
  include
)

add_executable(
  farmstead
  src/main.cpp
)

target_link_libraries(
  farmstead
  PRIVATE
  farmstead-core
)

target_compile_features(farmstead-core PUBLIC cxx_std_20)

set_target_properties(farmstead-core PROPERTIES
    CXX_EXTENSIONS OFF
)

target_link_libraries(farmstead PRIVATE SDL3::SDL3)

# Testing
include(CTest)

if(BUILD_TESTING)

  # GameModel tests
  add_executable(
    farmstead-tests
    tests/game_model_test.cpp
  )

  target_link_libraries(
    farmstead-tests
    PRIVATE
    farmstead-core
  )

  target_compile_features(farmstead-tests PRIVATE cxx_std_20)

  set_target_properties(farmstead-tests PROPERTIES
    CXX_EXTENSIONS OFF
  )

  add_test(
    NAME game-model-tests
    COMMAND farmstead-tests
  )

 # Crop tests
  add_executable(
    crop-tests
    tests/crop_test.cpp
  )

  target_link_libraries(
    crop-tests
    PRIVATE farmstead-core
  )

  target_compile_features(crop-tests PRIVATE cxx_std_20)

  set_target_properties(crop-tests PROPERTIES
    CXX_EXTENSIONS OFF
  )

  add_test(
    NAME crop-tests
    COMMAND crop-tests
  )

endif()
