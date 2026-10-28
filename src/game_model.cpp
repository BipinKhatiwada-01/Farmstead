#include "farmstead/game_model.hpp"
#include <string_view>

namespace farmstead {

namespace {
// '#' = obstacle, '.' = ground
constexpr std::array<std::string_view, GameModel::kMapHeight> kLayout = {
    "....................", ".....###............", ".....#..............",
    ".....#.......##.....", "............##......", "....................",
    "..##................", "..##.........#......", "..............#.....",
    "....................", "......####..........", "..........#.........",
    "..........#.....##..", "....................", "....................",
};

constexpr TilePosition kStartPosition{1, 1};

// ============================================================================
// Map Layout Integrity Validator (Compile-Time)
// ============================================================================
// Enforces that every horizontal row in the `kLayout` matrix strictly matches
// `GameModel::kMapWidth`. Because this runs via static_assert, it prevents
// out-of-bounds memory bugs/crashes at compile-time with zero runtime overhead.
//
// NOTE: If you add or modify a map layer in kLayout, make sure the string
// length exactly matches kMapWidth, otherwise the build will fail.
// ============================================================================
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
GameModel::GameModel() : player(kStartPosition) {
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

} // namespace farmstead
