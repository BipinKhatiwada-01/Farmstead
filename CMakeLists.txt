cmake_minimum_required(VERSION 3.20)

project(Farmstead LANGUAGES CXX)

find_package(SDL3 REQUIRED CONFIG)

add_library(
  farmstead-core
  src/game_model.cpp
  src/Crop.cpp
  src/Inventory.cpp
  src/Economy.cpp
  src/Progression.cpp
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

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set_target_properties(farmstead-core PROPERTIES
    CXX_EXTENSIONS OFF
)

target_link_libraries(farmstead PRIVATE SDL3::SDL3)

# Testing
include(CTest)

if(BUILD_TESTING)
  include(FetchContent)
  FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.8.1
    GIT_SHALLOW TRUE
  )
  FetchContent_MakeAvailable(Catch2)
  list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
  include(Catch)

  # GameModel tests
  add_executable(
    farmstead-tests
    tests/game_model_test.cpp
  )

  target_link_libraries(
    farmstead-tests
    PRIVATE
    farmstead-core
    Catch2::Catch2WithMain
  )

  target_compile_features(farmstead-tests PRIVATE cxx_std_20)

  set_target_properties(farmstead-tests PROPERTIES
    CXX_EXTENSIONS OFF
  )

  catch_discover_tests(farmstead-tests)

 # Crop tests
  add_executable(
    crop-tests
    tests/crop_test.cpp
  )

  target_link_libraries(
    crop-tests
    PRIVATE farmstead-core Catch2::Catch2WithMain
  )

  target_compile_features(crop-tests PRIVATE cxx_std_20)

  set_target_properties(crop-tests PROPERTIES
    CXX_EXTENSIONS OFF
  )

  catch_discover_tests(crop-tests)


  # Inventory tests
  add_executable(
    inventory-tests
    tests/inventory_test.cpp
  )

  target_link_libraries(
    inventory-tests
    PRIVATE farmstead-core Catch2::Catch2WithMain
  )

  target_compile_features(inventory-tests PRIVATE cxx_std_20)

  set_target_properties(inventory-tests PROPERTIES
    CXX_EXTENSIONS OFF
  )

  catch_discover_tests(inventory-tests)


  # Economy tests
  add_executable(
    economy-tests
    tests/economy_test.cpp
  )

  target_link_libraries(
    economy-tests
    PRIVATE farmstead-core Catch2::Catch2WithMain
  )

  target_compile_features(economy-tests PRIVATE cxx_std_20)

  set_target_properties(economy-tests PROPERTIES
    CXX_EXTENSIONS OFF
  )

  catch_discover_tests(economy-tests)

  # Progression tests
  add_executable(
    progression-tests
    tests/progression_test.cpp
  )

  target_link_libraries(
    progression-tests
    PRIVATE farmstead-core Catch2::Catch2WithMain
  )

  target_compile_features(progression-tests PRIVATE cxx_std_20)

  set_target_properties(progression-tests PROPERTIES
    CXX_EXTENSIONS OFF
  )

  catch_discover_tests(progression-tests)

endif()
