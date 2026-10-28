#pragma once

#include <array>
#include <cstddef>
namespace farmstead {

enum class TileType { GROUND, OBSTACLE };

enum class Direction { UP, DOWN, LEFT, RIGHT };

struct TilePosition {
  int column;
  int row;

  bool operator==(const TilePosition &obj) const {
    if (column == obj.column && row == obj.row) {
      return true;
    }
    return false;
  }
};

class GameModel {
public:
  static constexpr int kMapWidth = 20;
  static constexpr int kMapHeight = 15;

  GameModel();

  int mapWidth() const;
  int mapHeight() const;

  TileType tileAt(int column, int row) const;

  TilePosition playerPosition() const;

  // tries to move 1 tiles up, down, left, right
  // returns true if moved one tiles successfully
  // returns false if obstacles blocked the way and doesn't move
  bool tryMove(Direction direction);

  bool isInBounds(int column, int row) const;

private:
  std::size_t indexOf(int column, int row) const;

  // stores the map in 1d array
  std::array<TileType, kMapWidth * kMapHeight> tiles{};
  TilePosition player{};
};

} // namespace farmstead
