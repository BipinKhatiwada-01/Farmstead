#pragma once

#include "farmstead/game_model.hpp"
#include <vector>

namespace farmstead {

struct CropSaveData {
  TilePosition position;
  int growthPoints;
  bool wateredToday;
  CropStage stage;
};

struct GameSaveData {
  int day;

  TilePosition playerPosition;
  Direction facing;

  std::vector<TilePosition> tilledTiles;
  std::vector<CropSaveData> crops;

  int inventoryProduce;
  int coins;
  int totalCoinsEarned;
  int progression;
};
} // namespace farmstead
