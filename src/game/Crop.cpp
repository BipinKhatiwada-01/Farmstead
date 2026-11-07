#include "farmstead/Crop.hpp"

namespace farmstead {

bool Crop::tryWater() {
  if (wateredToday) {
    return false;
  }
  wateredToday = true;
  return true;
}

void Crop::advanceDay() {
  if (wateredToday) {
    ++growthPoints;
  }
  wateredToday = false;
}

bool Crop::isWateredToday() const { return wateredToday; }

CropStage Crop::stage() const {
  if (growthPoints >= 6) {
    return CropStage::MATURE;
  }
  if (growthPoints >= 3) {
    return CropStage::SPROUT;
  }
  return CropStage::SEED;
}

int Crop::getGrowthPoints() const { return growthPoints; }

bool Crop::restoreState(int points, bool watered) {
  if (points < 0) {
    return false;
  }

  growthPoints = points;
  wateredToday = watered;
  return true;
}

}; // namespace farmstead
