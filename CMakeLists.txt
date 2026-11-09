
cmake_minimum_required(VERSION 3.20)

project(Farmstead LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(SDL3 REQUIRED CONFIG)

# External dependencies
include(FetchContent)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.12.0
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(nlohmann_json)

# Core game logic (independent of SDL)
add_library(
    farmstead-core
    src/game/game_model.cpp
    src/game/Crop.cpp
    src/game/Inventory.cpp
    src/game/Economy.cpp
    src/game/Progression.cpp
    src/persistence/SaveSystem.cpp
)

target_include_directories(
    farmstead-core
    PUBLIC
        include
)

target_link_libraries(
    farmstead-core
    PRIVATE
        nlohmann_json::nlohmann_json
)

target_compile_features(farmstead-core PUBLIC cxx_std_20)

set_target_properties(farmstead-core PROPERTIES
    CXX_EXTENSIONS OFF
)

# Game executable
add_executable(
    farmstead
    src/app/main.cpp
    src/render/renderer.cpp
    src/audio/audio_system.cpp
)

target_link_libraries(
    farmstead
    PRIVATE
        farmstead-core
        SDL3::SDL3
)

target_compile_features(farmstead PRIVATE cxx_std_20)

set_target_properties(farmstead PROPERTIES
    CXX_EXTENSIONS OFF
)

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

    # Helper for consistently configuring Catch2 test executables.
    function(add_farmstead_test target_name test_source)
        add_executable(${target_name} ${test_source})

        target_link_libraries(
            ${target_name}
            PRIVATE
                farmstead-core
                Catch2::Catch2WithMain
        )

        target_compile_features(${target_name} PRIVATE cxx_std_20)

        set_target_properties(${target_name} PROPERTIES
            CXX_EXTENSIONS OFF
        )

        catch_discover_tests(${target_name})
    endfunction()

    # Integration tests
    add_farmstead_test(
        farmstead-tests
        tests/integration/game_model_test.cpp
    )

    add_farmstead_test(
        economy-integration-tests
        tests/integration/economy_integration_test.cpp
    )

    # Unit tests
    add_farmstead_test(crop-tests tests/unit/crop_test.cpp)
    add_farmstead_test(economy-tests tests/unit/economy_test.cpp)
    add_farmstead_test(inventory-tests tests/unit/inventory_test.cpp)
    add_farmstead_test(progression-tests tests/unit/progression_test.cpp)
endif()
