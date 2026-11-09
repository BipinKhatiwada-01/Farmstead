#pragma once

namespace farmstead {

enum class Direction {
  UP,
  DOWN,
  LEFT,
  RIGHT,
};

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

} // namespace farmstead
