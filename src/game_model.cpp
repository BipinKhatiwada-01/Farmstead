#include "../include/farmstead/game_model.hpp"
#include <string_view>

namespace farmstead {

namespace {
// '#' = obstacle, '.' = ground. One string per row, top row first:
// kLayout[row][column].
constexpr std::array<std::string_view, GameModel::kMapHeight> kLayout = {
    "....................", ".....###............", ".....#..............",
    ".....#.......##.....", "............##......", "....................",
    "..##................", "..##.........#......", "..............#.....",
    "....................", "......####..........", "..........#.........",
    "..........#.....##..", "....................", "....................",
};

constexpr TilePosition kStartPosition{1, 1};

// Every row must be exactly kMapWidth characters; checked at compile time
// so a typo in the layout fails the build instead of misbehaving at runtime.
constexpr bool layoutIsValid() {
  for (std::string_view row : kLayout) {
    if (static_cast<int>(row.size()) != GameModel::kMapWidth)
      return false;
  }
  return true;
}
static_assert(layoutIsValid(), "every layout row must be kMapWidth characters");

} // namespace

// constructor
GameModel::GameModel() : player(kStartPosition), facing(Direction::DOWN) {

  for (int row = 0; row < kMapHeight; row++) {
    for (int col = 0; col < kMapWidth; col++) {
      const char symbol = kLayout[row][col];
      if (symbol == '#') {
        tiles[indexOf(col, row)] = TileType::OBSTACLE;
      } else {
        tiles[indexOf(col, row)] = TileType::GROUND;
      }
    }
  }
}

std::size_t GameModel::indexOf(int column, int row) const {
  return static_cast<std::size_t>(row * kMapWidth + column);
}

int GameModel::mapWidth() const { return kMapWidth; }

int GameModel::mapHeight() const { return kMapHeight; }

Direction GameModel::facingDirection() const { return facing; }

bool GameModel::isInBounds(int column, int row) const {
  bool isInBound =
      column >= 0 && column < kMapWidth && row >= 0 && row < kMapHeight;
  return isInBound ? true : false;
}

TileType GameModel::tileAt(int column, int row) const {
  if (!isInBounds(column, row)) {
    return TileType::OBSTACLE;
  }
  return tiles[indexOf(column, row)];
}

TilePosition GameModel::playerPosition() const { return player; }

TilePosition GameModel::tileInFront() const {
  TilePosition facingTile = player;
  switch (facing) {
  case Direction::UP:
    facingTile.row--;
    break;
  case Direction::DOWN:
    facingTile.row++;
    break;
  case Direction::LEFT:
    facingTile.column--;
    break;
  case Direction::RIGHT:
    facingTile.column++;
    break;
  }

  return facingTile;
}

// Bounds are checked before reading the tile, and the position only changes
// on the last line, so a rejected move leaves the player where it was.
bool GameModel::tryMove(Direction direction) {
  int deltaColumn = 0;
  int deltaRow = 0;
  switch (direction) {
  case farmstead::Direction::UP:
    deltaRow = -1;
    break;
  case farmstead::Direction::DOWN:
    deltaRow = 1;
    break;
  case farmstead::Direction::LEFT:
    deltaColumn = -1;
    break;
  case farmstead::Direction::RIGHT:
    deltaColumn = 1;
    break;
  }

  if (deltaColumn == 0 && deltaRow == 0) {
    return false; // not a valid Direction
  }

  // assign facing direction
  facing = direction;

  const int targetColumn = player.column + deltaColumn;
  const int targetRow = player.row + deltaRow;

  // outside the map
  if (!isInBounds(targetColumn, targetRow))
    return false;
  // obstacle
  if (tileAt(targetColumn, targetRow) == TileType::OBSTACLE)
    return false;

  player = {targetColumn, targetRow};
  return true;
}

bool GameModel::tryTill(int column, int row) {
  if (!isInBounds(column, row)) {
    return false;
  }
  const std::size_t index = indexOf(column, row);
  if (tiles[index] != TileType::GROUND) {
    return false;
  }
  if (tilled[index]) {
    return false;
  }
  tilled[index] = true;
  return true;
}

bool GameModel::isTilled(int column, int row) const {
  if (!isInBounds(column, row)) {
    return false;
  }
  return tilled[indexOf(column, row)];
}

bool GameModel::tryPlant(int column, int row) {
  if (!isInBounds(column, row)) {
    return false;
  }
  const std::size_t index = indexOf(column, row);
  if (!tilled[index]) {
    return false;
  }
  if (crops[index].has_value()) {
    return false;
  }

  // create crop object to indicate its planted
  crops[index].emplace();
  return true;
}

bool GameModel::isPlanted(int column, int row) const {
  if (!isInBounds(column, row)) {
    return false;
  }
  return crops[indexOf(column, row)].has_value();
}

bool GameModel::tryWater(int column, int row) {
  if (!isInBounds(column, row)) {
    return false;
  }

  int index = indexOf(column, row);

  if (!crops[index].has_value()) {
    return false;
  }

  return crops[index]->tryWater();
}

bool GameModel::isWateredToday(int column, int row) const {
  if (!isInBounds(column, row)) {
    return false;
  }

  int index = indexOf(column, row);

  if (!crops[index].has_value()) {
    return false;
  }

  return crops[index]->isWateredToday();
}

CropStage GameModel::cropStageAt(int column, int row) const {
  if (!isInBounds(column, row)) {
    return CropStage::NONE;
  }

  int index = indexOf(column, row);

  if (!crops[index].has_value()) {
    return CropStage::NONE;
  }

  return crops[index]->stage();
}

void GameModel::advanceDay() {
  for (auto &crop : crops) {
    if (crop.has_value()) {
      crop->advanceDay();
    }
  }
}

bool GameModel::tryHarvest(int column, int row) {
  if (!isInBounds(column, row)) {
    return false;
  }

  int index = indexOf(column, row);

  if (!crops[index].has_value()) {
    return false;
  }

  if (crops[index]->stage() != CropStage::MATURE) {
    return false;
  }

  if (!inventory.addProduce(1)) {
    return false;
  }

  crops[index].reset();
  return true;
}

int GameModel::produceCount() const { return inventory.produceCount(); }

int GameModel::coins() const { return economy.coins(); }

int GameModel::coinsEarned() const { return progression.coinsEarned(); }

int GameModel::objectiveTarget() const { return progression.objectiveTarget(); }

bool GameModel::objectiveCompleted() const {
  return progression.objectiveCompleted();
}

bool GameModel::trySellProduce(int quantity) {

  if (inventory.produceCount() < quantity || quantity <= 0) {
    return false;
  }

  const int saleValue = economy.calculateSaleValue(quantity);

  if (saleValue <= 0) {
    return false;
  }

  if (!economy.addCoins(saleValue)) {
    return false;
  }

  inventory.removeProduce(quantity);
  progression.onCoinsEarned(saleValue);

  return true;
}

} // namespace farmstead
