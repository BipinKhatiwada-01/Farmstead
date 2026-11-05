#pragma once

// farmstead/game_model.hpp
//
// The game model: owns the tile map and the player's position, and enforces
// the movement rules. This header is the contract with the SDL layer.
//
// Conventions (everything below follows these):
//   * Positions are TILE coordinates (column, row), never pixels. Converting
//     to pixels (e.g. column * tileSize) is the renderer's job.
//   * Origin (0, 0) is the TOP-LEFT tile. Column grows to the right, row grows
//     DOWNWARD. So moving UP means row - 1.
//   * This file and game_model.cpp must never include SDL headers.
//
// Typical use from the SDL layer:
//   Input:  translate a key press into a Direction, then call tryMove().
//           Do not move the player or check collisions yourself.
//   Draw:   for row in [0, mapHeight()), for column in [0, mapWidth()):
//             draw a rectangle for tileAt(column, row), then draw the player
//             at playerPosition(). The model is only read while drawing.

#include "farmstead/Crop.hpp"
#include "farmstead/Economy.hpp"
#include "farmstead/Inventory.hpp"
#include "farmstead/Progression.hpp"
#include <array>
#include <cstddef>
#include <optional>

namespace farmstead {

/// What kind of square a tile is.
enum class TileType {
  GROUND,   ///< Walkable.
  OBSTACLE, ///< Blocks movement. Also what tileAt() reports off the map.
  MARKET,
};

/// A direction for a one-tile move. UP/DOWN change the row, LEFT/RIGHT the
/// column.
enum class Direction {
  UP,    ///< row - 1 (towards the top of the screen)
  DOWN,  ///< row + 1
  LEFT,  ///< column - 1
  RIGHT, ///< column + 1
};

/// A tile coordinate. Not pixels. Plain ints (not unsigned) so that
/// out-of-range values like -1 can be represented and rejected.
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

//
// GameModel manages the game world, including player movement,
// terrain interaction, planting, watering, and crop growth.
class GameModel {
public:
  static constexpr int kMapWidth = 20;
  static constexpr int kMapHeight = 15;

  // Builds the fixed map and puts the player on the start tile (1, 1), which
  /// is guaranteed to be ground.
  GameModel();

  int mapWidth() const;
  int mapHeight() const;

  /// The tile type at (column, row).
  ///
  /// Safe to call with any int: a coordinate OUTSIDE the map returns
  /// TileType::OBSTACLE (it never throws or reads invalid memory). A renderer
  /// should still only loop over 0..mapWidth()-1 and 0..mapHeight()-1; the
  /// off-map rule is a safety net, not something to draw.
  TileType tileAt(int column, int row) const;

  TilePosition playerPosition() const;

  TilePosition tileInFront() const;

  /// Tries to move the player exactly one tile in `direction`.
  ///
  /// Returns true if the player moved. Returns false if the target tile is off
  /// the map or is an OBSTACLE; in that case the player's position is
  /// UNCHANGED. Call it once per key press; held-key repeat handling belongs to
  /// the SDL layer, not here.
  bool tryMove(Direction direction);

  /// True if (column, row) is on the map, i.e. 0 <= column < mapWidth() and
  /// 0 <= row < mapHeight(). Safe to call with any int.
  bool isInBounds(int column, int row) const;

  Direction facingDirection() const;

  /// Tries to till the ground tile at (column, row) into farmable soil.
  ///
  /// Returns true if the tile was untilled GROUND and is now tilled.
  /// Returns false, changing nothing, if the tile is an OBSTACLE, is already
  /// tilled, or is off the map. Tilling does not change the terrain: tileAt()
  /// still reports GROUND and the tile stays walkable. The model does not check
  /// where the player is; the caller chooses the target (e.g. tileInFront()).
  bool tryTill(int column, int row);

  /// True if the tile at (column, row) has been tilled. False for untilled
  /// ground, for obstacles, and for anything off the map. Safe with any int.
  bool isTilled(int column, int row) const;

  /// Tries to plant a seed at (column, row). One crop type, unlimited seeds.
  ///
  /// Returns true if the tile was tilled and empty and now holds a crop.
  /// Returns false, changing nothing, if the tile is untilled ground, an
  /// OBSTACLE, already planted, or off the map. Planting does not change the
  /// terrain or un-till the soil: tileAt() still reports GROUND, isTilled()
  /// stays true, and the tile stays walkable. As with tilling, the caller
  /// chooses the target (e.g. tileInFront()).
  bool tryPlant(int column, int row);

  /// True if a crop has been planted at (column, row). False for empty soil,
  /// untilled ground, obstacles, and anything off the map. Safe with any int.
  bool isPlanted(int column, int row) const;

  // Attempts to water the crop at the given tile.
  // Returns false if the position is invalid, no crop exists,
  // or the crop has already been watered today.
  bool tryWater(int column, int row);

  // Returns whether the crop at the given tile has been watered today.
  // Returns false for invalid positions or empty tiles.
  bool isWateredToday(int column, int row) const;

  // Returns the current growth stage of the crop at the given tile.
  // Returns CropStage::NONE for invalid positions or empty tiles.
  CropStage cropStageAt(int column, int row) const;

  // Advances the game by one day.
  // Crops watered during the previous day gain growth, and all
  // crops have their daily watering status reset.
  void advanceDay();

  bool tryHarvest(int column, int row);

  // Inventory
  int produceCount() const;

  // Attempts to sell the requested quantity of produce.
  // Returns false if the player is not at the market,
  // the quantity is invalid, or the transaction cannot be completed.
  bool trySellProduce(int quantity);

  // Economy
  int coins() const;

  // Progression
  int coinsEarned() const;
  int objectiveTarget() const;
  bool objectiveCompleted() const;

  // market
  bool isFacingMarket() const;

  static constexpr TilePosition kMarketInteractionPosition{10, 4};

  static constexpr std::array<TilePosition, 6> kMarketFootprint{
      {{9, 3}, {10, 3}, {11, 3}, {9, 4}, {10, 4}, {11, 4}}};

private:
  // Flat index of a tile, row-major: row * kMapWidth + column.
  // Precondition: (column, row) is in bounds; callers check isInBounds first.
  std::size_t indexOf(int column, int row) const;

  // Private on purpose: other code must not edit the map or the player
  // position directly, or it could bypass the movement rules.
  std::array<TileType, kMapWidth * kMapHeight> tiles{};
  TilePosition player{};

  // what direction player is facing currently
  Direction facing;
  // Farming state, kept apart from terrain: same size and row-major indexing
  // as `tiles`. Only GROUND tiles are ever set to true.
  std::array<bool, kMapWidth * kMapHeight> tilled{};

  // Stores crops by tile index. An empty optional represents
  // a tile without a planted crop.
  std::array<std::optional<Crop>, kMapWidth * kMapHeight> crops{};

  // New gameplay systems
  Inventory inventory;
  Economy economy;
  Progression progression;
};

}; // namespace farmstead
