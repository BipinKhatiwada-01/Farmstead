#include "../include/farmstead/Crop.hpp"
#include "catch2/catch_test_macros.hpp"

#include <catch2/catch_all.hpp>

using namespace farmstead;

TEST_CASE("Initial state is seed and not watered") {
  Crop crop;

  REQUIRE(crop.stage() == CropStage::SEED);
  REQUIRE(!crop.isWateredToday());
}

TEST_CASE("Watering succedes once per day") {
  Crop crop;

  REQUIRE(crop.tryWater());
  REQUIRE(crop.isWateredToday());

  REQUIRE(!crop.tryWater());
}

TEST_CASE("Watering resets after advance day") {
  Crop crop;

  REQUIRE(crop.tryWater());
  crop.advanceDay();

  REQUIRE(!crop.isWateredToday());
  REQUIRE(crop.tryWater());
}

TEST_CASE("Becomes sprout after 3 growth points") {
  Crop crop;

  for (int i = 0; i < 3; ++i) {
    REQUIRE(crop.tryWater());
    crop.advanceDay();
  }

  REQUIRE(crop.stage() == CropStage::SPROUT);
}

TEST_CASE("Becomes mature after 6 growth points") {
  Crop crop;

  for (int i = 0; i < 6; ++i) {
    REQUIRE(crop.tryWater());
    crop.advanceDay();
  }

  REQUIRE(crop.stage() == CropStage::MATURE);
}

TEST_CASE("Missed days preserves progress") {
  Crop crop;

  for (int i = 0; i < 2; ++i) {
    REQUIRE(crop.tryWater());
    crop.advanceDay();
  }

  crop.advanceDay(); // Missed watering day.

  REQUIRE(crop.stage() == CropStage::SEED);

  REQUIRE(crop.tryWater());
  crop.advanceDay();

  REQUIRE(crop.stage() == CropStage::SPROUT);
}
