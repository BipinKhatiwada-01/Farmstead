#include "../include/farmstead/Crop.hpp"

#include <catch2/catch_all.hpp>

using namespace farmstead;

static void initialState_isSeedAndNotWatered() {
  Crop crop;

  CHECK(crop.stage() == CropStage::SEED);
  CHECK(!crop.isWateredToday());
}

static void watering_succeedsOncePerDay() {
  Crop crop;

  CHECK(crop.tryWater());
  CHECK(crop.isWateredToday());

  CHECK(!crop.tryWater());
}

static void watering_resetsAfterAdvanceDay() {
  Crop crop;

  CHECK(crop.tryWater());
  crop.advanceDay();

  CHECK(!crop.isWateredToday());
  CHECK(crop.tryWater());
}

static void growth_threePointsBecomeSprout() {
  Crop crop;

  for (int i = 0; i < 3; ++i) {
    CHECK(crop.tryWater());
    crop.advanceDay();
  }

  CHECK(crop.stage() == CropStage::SPROUT);
}

static void growth_sixPointsBecomeMature() {
  Crop crop;

  for (int i = 0; i < 6; ++i) {
    CHECK(crop.tryWater());
    crop.advanceDay();
  }

  CHECK(crop.stage() == CropStage::MATURE);
}

static void growth_missedDayPreservesProgress() {
  Crop crop;

  for (int i = 0; i < 2; ++i) {
    CHECK(crop.tryWater());
    crop.advanceDay();
  }

  crop.advanceDay(); // Missed watering day.

  CHECK(crop.stage() == CropStage::SEED);

  CHECK(crop.tryWater());
  crop.advanceDay();

  CHECK(crop.stage() == CropStage::SPROUT);
}

TEST_CASE("Crop behavior") {
#define RUN(test)                                                              \
  SECTION(#test) { test(); }
  RUN(initialState_isSeedAndNotWatered);
  RUN(watering_succeedsOncePerDay);
  RUN(watering_resetsAfterAdvanceDay);
  RUN(growth_threePointsBecomeSprout);
  RUN(growth_sixPointsBecomeMature);
  RUN(growth_missedDayPreservesProgress);
}
