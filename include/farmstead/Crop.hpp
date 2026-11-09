#pragma once

namespace farmstead {

enum class CropStage {
  NONE,
  SEED,
  SPROUT,
  MATURE,
};

class Crop {
private:
  int growthPoints = 0;
  bool wateredToday = false;

public:
  // Attempts to water the crop. Returns false if already watered today.
  bool tryWater();

  // Awards growth if watered today and resets the daily watering status.
  void advanceDay();

  // Returns whether the crop has been watered during the current day.
  bool isWateredToday() const;

  // Returns the current growth stage based on accumulated growth points.
  CropStage stage() const;

  int getGrowthPoints() const;

  bool restoreState(int points, bool watered);
};

}; // namespace farmstead
