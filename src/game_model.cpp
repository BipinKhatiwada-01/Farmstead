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
GameModel::GameModel() : player(kStartPosition) {
  // initialize facing direction to down
  facing = Direction::DOWN;

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

} // namespace farmstead
