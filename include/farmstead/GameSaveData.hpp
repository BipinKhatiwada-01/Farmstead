#pragma once

#include "farmstead/game_model.hpp"
#include <vector>

namespace farmstead {

struct CropSaveData {
  TilePosition position;
  int growthPoints;
  bool wateredToday;
};

struct GameSaveData {
  int day;

  TilePosition playerPosition;
  Direction facing;

  std::vector<TilePosition> tilledTiles;
  std::vector<CropSaveData> crops;

  int produce;
  int coins;
  int totalCoinsEarned;
};
} // namespace farmstead
