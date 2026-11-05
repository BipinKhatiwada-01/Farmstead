// tests/game_model_checks.cpp
//
// Behaviour checks for farmstead::GameModel. No SDL dependency.
//
// These checks are written against the CURRENT map in game_model.cpp.
// Coordinates are (column, row); row 0 is the TOP; P marks the start (1,1).
//
//        0         1
//        01234567890123456789
//    0   ....................
//    1   .P...###............
//    2   .....#..............
//    3   .....#.......##.....
//    4   ............##......
//    5   ....................
//    6   ..##................
//    7   ..##.........#......
//    8   ..............#.....
//    9   ....................
//   10   ......####..........
//   11   ..........#.........
//   12   ..........#.....##..
//   13   ....................
//   14   ....................
//
// Facts the tests rely on (if you edit the map, update these tests):
//   - 23 obstacles, listed in tileAt_wallTilesAreObstacles
//   - the border (row 0, row 14, column 0, column 19) is all ground
//   - column 1, row 5, row 9 and row 0 are all ground (used as walking routes)
//   - (14,8) is an isolated obstacle: all four neighbours are ground

#include "farmstead/game_model.hpp"
#include "catch2/catch_test_macros.hpp"

#include <catch2/catch_all.hpp>

#include <cstdint>
#include <cstdlib>

using namespace farmstead;

// ---------------------------------------------------------------------------
// Test helpers
// ---------------------------------------------------------------------------

#define REQUIRE_POS(model, expectedColumn, expectedRow)                        \
  do {                                                                         \
    REQUIRE((model).playerPosition().column == (expectedColumn));              \
    REQUIRE((model).playerPosition().row == (expectedRow));                    \
  } while (false)

#define REQUIRE_TILE(model, column, row, expected)                             \
  REQUIRE((model).tileAt((column), (row)) == (expected))

// Moves exactly `steps` times. Every step MUST succeed; if one is blocked the
// failure is reported at the caller's line, with the position where it stopped.
static void moveExactlyImpl(GameModel &m, Direction d, int steps) {
  for (int i = 0; i < steps; ++i) {
    INFO("step " << i + 1 << " of " << steps);
    REQUIRE(m.tryMove(d));
  }
}
#define MOVE(model, dir, steps) moveExactlyImpl((model), (dir), (steps))

// ---------------------------------------------------------------------------
// Map dimensions
// ---------------------------------------------------------------------------

TEST_CASE("mapDimensionsAre20By15") {
  GameModel m;
  REQUIRE(m.mapWidth() == 20);
  REQUIRE(m.mapHeight() == 15);
  REQUIRE(m.mapWidth() == GameModel::kMapWidth);
  REQUIRE(m.mapHeight() == GameModel::kMapHeight);
}

// ---------------------------------------------------------------------------
// isInBounds
// ---------------------------------------------------------------------------

TEST_CASE("isInBounds_cornersAreInside") {
  GameModel m;
  REQUIRE(m.isInBounds(0, 0));
  REQUIRE(m.isInBounds(19, 0));
  REQUIRE(m.isInBounds(0, 14));
  REQUIRE(m.isInBounds(19, 14));
}

TEST_CASE("isInBounds_negativeCoordinatesAreOutside") {
  GameModel m;
  REQUIRE(!m.isInBounds(-1, 0));
  REQUIRE(!m.isInBounds(0, -1));
  REQUIRE(!m.isInBounds(-1, -1));
  REQUIRE(!m.isInBounds(-100, 5));
}

TEST_CASE("isInBounds_pastTheFarEdgeIsOutside") {
  GameModel m;
  REQUIRE(!m.isInBounds(20, 0));
  REQUIRE(!m.isInBounds(0, 15));
  REQUIRE(!m.isInBounds(20, 15));
  REQUIRE(!m.isInBounds(100, 5));
}

// ---------------------------------------------------------------------------
// tileAt
// ---------------------------------------------------------------------------

TEST_CASE("tileAt_openGroundIsGround") {
  GameModel m;
  REQUIRE_TILE(m, 0, 0, TileType::GROUND);
  REQUIRE_TILE(m, 1, 1, TileType::GROUND);
  REQUIRE_TILE(m, 10, 5, TileType::GROUND);
  REQUIRE_TILE(m, 19, 14, TileType::GROUND);
}

TEST_CASE("tileAt_marketFootprintContainsSixMarketTiles") {
  GameModel m;
  REQUIRE(GameModel::kMarketFootprint.size() == 6);
  for (const TilePosition &position : GameModel::kMarketFootprint) {
    INFO("market footprint tile (" << position.column << ", " << position.row
                                   << ")");
    REQUIRE_TILE(m, position.column, position.row, TileType::MARKET);
  }
  REQUIRE_TILE(m, 10, 5, TileType::GROUND);
  REQUIRE_TILE(m, 8, 3, TileType::GROUND);
  REQUIRE_TILE(m, 13, 3, TileType::OBSTACLE);
}

TEST_CASE("tileAt_wallTilesAreObstacles") {
  GameModel m;
  const TilePosition walls[] = {
      {5, 1},  {6, 1},   {7, 1},   {5, 2},   {5, 3},   {13, 3},
      {14, 3}, {12, 4},  {13, 4},  {2, 6},   {3, 6},   {2, 7},
      {3, 7},  {13, 7},  {14, 8},  {6, 10},  {7, 10},  {8, 10},
      {9, 10}, {10, 11}, {10, 12}, {16, 12}, {17, 12},
  };
  for (const TilePosition &w : walls) {
    REQUIRE_TILE(m, w.column, w.row, TileType::OBSTACLE);
  }
}

TEST_CASE("tileAt_tilesBesideWallsAreGround") {
  GameModel m;
  REQUIRE_TILE(m, 4, 1, TileType::GROUND);  // left of the row-1 wall
  REQUIRE_TILE(m, 8, 1, TileType::GROUND);  // right of the row-1 wall
  REQUIRE_TILE(m, 6, 2, TileType::GROUND);  // inside the "C" shape
  REQUIRE_TILE(m, 14, 7, TileType::GROUND); // above isolated wall (14,8)
  REQUIRE_TILE(m, 14, 9, TileType::GROUND); // below it
  REQUIRE_TILE(m, 13, 8, TileType::GROUND); // left of it
  REQUIRE_TILE(m, 15, 8, TileType::GROUND); // right of it
}

// Together with the wall list above, this proves there are no EXTRA obstacles.
TEST_CASE("tileAt_totalObstacleCountIs23") {
  GameModel m;
  int obstacles = 0;
  for (int row = 0; row < m.mapHeight(); ++row) {
    for (int column = 0; column < m.mapWidth(); ++column) {
      if (m.tileAt(column, row) == TileType::OBSTACLE) {
        ++obstacles;
      }
    }
  }
  REQUIRE(obstacles == 23);
}

TEST_CASE("tileAt_outOfBoundsIsObstacle") {
  GameModel m;
  REQUIRE_TILE(m, -1, 0, TileType::OBSTACLE);
  REQUIRE_TILE(m, 0, -1, TileType::OBSTACLE);
  REQUIRE_TILE(m, -1, -1, TileType::OBSTACLE);
  REQUIRE_TILE(m, GameModel::kMapWidth, 0, TileType::OBSTACLE);
  REQUIRE_TILE(m, 0, GameModel::kMapHeight, TileType::OBSTACLE);
  REQUIRE_TILE(m, GameModel::kMapWidth, GameModel::kMapHeight,
               TileType::OBSTACLE);
}

// The edge tests below rely on the border being walkable, otherwise the
// "map edge" rule could never be reached. This documents that on purpose.
TEST_CASE("tileAt_borderIsAllGround") {
  GameModel m;
  for (int column = 0; column < GameModel::kMapWidth; ++column) {
    REQUIRE_TILE(m, column, 0, TileType::GROUND);
    REQUIRE_TILE(m, column, GameModel::kMapHeight - 1, TileType::GROUND);
  }
  for (int row = 0; row < GameModel::kMapHeight; ++row) {
    REQUIRE_TILE(m, 0, row, TileType::GROUND);
    REQUIRE_TILE(m, GameModel::kMapWidth - 1, row, TileType::GROUND);
  }
}

// ---------------------------------------------------------------------------
// Start state
// ---------------------------------------------------------------------------

TEST_CASE("start_positionIsColumn1Row1") {
  GameModel m;
  REQUIRE_POS(m, 1, 1);
}

TEST_CASE("start_isInBoundsAndOnGround") {
  GameModel m;
  const TilePosition p = m.playerPosition();
  REQUIRE(m.isInBounds(p.column, p.row));
  REQUIRE_TILE(m, p.column, p.row, TileType::GROUND);
}

TEST_CASE("start_eachModelHasItsOwnState") {
  GameModel a;
  GameModel b;
  REQUIRE(a.tryMove(Direction::RIGHT));
  REQUIRE_POS(a, 2, 1);
  REQUIRE_POS(b, 1, 1); // b must not be affected by a
}

// ---------------------------------------------------------------------------
// tryMove: direction meaning (one test per direction, so a swapped axis is
// reported against exactly the direction that is wrong)
// ---------------------------------------------------------------------------

TEST_CASE("move_upDecreasesRowByOne") {
  GameModel m;
  REQUIRE(m.tryMove(Direction::UP));
  REQUIRE_POS(m, 1, 0);
}

TEST_CASE("move_downIncreasesRowByOne") {
  GameModel m;
  REQUIRE(m.tryMove(Direction::DOWN));
  REQUIRE_POS(m, 1, 2);
}

TEST_CASE("move_leftDecreasesColumnByOne") {
  GameModel m;
  REQUIRE(m.tryMove(Direction::LEFT));
  REQUIRE_POS(m, 0, 1);
}

TEST_CASE("move_rightIncreasesColumnByOne") {
  GameModel m;
  REQUIRE(m.tryMove(Direction::RIGHT));
  REQUIRE_POS(m, 2, 1);
}

TEST_CASE("move_oppositeMovesCancelOut") {
  GameModel m;
  REQUIRE(m.tryMove(Direction::RIGHT));
  REQUIRE(m.tryMove(Direction::LEFT));
  REQUIRE_POS(m, 1, 1);
  REQUIRE(m.tryMove(Direction::DOWN));
  REQUIRE(m.tryMove(Direction::UP));
  REQUIRE_POS(m, 1, 1);
}

TEST_CASE("move_squarePathReturnsToStart") {
  GameModel m;
  REQUIRE(m.tryMove(Direction::RIGHT)); // (2,1)
  REQUIRE(m.tryMove(Direction::DOWN));  // (2,2)
  REQUIRE(m.tryMove(Direction::LEFT));  // (1,2)
  REQUIRE(m.tryMove(Direction::UP));    // (1,1)
  REQUIRE_POS(m, 1, 1);
}

TEST_CASE("move_longWalkAcrossOpenGroundWorks") {
  GameModel m;
  MOVE(m, Direction::DOWN, 4);   // (1,5)   column 1 is clear
  MOVE(m, Direction::RIGHT, 18); // (19,5)  row 5 is clear
  REQUIRE_POS(m, 19, 5);
}

TEST_CASE("marketFootprint_blocksMovementIntoEveryMarketTile") {
  for (const TilePosition &marketTile : GameModel::kMarketFootprint) {
    GameModel m;
    INFO("market tile (" << marketTile.column << ", " << marketTile.row
                         << ")");
    if (marketTile.row == 3) {
      MOVE(m, Direction::UP, 1);
      MOVE(m, Direction::RIGHT, marketTile.column - 1);
      MOVE(m, Direction::DOWN, 2);
      REQUIRE_FALSE(m.tryMove(Direction::DOWN));
      REQUIRE_POS(m, marketTile.column, 2);
    } else {
      MOVE(m, Direction::DOWN, 4);
      MOVE(m, Direction::RIGHT, marketTile.column - 1);
      REQUIRE_FALSE(m.tryMove(Direction::UP));
      REQUIRE_POS(m, marketTile.column, 5);
    }
  }
}

TEST_CASE("marketInteractionStandTileIsWalkableGround") {
  GameModel m;
  MOVE(m, Direction::DOWN, 4);
  MOVE(m, Direction::RIGHT, 9);
  REQUIRE_POS(m, 10, 5);
  REQUIRE(m.tryMove(Direction::RIGHT));
  REQUIRE_POS(m, 11, 5);
  REQUIRE_TILE(m, 10, 5, TileType::GROUND);
  REQUIRE_TILE(m, 11, 5, TileType::GROUND);
}

TEST_CASE("market_isFacingRequiresTheDesignatedTileInFront") {
  GameModel m;
  REQUIRE_FALSE(m.isFacingMarket());

  MOVE(m, Direction::DOWN, 4);
  MOVE(m, Direction::RIGHT, 9);
  REQUIRE_POS(m, 10, 5);
  REQUIRE_FALSE(m.tryMove(Direction::UP));
  REQUIRE(m.facingDirection() == Direction::UP);
  REQUIRE(m.tileInFront() == GameModel::kMarketInteractionPosition);
  REQUIRE(m.isFacingMarket());

  REQUIRE(m.tryMove(Direction::LEFT));
  REQUIRE_POS(m, 9, 5);
  REQUIRE_FALSE(m.isFacingMarket());
}

// ---------------------------------------------------------------------------
// tryMove: obstacles
// ---------------------------------------------------------------------------

TEST_CASE("obstacle_blocksMovingRight") {
  GameModel m;
  MOVE(m, Direction::RIGHT, 3); // (4,1); next tile (5,1) is a wall
  REQUIRE_POS(m, 4, 1);
  REQUIRE(!m.tryMove(Direction::RIGHT));
  REQUIRE_POS(m, 4, 1); // unchanged
}

TEST_CASE("obstacle_blocksMovingDown") {
  GameModel m;
  MOVE(m, Direction::DOWN, 4);   // (1,5)
  MOVE(m, Direction::RIGHT, 13); // (14,5)
  MOVE(m, Direction::DOWN, 2);   // (14,7); next tile (14,8) is a wall
  REQUIRE_POS(m, 14, 7);
  REQUIRE(!m.tryMove(Direction::DOWN));
  REQUIRE_POS(m, 14, 7);
}

TEST_CASE("obstacle_blocksMovingUp") {
  GameModel m;
  MOVE(m, Direction::DOWN, 8);   // (1,9)
  MOVE(m, Direction::RIGHT, 13); // (14,9); tile above (14,8) is a wall
  REQUIRE_POS(m, 14, 9);
  REQUIRE(!m.tryMove(Direction::UP));
  REQUIRE_POS(m, 14, 9);
}

TEST_CASE("obstacle_blocksMovingLeft") {
  GameModel m;
  MOVE(m, Direction::DOWN, 8);   // (1,9)
  MOVE(m, Direction::RIGHT, 14); // (15,9)
  MOVE(m, Direction::UP, 1);     // (15,8); tile to the left (14,8) is a wall
  REQUIRE_POS(m, 15, 8);
  REQUIRE(!m.tryMove(Direction::LEFT));
  REQUIRE_POS(m, 15, 8);
}

TEST_CASE("obstacle_blockedFromTheLeftSide") {
  GameModel m;
  MOVE(m, Direction::DOWN, 8);   // (1,9)
  MOVE(m, Direction::RIGHT, 12); // (13,9)
  MOVE(m, Direction::UP, 1);     // (13,8); tile to the right (14,8) is a wall
  REQUIRE_POS(m, 13, 8);
  REQUIRE(!m.tryMove(Direction::RIGHT));
  REQUIRE_POS(m, 13, 8);
}

TEST_CASE("obstacle_repeatedBlockedMovesNeverChangePosition") {
  GameModel m;
  MOVE(m, Direction::RIGHT, 3); // (4,1)
  for (int i = 0; i < 5; ++i) {
    REQUIRE(!m.tryMove(Direction::RIGHT));
    REQUIRE_POS(m, 4, 1);
  }
}

TEST_CASE("obstacle_canMoveAwayAfterBeingBlocked") {
  GameModel m;
  MOVE(m, Direction::RIGHT, 3); // (4,1)
  REQUIRE(!m.tryMove(Direction::RIGHT));
  REQUIRE(m.tryMove(Direction::DOWN)); // a blocked move must not "stick"
  REQUIRE_POS(m, 4, 2);
}

// ---------------------------------------------------------------------------
// tryMove: map edges
// ---------------------------------------------------------------------------

TEST_CASE("edge_topBlocksMovingUp") {
  GameModel m;
  MOVE(m, Direction::UP, 1); // (1,0)
  REQUIRE_POS(m, 1, 0);
  REQUIRE(!m.tryMove(Direction::UP)); // would be row -1
  REQUIRE_POS(m, 1, 0);
}

TEST_CASE("edge_leftBlocksMovingLeft") {
  GameModel m;
  MOVE(m, Direction::LEFT, 1); // (0,1)
  REQUIRE_POS(m, 0, 1);
  REQUIRE(!m.tryMove(Direction::LEFT)); // would be column -1
  REQUIRE_POS(m, 0, 1);
}

TEST_CASE("edge_bottomBlocksMovingDown") {
  GameModel m;
  MOVE(m, Direction::DOWN, 13); // (1,14); column 1 is clear
  REQUIRE_POS(m, 1, 14);
  REQUIRE(!m.tryMove(Direction::DOWN)); // would be row 15
  REQUIRE_POS(m, 1, 14);
}

TEST_CASE("edge_rightBlocksMovingRight") {
  GameModel m;
  MOVE(m, Direction::UP, 1);     // (1,0)
  MOVE(m, Direction::RIGHT, 18); // (19,0); row 0 is clear
  REQUIRE_POS(m, 19, 0);
  REQUIRE(!m.tryMove(Direction::RIGHT)); // would be column 20
  REQUIRE_POS(m, 19, 0);
}

TEST_CASE("corner_topLeftBlocksUpAndLeft") {
  GameModel m;
  MOVE(m, Direction::UP, 1);
  MOVE(m, Direction::LEFT, 1);
  REQUIRE_POS(m, 0, 0);
  REQUIRE(!m.tryMove(Direction::UP));
  REQUIRE(!m.tryMove(Direction::LEFT));
  REQUIRE_POS(m, 0, 0);
}

TEST_CASE("corner_bottomRightBlocksDownAndRight") {
  GameModel m;
  MOVE(m, Direction::UP, 1);     // (1,0)
  MOVE(m, Direction::RIGHT, 18); // (19,0)
  MOVE(m, Direction::DOWN, 14);  // (19,14); column 19 is clear
  REQUIRE_POS(m, 19, 14);
  REQUIRE(!m.tryMove(Direction::DOWN));
  REQUIRE(!m.tryMove(Direction::RIGHT));
  REQUIRE_POS(m, 19, 14);
}

// ---------------------------------------------------------------------------
// Property check: does not depend on the layout at all.
// 20,000 pseudo-random moves (fixed seed, so it is repeatable). After EVERY
// move: the player is in bounds and on ground; a successful move changed the
// position by exactly one tile; a rejected move changed nothing.
// ---------------------------------------------------------------------------

TEST_CASE("randomWalk_alwaysObeysTheRules") {
  GameModel m;
  const Direction directions[4] = {Direction::UP, Direction::DOWN,
                                   Direction::LEFT, Direction::RIGHT};
  std::uint32_t state = 12345u;
  for (int i = 0; i < 20000; ++i) {
    state = state * 1664525u + 1013904223u; // simple LCG
    const Direction d = directions[(state >> 16) % 4];

    const TilePosition before = m.playerPosition();
    const bool moved = m.tryMove(d);
    const TilePosition after = m.playerPosition();

    const int distance = std::abs(after.column - before.column) +
                         std::abs(after.row - before.row);

    INFO("random walk step " << i + 1);
    REQUIRE(m.isInBounds(after.column, after.row));
    REQUIRE(m.tileAt(after.column, after.row) == TileType::GROUND);
    REQUIRE(distance == (moved ? 1 : 0));
  }
}

// ---------------------------------------------------------------------------
// Facing direction
// The player starts facing DOWN. EVERY tryMove() call turns the player to the
// requested direction, whether the move succeeds or is blocked. An invalid
// Direction value changes nothing.
// ---------------------------------------------------------------------------

#define REQUIRE_FACING(model, expected)                                        \
  REQUIRE((model).facingDirection() == (expected))

TEST_CASE("facing_startsDown") {
  GameModel m;
  REQUIRE_FACING(m, Direction::DOWN);
}

// Successful moves. Each one checks position too, so we know it really moved.

TEST_CASE("facing_successfulMoveUpTurnsUp") {
  GameModel m;
  REQUIRE(m.tryMove(Direction::UP));
  REQUIRE_POS(m, 1, 0);
  REQUIRE_FACING(m, Direction::UP);
}

TEST_CASE("facing_successfulMoveLeftTurnsLeft") {
  GameModel m;
  REQUIRE(m.tryMove(Direction::LEFT));
  REQUIRE_POS(m, 0, 1);
  REQUIRE_FACING(m, Direction::LEFT);
}

TEST_CASE("facing_successfulMoveRightTurnsRight") {
  GameModel m;
  REQUIRE(m.tryMove(Direction::RIGHT));
  REQUIRE_POS(m, 2, 1);
  REQUIRE_FACING(m, Direction::RIGHT);
}

// The default is already DOWN, so turn away first or this proves nothing.
TEST_CASE("facing_successfulMoveDownTurnsDown") {
  GameModel m;
  REQUIRE(m.tryMove(Direction::RIGHT)); // (2,1), facing RIGHT
  REQUIRE(m.tryMove(Direction::DOWN));  // (2,2)
  REQUIRE_POS(m, 2, 2);
  REQUIRE_FACING(m, Direction::DOWN);
}

TEST_CASE("facing_followsTheLastRequestedDirection") {
  GameModel m;
  REQUIRE(m.tryMove(Direction::RIGHT));
  REQUIRE(m.tryMove(Direction::DOWN));
  REQUIRE(m.tryMove(Direction::LEFT));
  REQUIRE(m.tryMove(Direction::UP));
  REQUIRE_POS(m, 1, 1);
  REQUIRE_FACING(m, Direction::UP);
}

// Blocked moves. Each one starts facing a DIFFERENT way than the blocked
// request, so the turn is visible.

TEST_CASE("facing_blockedByObstacleStillTurns") {
  GameModel m;
  MOVE(m, Direction::DOWN, 8);   // (1,9)
  MOVE(m, Direction::RIGHT, 12); // (13,9)
  MOVE(m, Direction::UP, 1); // (13,8), facing UP; wall at (14,8) to the right
  REQUIRE_POS(m, 13, 8);
  REQUIRE_FACING(m, Direction::UP);

  REQUIRE(!m.tryMove(Direction::RIGHT));
  REQUIRE_POS(m, 13, 8);               // did not move
  REQUIRE_FACING(m, Direction::RIGHT); // but turned
}

TEST_CASE("facing_blockedFromBelowStillTurnsUp") {
  GameModel m;
  MOVE(m, Direction::DOWN, 8);   // (1,9)
  MOVE(m, Direction::RIGHT, 13); // (14,9), facing RIGHT; wall at (14,8) above
  REQUIRE_FACING(m, Direction::RIGHT);

  REQUIRE(!m.tryMove(Direction::UP));
  REQUIRE_POS(m, 14, 9);
  REQUIRE_FACING(m, Direction::UP);
}

TEST_CASE("facing_blockedByMapEdgeStillTurns") {
  GameModel m;
  MOVE(m, Direction::UP, 1);   // (1,0)
  MOVE(m, Direction::LEFT, 1); // (0,0), facing LEFT, in the top-left corner
  REQUIRE_FACING(m, Direction::LEFT);

  REQUIRE(!m.tryMove(Direction::UP)); // off the top of the map
  REQUIRE_POS(m, 0, 0);
  REQUIRE_FACING(m, Direction::UP);
}

// (13,8) has walls above (13,7) and to the right (14,8): two different blocked
// directions from one tile, so facing must follow each attempt.
TEST_CASE("facing_followsEachBlockedAttemptOnTheSameTile") {
  GameModel m;
  MOVE(m, Direction::DOWN, 8);   // (1,9)
  MOVE(m, Direction::RIGHT, 12); // (13,9)
  MOVE(m, Direction::UP, 1);     // (13,8)

  REQUIRE(!m.tryMove(Direction::UP));
  REQUIRE_FACING(m, Direction::UP);
  REQUIRE(!m.tryMove(Direction::RIGHT));
  REQUIRE_FACING(m, Direction::RIGHT);
  REQUIRE(!m.tryMove(Direction::UP));
  REQUIRE_FACING(m, Direction::UP);
  REQUIRE_POS(m, 13, 8);
}

// Invalid / separate state

TEST_CASE("facing_invalidDirectionIsIgnored") {
  GameModel m;
  REQUIRE(!m.tryMove(static_cast<Direction>(99)));
  REQUIRE_FACING(m, Direction::DOWN); // untouched at the start

  REQUIRE(m.tryMove(Direction::RIGHT));
  REQUIRE(!m.tryMove(static_cast<Direction>(99)));
  REQUIRE_FACING(m, Direction::RIGHT); // untouched after a real move
  REQUIRE_POS(m, 2, 1);
}

TEST_CASE("facing_eachModelHasItsOwnFacing") {
  GameModel a;
  GameModel b;
  REQUIRE(a.tryMove(Direction::RIGHT));
  REQUIRE_FACING(a, Direction::RIGHT);
  REQUIRE_FACING(b, Direction::DOWN); // b must not be affected by a
}

// ---------------------------------------------------------------------------
// tileInFront
// The tile the player faces = playerPosition() moved one step in
// facingDirection(). It only IDENTIFIES a target: no walkability check, no
// clamping (it may be off the map), and it never changes position or facing.
// ---------------------------------------------------------------------------

#define REQUIRE_FRONT(model, expectedColumn, expectedRow)                      \
  do {                                                                         \
    REQUIRE((model).tileInFront().column == (expectedColumn));                 \
    REQUIRE((model).tileInFront().row == (expectedRow));                       \
  } while (false)

// Each direction. Every test checks facing first, so a wrong result is not
// confused with the player simply not facing where the test thinks it is.

TEST_CASE("tileInFront_atStartIsTheTileBelow") {
  GameModel m; // (1,1), facing DOWN
  REQUIRE_FRONT(m, 1, 2);
}

TEST_CASE("tileInFront_facingUpIsRowMinusOne") {
  GameModel m;
  MOVE(m, Direction::DOWN, 4); // (1,5)
  MOVE(m, Direction::UP, 1);   // (1,4), facing UP
  REQUIRE_FACING(m, Direction::UP);
  REQUIRE_FRONT(m, 1, 3);
}

TEST_CASE("tileInFront_facingDownIsRowPlusOne") {
  GameModel m;
  MOVE(m, Direction::RIGHT, 1); // (2,1): turn away from the default first
  MOVE(m, Direction::DOWN, 1);  // (2,2), facing DOWN
  REQUIRE_FACING(m, Direction::DOWN);
  REQUIRE_FRONT(m, 2, 3);
}

TEST_CASE("tileInFront_facingLeftIsColumnMinusOne") {
  GameModel m;
  MOVE(m, Direction::RIGHT, 1); // (2,1)
  MOVE(m, Direction::LEFT, 1);  // (1,1), facing LEFT
  REQUIRE_FACING(m, Direction::LEFT);
  REQUIRE_FRONT(m, 0, 1);
}

TEST_CASE("tileInFront_facingRightIsColumnPlusOne") {
  GameModel m;
  MOVE(m, Direction::RIGHT, 1); // (2,1), facing RIGHT
  REQUIRE_FACING(m, Direction::RIGHT);
  REQUIRE_FRONT(m, 3, 1);
}

// Off the map: returned as-is, never clamped.

TEST_CASE("tileInFront_pastTopEdgeIsReturnedUnclamped") {
  GameModel m;
  MOVE(m, Direction::UP, 1); // (1,0), facing UP
  REQUIRE_FRONT(m, 1, -1);
  const TilePosition front = m.tileInFront();
  REQUIRE(!m.isInBounds(front.column, front.row));
}

TEST_CASE("tileInFront_pastLeftEdgeIsReturnedUnclamped") {
  GameModel m;
  MOVE(m, Direction::LEFT, 1); // (0,1), facing LEFT
  REQUIRE_FRONT(m, -1, 1);
  const TilePosition front = m.tileInFront();
  REQUIRE(!m.isInBounds(front.column, front.row));
}

TEST_CASE("tileInFront_pastBottomEdgeIsReturnedUnclamped") {
  GameModel m;
  MOVE(m, Direction::DOWN, 13); // (1,14), facing DOWN
  REQUIRE_FRONT(m, 1, GameModel::kMapHeight);
  const TilePosition front = m.tileInFront();
  REQUIRE(!m.isInBounds(front.column, front.row));
}

TEST_CASE("tileInFront_pastRightEdgeIsReturnedUnclamped") {
  GameModel m;
  MOVE(m, Direction::UP, 1);     // (1,0)
  MOVE(m, Direction::RIGHT, 18); // (19,0), facing RIGHT
  REQUIRE_FRONT(m, GameModel::kMapWidth, 0);
  const TilePosition front = m.tileInFront();
  REQUIRE(!m.isInBounds(front.column, front.row));
}

// No walkability check: a wall in front is still "the tile in front".
TEST_CASE("tileInFront_returnsAnObstacleTileWithoutJudgingIt") {
  GameModel m;
  MOVE(m, Direction::RIGHT, 3); // (4,1), facing RIGHT; (5,1) is a wall
  REQUIRE_FRONT(m, 5, 1);
  REQUIRE_TILE(m, 5, 1, TileType::OBSTACLE);
}

// Read-only: no side effects, and callable on a const model.
TEST_CASE("tileInFront_doesNotChangePositionOrFacing") {
  GameModel m;
  MOVE(m, Direction::RIGHT, 1); // (2,1), facing RIGHT
  const GameModel &readOnly = m;
  for (int i = 0; i < 3; ++i) {
    REQUIRE_FRONT(readOnly, 3, 1);
    REQUIRE_POS(readOnly, 2, 1);
    REQUIRE_FACING(readOnly, Direction::RIGHT);
  }
}

// After movement.

TEST_CASE("tileInFront_followsThePlayerAfterSuccessfulMoves") {
  GameModel m;
  REQUIRE_FRONT(m, 1, 2);
  REQUIRE(m.tryMove(Direction::RIGHT)); // (2,1)
  REQUIRE_FRONT(m, 3, 1);
  REQUIRE(m.tryMove(Direction::RIGHT)); // (3,1)
  REQUIRE_FRONT(m, 4, 1);
  REQUIRE(m.tryMove(Direction::DOWN)); // (3,2), now facing DOWN
  REQUIRE_POS(m, 3, 2);
  REQUIRE_FRONT(m, 3, 3);
}

TEST_CASE("tileInFront_afterBlockedMoveStaysOnTheWall") {
  GameModel m;
  MOVE(m, Direction::RIGHT, 3);          // (4,1), facing RIGHT
  REQUIRE(!m.tryMove(Direction::RIGHT)); // blocked by the wall at (5,1)
  REQUIRE_POS(m, 4, 1);
  REQUIRE_FRONT(m, 5, 1);
}

// The blocked move turns the player, so the front tile must switch sides even
// though the position did not change.
TEST_CASE("tileInFront_afterBlockedMoveUsesTheNewFacing") {
  GameModel m;
  MOVE(m, Direction::DOWN, 8);   // (1,9)
  MOVE(m, Direction::RIGHT, 12); // (13,9)
  MOVE(m, Direction::UP, 1);     // (13,8), facing UP
  REQUIRE_FRONT(m, 13, 7);

  REQUIRE(!m.tryMove(Direction::RIGHT)); // wall at (14,8)
  REQUIRE_POS(m, 13, 8);
  REQUIRE_FACING(m, Direction::RIGHT);
  REQUIRE_FRONT(m, 14, 8);
}

TEST_CASE("tileInFront_afterBlockedMoveAtMapEdgeIsOffTheMap") {
  GameModel m;
  MOVE(m, Direction::UP, 1);   // (1,0)
  MOVE(m, Direction::LEFT, 1); // (0,0), facing LEFT
  REQUIRE_FRONT(m, -1, 0);

  REQUIRE(!m.tryMove(Direction::UP)); // off the top of the map
  REQUIRE_POS(m, 0, 0);
  REQUIRE_FACING(m, Direction::UP);
  REQUIRE_FRONT(m, 0, -1);
}

// ---------------------------------------------------------------------------
// Tilling
// Terrain (tileAt) and farming state (isTilled) are separate layers. Only
// GROUND can be tilled, and only once. Tilled ground is still GROUND and still
// walkable. Test tiles are deliberately NOT on the diagonal (e.g. (3,2), not
// (2,2)), so a swapped column/row index cannot hide.
// ---------------------------------------------------------------------------

static int countTilled(const GameModel &m) {
  int count = 0;
  for (int row = 0; row < m.mapHeight(); ++row) {
    for (int column = 0; column < m.mapWidth(); ++column) {
      if (m.isTilled(column, row)) {
        ++count;
      }
    }
  }
  return count;
}

TEST_CASE("tilling_everyTileStartsUntilled") {
  GameModel m;
  REQUIRE(countTilled(m) == 0);
  REQUIRE(!m.isTilled(3, 2));
  REQUIRE(!m.isTilled(1, 1)); // the tile the player stands on
}

TEST_CASE("tilling_validGroundSucceeds") {
  GameModel m;
  REQUIRE(m.tryTill(3, 2));
  REQUIRE(m.isTilled(3, 2));
}

TEST_CASE("tilling_sameTileCannotBeTilledTwice") {
  GameModel m;
  REQUIRE(m.tryTill(3, 2));
  REQUIRE(!m.tryTill(3, 2));
  REQUIRE(!m.tryTill(3, 2));
  REQUIRE(m.isTilled(3, 2)); // a refused attempt must not undo it
}

TEST_CASE("tilling_everyObstacleRejectsTilling") {
  GameModel m;
  int obstaclesTried = 0;
  for (int row = 0; row < m.mapHeight(); ++row) {
    for (int column = 0; column < m.mapWidth(); ++column) {
      if (m.tileAt(column, row) == TileType::OBSTACLE) {
        REQUIRE(!m.tryTill(column, row));
        REQUIRE(!m.isTilled(column, row));
        ++obstaclesTried;
      }
    }
  }
  REQUIRE(obstaclesTried == 23);
  REQUIRE(countTilled(m) == 0);
}

TEST_CASE("tilling_outOfBoundsRejectsTilling") {
  GameModel m;
  REQUIRE(!m.tryTill(-1, 0));
  REQUIRE(!m.tryTill(0, -1));
  REQUIRE(!m.tryTill(-1, -1));
  REQUIRE(!m.tryTill(GameModel::kMapWidth, 0));
  REQUIRE(!m.tryTill(0, GameModel::kMapHeight));
  REQUIRE(!m.tryTill(GameModel::kMapWidth, GameModel::kMapHeight));
  REQUIRE(countTilled(m) == 0);
}

// In the flat array, an off-map coordinate can land on a REAL tile:
// (20,0) -> index 20 -> tile (0,1), and (-1,1) -> index 19 -> tile (19,0).
// A missing bounds check would till those instead of rejecting.
TEST_CASE("tilling_outOfBoundsNeverTouchesAnotherTile") {
  GameModel m;
  REQUIRE(!m.tryTill(GameModel::kMapWidth, 0));
  REQUIRE(!m.isTilled(0, 1));
  REQUIRE(!m.tryTill(-1, 1));
  REQUIRE(!m.isTilled(19, 0));
  REQUIRE(countTilled(m) == 0);
}

// Same aliasing, on the read side.
TEST_CASE("isTilled_outOfBoundsIsFalseBesideTilledTiles") {
  GameModel m;
  REQUIRE(m.tryTill(0, 1));
  REQUIRE(m.tryTill(19, 0));
  REQUIRE(!m.isTilled(GameModel::kMapWidth, 0)); // would alias (0,1)
  REQUIRE(!m.isTilled(-1, 1));                   // would alias (19,0)
}

TEST_CASE("tilling_tilledTileRemainsGround") {
  GameModel m;
  REQUIRE(m.tryTill(3, 2));
  REQUIRE_TILE(m, 3, 2, TileType::GROUND);
}

TEST_CASE("tilling_marketFootprintRejectsEveryMarketTile") {
  GameModel m;
  for (const TilePosition &marketTile : GameModel::kMarketFootprint) {
    INFO("market tile (" << marketTile.column << ", " << marketTile.row
                         << ")");
    REQUIRE_FALSE(m.tryTill(marketTile.column, marketTile.row));
    REQUIRE_FALSE(m.isTilled(marketTile.column, marketTile.row));
    REQUIRE_FALSE(m.tryPlant(marketTile.column, marketTile.row));
    REQUIRE_FALSE(m.isPlanted(marketTile.column, marketTile.row));
  }
  REQUIRE(m.tryTill(3, 2));
  REQUIRE(m.tryPlant(3, 2));
}

TEST_CASE("tilling_neverChangesTerrainOnAnyTile") {
  GameModel m;
  for (int row = 0; row < m.mapHeight(); ++row) {
    for (int column = 0; column < m.mapWidth(); ++column) {
      const TileType before = m.tileAt(column, row);
      m.tryTill(column, row);
      REQUIRE_TILE(m, column, row, before);
    }
  }
}

TEST_CASE("tilling_changesOnlyTheTargetTile") {
  GameModel m;
  REQUIRE(m.tryTill(3, 2));
  REQUIRE(countTilled(m) == 1);
  REQUIRE(!m.isTilled(2, 2));
  REQUIRE(!m.isTilled(4, 2));
  REQUIRE(!m.isTilled(3, 1));
  REQUIRE(!m.isTilled(3, 3));
  REQUIRE(!m.isTilled(2, 3)); // what a swapped (column,row) index would hit
}

TEST_CASE("tilling_failedAttemptsDoNotChangeState") {
  GameModel m;
  REQUIRE(m.tryTill(3, 2));
  REQUIRE(countTilled(m) == 1);
  REQUIRE(!m.tryTill(3, 2));                    // already tilled
  REQUIRE(!m.tryTill(5, 1));                    // obstacle
  REQUIRE(!m.tryTill(-1, 0));                   // off the map
  REQUIRE(!m.tryTill(GameModel::kMapWidth, 0)); // off the map
  REQUIRE(countTilled(m) == 1);
  REQUIRE(m.isTilled(3, 2));
}

TEST_CASE("tilling_playerCanWalkAcrossTilledGround") {
  GameModel m;
  REQUIRE(m.tryTill(2, 1));
  REQUIRE(m.tryTill(3, 1));
  REQUIRE(m.tryTill(4, 1));
  MOVE(m, Direction::RIGHT, 3); // (4,1): every step lands on tilled soil
  REQUIRE_POS(m, 4, 1);
  MOVE(m, Direction::LEFT, 3); // and back to (1,1)
  REQUIRE_POS(m, 1, 1);
  REQUIRE(m.isTilled(2, 1)); // walking does not undo tilling
  REQUIRE(m.isTilled(3, 1));
  REQUIRE(m.isTilled(4, 1));
}

// The way the SDL layer will use it: target = tileInFront().
TEST_CASE("tilling_worksWithTileInFront") {
  GameModel m; // (1,1), facing DOWN
  const TilePosition front = m.tileInFront();
  REQUIRE(m.tryTill(front.column, front.row));
  REQUIRE(m.isTilled(1, 2));
  REQUIRE(m.tryMove(Direction::DOWN)); // step onto the new soil
  REQUIRE_POS(m, 1, 2);
  REQUIRE(m.isTilled(1, 2));
}

TEST_CASE("tilling_tileInFrontThatCannotBeTilledIsRejected") {
  GameModel wall;
  MOVE(wall, Direction::RIGHT, 3); // (4,1), facing RIGHT; wall at (5,1)
  const TilePosition wallFront = wall.tileInFront();
  REQUIRE(!wall.tryTill(wallFront.column, wallFront.row));
  REQUIRE(!wall.isTilled(5, 1));

  GameModel edge;
  MOVE(edge, Direction::UP, 1); // (1,0), facing UP; front is off the map
  const TilePosition edgeFront = edge.tileInFront();
  REQUIRE(!edge.tryTill(edgeFront.column, edgeFront.row));
  REQUIRE(countTilled(edge) == 0);
}

TEST_CASE("tilling_eachModelHasItsOwnFarmState") {
  GameModel a;
  GameModel b;
  REQUIRE(a.tryTill(3, 2));
  REQUIRE(a.isTilled(3, 2));
  REQUIRE(!b.isTilled(3, 2));
  REQUIRE(b.tryTill(3, 2)); // b is unaffected, so it can till the same tile
}

// ---------------------------------------------------------------------------
// Planting
// A seed can only go on TILLED, EMPTY soil. Planting adds crop state on top of
// the existing layers: it must not change terrain and must not un-till the
// soil. One crop type, unlimited seeds, no growth yet.
// ---------------------------------------------------------------------------

static int countPlanted(const GameModel &m) {
  int count = 0;
  for (int row = 0; row < m.mapHeight(); ++row) {
    for (int column = 0; column < m.mapWidth(); ++column) {
      if (m.isPlanted(column, row)) {
        ++count;
      }
    }
  }
  return count;
}

static void tillAndPlant(GameModel &m, int column, int row) {
  REQUIRE(m.tryTill(column, row));
  REQUIRE(m.tryPlant(column, row));
}

TEST_CASE("planting_everyTileStartsEmpty") {
  GameModel m;
  REQUIRE(countPlanted(m) == 0);
  REQUIRE(m.tryTill(3, 2));
  REQUIRE(!m.isPlanted(3, 2)); // tilling alone does not plant anything
}

TEST_CASE("planting_tilledEmptySoilSucceeds") {
  GameModel m;
  REQUIRE(m.tryTill(3, 2));
  REQUIRE(m.tryPlant(3, 2));
  REQUIRE(m.isPlanted(3, 2));
}

TEST_CASE("planting_untilledGroundFails") {
  GameModel m;
  REQUIRE(!m.tryPlant(3, 2));
  REQUIRE(!m.isPlanted(3, 2));
  REQUIRE(
      !m.isTilled(3, 2)); // a refused plant must not till it as a side effect
}

TEST_CASE("planting_everyObstacleRejects") {
  GameModel m;
  int obstaclesTried = 0;
  for (int row = 0; row < m.mapHeight(); ++row) {
    for (int column = 0; column < m.mapWidth(); ++column) {
      if (m.tileAt(column, row) == TileType::OBSTACLE) {
        REQUIRE(!m.tryPlant(column, row));
        REQUIRE(!m.isPlanted(column, row));
        ++obstaclesTried;
      }
    }
  }
  REQUIRE(obstaclesTried == 23);
  REQUIRE(countPlanted(m) == 0);
}

TEST_CASE("planting_sameTileCannotBePlantedTwice") {
  GameModel m;
  tillAndPlant(m, 3, 2);
  REQUIRE(!m.tryPlant(3, 2));
  REQUIRE(!m.tryPlant(3, 2));
  REQUIRE(m.isPlanted(3, 2)); // a refused attempt must not remove the crop
  REQUIRE(countPlanted(m) == 1);
}

TEST_CASE("planting_outOfBoundsFails") {
  GameModel m;
  REQUIRE(!m.tryPlant(-1, 0));
  REQUIRE(!m.tryPlant(0, -1));
  REQUIRE(!m.tryPlant(-1, -1));
  REQUIRE(!m.tryPlant(GameModel::kMapWidth, 0));
  REQUIRE(!m.tryPlant(0, GameModel::kMapHeight));
  REQUIRE(!m.tryPlant(GameModel::kMapWidth, GameModel::kMapHeight));
  REQUIRE(countPlanted(m) == 0);
}

// Flat-array trap again: (20,0) is index 20 = tile (0,1); (-1,1) is index 19 =
// tile (19,0). Both are made TILLED here, so a missing bounds check would let
// the off-map attempt plant on them instead of being rejected.
TEST_CASE("planting_outOfBoundsNeverTouchesAnotherTile") {
  GameModel m;
  REQUIRE(m.tryTill(0, 1));
  REQUIRE(m.tryTill(19, 0));
  REQUIRE(!m.tryPlant(GameModel::kMapWidth, 0));
  REQUIRE(!m.isPlanted(0, 1));
  REQUIRE(!m.tryPlant(-1, 1));
  REQUIRE(!m.isPlanted(19, 0));
  REQUIRE(countPlanted(m) == 0);
}

TEST_CASE("isPlanted_outOfBoundsIsFalseBesidePlantedTiles") {
  GameModel m;
  tillAndPlant(m, 0, 1);
  tillAndPlant(m, 19, 0);
  REQUIRE(!m.isPlanted(GameModel::kMapWidth, 0)); // would alias (0,1)
  REQUIRE(!m.isPlanted(-1, 1));                   // would alias (19,0)
}

TEST_CASE("planting_tilledSoilRemainsTilled") {
  GameModel m;
  tillAndPlant(m, 3, 2);
  REQUIRE(m.isTilled(3, 2));
}

TEST_CASE("planting_terrainRemainsGround") {
  GameModel m;
  tillAndPlant(m, 3, 2);
  REQUIRE_TILE(m, 3, 2, TileType::GROUND);
}

TEST_CASE("planting_changesOnlyTheTargetTile") {
  GameModel m;
  REQUIRE(m.tryTill(3, 2));
  REQUIRE(m.tryTill(4, 2)); // tilled neighbour that stays EMPTY
  REQUIRE(m.tryPlant(3, 2));
  REQUIRE(countPlanted(m) == 1);
  REQUIRE(m.isTilled(4, 2));
  REQUIRE(!m.isPlanted(4, 2)); // tilled but empty is its own state
  REQUIRE(!m.isPlanted(2, 2));
  REQUIRE(!m.isPlanted(3, 1));
  REQUIRE(!m.isPlanted(3, 3));
  REQUIRE(!m.isPlanted(2, 3)); // what a swapped (column,row) index would hit
  REQUIRE(m.tryPlant(4, 2));   // the neighbour can still be planted afterwards
  REQUIRE(countPlanted(m) == 2);
}

TEST_CASE("planting_failedAttemptsDoNotChangeState") {
  GameModel m;
  tillAndPlant(m, 3, 2);
  REQUIRE(!m.tryPlant(3, 2));                    // already planted
  REQUIRE(!m.tryPlant(6, 2));                    // untilled ground
  REQUIRE(!m.tryPlant(5, 1));                    // obstacle
  REQUIRE(!m.tryPlant(-1, 0));                   // off the map
  REQUIRE(!m.tryPlant(GameModel::kMapWidth, 0)); // off the map
  REQUIRE(countPlanted(m) == 1);
  REQUIRE(countTilled(m) == 1); // failed plants did not till anything
  REQUIRE(m.isPlanted(3, 2));
}

// Tilling an already-planted tile is refused, and must not wipe the crop.
TEST_CASE("planting_cropSurvivesAFailedTill") {
  GameModel m;
  tillAndPlant(m, 3, 2);
  REQUIRE(!m.tryTill(3, 2));
  REQUIRE(m.isTilled(3, 2));
  REQUIRE(m.isPlanted(3, 2));
}

TEST_CASE("planting_playerCanWalkOverPlantedSoil") {
  GameModel m;
  tillAndPlant(m, 2, 1);
  tillAndPlant(m, 3, 1);
  tillAndPlant(m, 4, 1);
  MOVE(m, Direction::RIGHT, 3); // (4,1): every step lands on a crop
  REQUIRE_POS(m, 4, 1);
  MOVE(m, Direction::LEFT, 3); // and back to (1,1)
  REQUIRE_POS(m, 1, 1);
  REQUIRE(m.isPlanted(2, 1)); // walking does not trample anything
  REQUIRE(m.isPlanted(3, 1));
  REQUIRE(m.isPlanted(4, 1));
  REQUIRE(m.isTilled(3, 1));
}

// The way the SDL layer will use it: target = tileInFront().
TEST_CASE("planting_worksWithTileInFront") {
  GameModel m; // (1,1), facing DOWN
  const TilePosition front = m.tileInFront();
  REQUIRE(m.tryTill(front.column, front.row));
  REQUIRE(m.tryPlant(front.column, front.row));
  REQUIRE(m.isPlanted(1, 2));
  REQUIRE(m.tryMove(Direction::DOWN)); // step onto the planted tile
  REQUIRE_POS(m, 1, 2);
  REQUIRE(m.isPlanted(1, 2));
}

TEST_CASE("planting_tileInFrontThatCannotBePlantedIsRejected") {
  GameModel untilled; // (1,2) is ground but not tilled
  const TilePosition soil = untilled.tileInFront();
  REQUIRE(!untilled.tryPlant(soil.column, soil.row));

  GameModel wall;
  MOVE(wall, Direction::RIGHT, 3); // (4,1), facing RIGHT; wall at (5,1)
  const TilePosition wallFront = wall.tileInFront();
  REQUIRE(!wall.tryPlant(wallFront.column, wallFront.row));

  GameModel edge;
  MOVE(edge, Direction::UP, 1); // (1,0), facing UP; front is off the map
  const TilePosition edgeFront = edge.tileInFront();
  REQUIRE(!edge.tryPlant(edgeFront.column, edgeFront.row));

  REQUIRE(countPlanted(untilled) + countPlanted(wall) + countPlanted(edge) ==
          0);
}

TEST_CASE("planting_eachModelHasItsOwnCrops") {
  GameModel a;
  GameModel b;
  tillAndPlant(a, 3, 2);
  REQUIRE(a.isPlanted(3, 2));
  REQUIRE(!b.isPlanted(3, 2));
  REQUIRE(!b.tryPlant(3, 2)); // b's soil is untilled, so it still refuses
}

// ---------------------------------------------------------------------------
// Watering and growth (GameModel level; Crop is tested separately)
// Rules under test:
//   - only an existing crop that is not yet watered today can be watered
//   - advanceDay() gives one growth day to each crop watered that day, then
//     everything is dry again
//   - progress is cumulative: a missed day pauses growth, never resets it
//   - SPROUT after 3 watered days, MATURE after 3 MORE (6 in total)
// Tiles are off the diagonal ((3,2), (4,2), ...) so a swapped column/row index
// cannot hide.
// ---------------------------------------------------------------------------

// Watered days needed, counted from planting. If the design changes, change
// these two numbers; every growth test below is written in terms of them.
static const int kDaysToSprout = 3;
static const int kDaysToMature = 6; // 3 more days after sprouting

#define REQUIRE_STAGE(model, column, row, expected)                            \
  REQUIRE((model).cropStageAt((column), (row)) == (expected))

static int countWatered(const GameModel &m) {
  int count = 0;
  for (int row = 0; row < m.mapHeight(); ++row) {
    for (int column = 0; column < m.mapWidth(); ++column) {
      if (m.isWateredToday(column, row)) {
        ++count;
      }
    }
  }
  return count;
}

static void advanceDays(GameModel &m, int days) {
  for (int i = 0; i < days; ++i) {
    m.advanceDay();
  }
}

// One watered day for a single crop: water it, then end the day.
static void waterAndAdvance(GameModel &m, int column, int row) {
  REQUIRE(m.tryWater(column, row));
  m.advanceDay();
}

// `days` watered days in a row for a single crop.
static void waterDays(GameModel &m, int column, int row, int days) {
  for (int i = 0; i < days; ++i) {
    waterAndAdvance(m, column, row);
  }
}

// ---- Watering eligibility --------------------------------------------------

TEST_CASE("watering_noCropMeansStageNoneAndNotWatered") {
  GameModel m;
  REQUIRE_STAGE(m, 3, 2, CropStage::NONE);
  REQUIRE(!m.isWateredToday(3, 2));
  for (int row = 0; row < m.mapHeight(); ++row) {
    for (int column = 0; column < m.mapWidth(); ++column) {
      REQUIRE_STAGE(m, column, row, CropStage::NONE);
    }
  }
  REQUIRE(countWatered(m) == 0);
}

TEST_CASE("watering_plantedCropStartsAsADrySeed") {
  GameModel m;
  tillAndPlant(m, 3, 2);
  REQUIRE_STAGE(m, 3, 2, CropStage::SEED);
  REQUIRE(!m.isWateredToday(3, 2));
}

TEST_CASE("watering_plantedCropCanBeWatered") {
  GameModel m;
  tillAndPlant(m, 3, 2);
  REQUIRE(m.tryWater(3, 2));
  REQUIRE(m.isWateredToday(3, 2));
  REQUIRE_STAGE(m, 3, 2,
                CropStage::SEED); // growth only happens in advanceDay()
}

TEST_CASE("watering_soilWithoutACropRejects") {
  GameModel m;
  REQUIRE(m.tryTill(3, 2)); // tilled but empty
  REQUIRE(!m.tryWater(3, 2));
  REQUIRE(!m.isWateredToday(3, 2));
  REQUIRE_STAGE(m, 3, 2, CropStage::NONE);
  REQUIRE(!m.tryWater(6, 2)); // ordinary untilled ground
  REQUIRE(!m.isWateredToday(6, 2));
}

TEST_CASE("watering_everyObstacleRejects") {
  GameModel m;
  int obstaclesTried = 0;
  for (int row = 0; row < m.mapHeight(); ++row) {
    for (int column = 0; column < m.mapWidth(); ++column) {
      if (m.tileAt(column, row) == TileType::OBSTACLE) {
        REQUIRE(!m.tryWater(column, row));
        REQUIRE(!m.isWateredToday(column, row));
        REQUIRE_STAGE(m, column, row, CropStage::NONE);
        ++obstaclesTried;
      }
    }
  }
  REQUIRE(obstaclesTried == 23);
}

TEST_CASE("watering_outOfBoundsRejects") {
  GameModel m;
  REQUIRE(!m.tryWater(-1, 0));
  REQUIRE(!m.tryWater(0, -1));
  REQUIRE(!m.tryWater(-1, -1));
  REQUIRE(!m.tryWater(GameModel::kMapWidth, 0));
  REQUIRE(!m.tryWater(0, GameModel::kMapHeight));
  REQUIRE(!m.tryWater(GameModel::kMapWidth, GameModel::kMapHeight));
  REQUIRE(countWatered(m) == 0);
}

TEST_CASE("watering_queriesOffTheMapAreFalseOrNone") {
  GameModel m;
  REQUIRE(!m.isWateredToday(-1, 0));
  REQUIRE(!m.isWateredToday(0, GameModel::kMapHeight));
  REQUIRE(!m.isWateredToday(GameModel::kMapWidth, GameModel::kMapHeight));
  REQUIRE_STAGE(m, -1, 0, CropStage::NONE);
  REQUIRE_STAGE(m, 0, GameModel::kMapHeight, CropStage::NONE);
  REQUIRE_STAGE(m, GameModel::kMapWidth, GameModel::kMapHeight,
                CropStage::NONE);
}

// Flat-array trap: (20,0) is index 20 = tile (0,1); (-1,1) is index 19 = tile
// (19,0). Both have real crops here, so a missing bounds check would water
// them instead of rejecting the off-map attempt.
TEST_CASE("watering_outOfBoundsNeverTouchesAnotherCrop") {
  GameModel m;
  tillAndPlant(m, 0, 1);
  tillAndPlant(m, 19, 0);
  REQUIRE(!m.tryWater(GameModel::kMapWidth, 0));
  REQUIRE(!m.isWateredToday(0, 1));
  REQUIRE(!m.tryWater(-1, 1));
  REQUIRE(!m.isWateredToday(19, 0));
  REQUIRE(countWatered(m) == 0);
}

// Same aliasing on the read side: the real crops are watered, the off-map
// neighbours must still report "not watered" and "no crop".
TEST_CASE("watering_queriesOffTheMapIgnoreCropsAtAliasedTiles") {
  GameModel m;
  tillAndPlant(m, 0, 1);
  tillAndPlant(m, 19, 0);
  REQUIRE(m.tryWater(0, 1));
  REQUIRE(m.tryWater(19, 0));
  REQUIRE(!m.isWateredToday(GameModel::kMapWidth, 0)); // would alias (0,1)
  REQUIRE(!m.isWateredToday(-1, 1));                   // would alias (19,0)
  REQUIRE_STAGE(m, GameModel::kMapWidth, 0, CropStage::NONE);
  REQUIRE_STAGE(m, -1, 1, CropStage::NONE);
}

// ---- Duplicate watering ----------------------------------------------------

TEST_CASE("watering_sameCropCannotBeWateredTwiceInOneDay") {
  GameModel m;
  tillAndPlant(m, 3, 2);
  REQUIRE(m.tryWater(3, 2));
  REQUIRE(!m.tryWater(3, 2));
  REQUIRE(!m.tryWater(3, 2));
  REQUIRE(m.isWateredToday(3, 2));
}

// Failed attempts must not add growth either: the crop should still need
// exactly kDaysToSprout watered DAYS, however many extra attempts were made.
TEST_CASE("watering_failedAttemptsDoNotChangeState") {
  GameModel m;
  tillAndPlant(m, 3, 2);
  REQUIRE(m.tryWater(3, 2));
  REQUIRE(!m.tryWater(3, 2));                    // already watered
  REQUIRE(!m.tryWater(3, 2));                    // and again
  REQUIRE(!m.tryWater(6, 2));                    // untilled ground
  REQUIRE(!m.tryWater(5, 1));                    // obstacle
  REQUIRE(!m.tryWater(-1, 0));                   // off the map
  REQUIRE(!m.tryWater(GameModel::kMapWidth, 0)); // off the map
  REQUIRE(countWatered(m) == 1);
  m.advanceDay();                        // watered day 1
  waterDays(m, 3, 2, kDaysToSprout - 2); // watered days 2 .. (sprout - 1)
  REQUIRE_STAGE(m, 3, 2, CropStage::SEED);
  waterAndAdvance(m, 3, 2); // watered day 3
  REQUIRE_STAGE(m, 3, 2, CropStage::SPROUT);
}

TEST_CASE("watering_onlyTheTargetedCropGetsWater") {
  GameModel m;
  tillAndPlant(m, 3, 2);
  tillAndPlant(m, 4, 2);
  REQUIRE(m.tryWater(3, 2));
  REQUIRE(m.isWateredToday(3, 2));
  REQUIRE(!m.isWateredToday(4, 2));
  REQUIRE(
      !m.isWateredToday(2, 3)); // what a swapped (column,row) index would hit
  REQUIRE(countWatered(m) == 1);
  m.advanceDay();
  waterDays(m, 3, 2, kDaysToSprout - 1); // only (3,2) is ever watered
  REQUIRE_STAGE(m, 3, 2, CropStage::SPROUT);
  REQUIRE_STAGE(m, 4, 2, CropStage::SEED);
}

// ---- Daily reset -----------------------------------------------------------

TEST_CASE("advanceDay_clearsWateringSoCropsCanBeWateredAgain") {
  GameModel m;
  tillAndPlant(m, 3, 2);
  REQUIRE(m.tryWater(3, 2));
  REQUIRE(m.isWateredToday(3, 2));
  m.advanceDay();
  REQUIRE(!m.isWateredToday(3, 2));
  REQUIRE(m.tryWater(3, 2));  // allowed again on the new day
  REQUIRE(!m.tryWater(3, 2)); // but still only once per day
}

TEST_CASE("advanceDay_onAFarmWithNoCropsChangesNothing") {
  GameModel m;
  REQUIRE(m.tryTill(3, 2)); // tilled, no crop
  advanceDays(m, 3);
  REQUIRE(countPlanted(m) == 0);
  REQUIRE(countWatered(m) == 0);
  REQUIRE(countTilled(m) == 1);
  REQUIRE_STAGE(m, 3, 2, CropStage::NONE);
  REQUIRE_POS(m, 1, 1);
  REQUIRE_FACING(m, Direction::DOWN);
}

// ---- Growth stages: SEED -> (3 watered days) SPROUT -> (3 more) MATURE -----

TEST_CASE("growth_wateringAloneDoesNotGrowAndOneDayIsNotEnough") {
  GameModel m;
  tillAndPlant(m, 3, 2);
  REQUIRE(m.tryWater(3, 2));
  REQUIRE_STAGE(m, 3, 2, CropStage::SEED); // watering alone does not grow it
  m.advanceDay();
  REQUIRE_STAGE(m, 3, 2, CropStage::SEED); // one watered day is not enough
}

TEST_CASE("growth_thirdWateredDayMakesASprout") {
  GameModel m;
  tillAndPlant(m, 3, 2);
  waterDays(m, 3, 2, kDaysToSprout - 1);
  REQUIRE_STAGE(m, 3, 2, CropStage::SEED); // two watered days: still a seed
  waterAndAdvance(m, 3, 2);
  REQUIRE_STAGE(m, 3, 2, CropStage::SPROUT);
}

TEST_CASE("growth_sproutStaysASproutUntilTheSixthWateredDay") {
  GameModel m;
  tillAndPlant(m, 3, 2);
  waterDays(m, 3, 2, kDaysToSprout);
  REQUIRE_STAGE(m, 3, 2, CropStage::SPROUT);
  waterDays(m, 3, 2, kDaysToMature - kDaysToSprout - 1);
  REQUIRE_STAGE(m, 3, 2, CropStage::SPROUT); // five watered days in total
}

TEST_CASE("growth_sixthWateredDayMakesItMature") {
  GameModel m;
  tillAndPlant(m, 3, 2);
  waterDays(m, 3, 2, kDaysToMature - 1);
  REQUIRE_STAGE(m, 3, 2, CropStage::SPROUT);
  waterAndAdvance(m, 3, 2);
  REQUIRE_STAGE(m, 3, 2, CropStage::MATURE);
}

// Return value of tryWater is deliberately not asserted here: the rules do not
// say whether a mature crop may still be watered, only that it stays MATURE.
TEST_CASE("growth_matureCropStaysMature") {
  GameModel m;
  tillAndPlant(m, 3, 2);
  waterDays(m, 3, 2, kDaysToMature);
  REQUIRE_STAGE(m, 3, 2, CropStage::MATURE);
  for (int day = 0; day < 6; ++day) {
    if (day % 2 == 0) {
      m.tryWater(3, 2);
    }
    m.advanceDay();
    REQUIRE_STAGE(m, 3, 2, CropStage::MATURE);
  }
}

TEST_CASE("growth_cropsGrowIndependently") {
  GameModel m;
  tillAndPlant(m, 3, 2); // A: watered every day
  tillAndPlant(m, 4, 2); // B: watered for the first 3 days only
  tillAndPlant(m, 3, 3); // C: never watered
  for (int day = 0; day < kDaysToMature; ++day) {
    REQUIRE(m.tryWater(3, 2));
    if (day < kDaysToSprout) {
      REQUIRE(m.tryWater(4, 2));
    }
    m.advanceDay();
  }
  REQUIRE_STAGE(m, 3, 2, CropStage::MATURE);
  REQUIRE_STAGE(m, 4, 2, CropStage::SPROUT);
  REQUIRE_STAGE(m, 3, 3, CropStage::SEED);
}

TEST_CASE("growth_cropPlantedLaterStartsAsASeed") {
  GameModel m;
  tillAndPlant(m, 3, 2);
  waterDays(m, 3, 2, kDaysToMature);
  REQUIRE_STAGE(m, 3, 2, CropStage::MATURE);

  tillAndPlant(m, 4, 2); // a new crop, planted much later
  REQUIRE_STAGE(m, 4, 2, CropStage::SEED);
  REQUIRE_STAGE(m, 3, 2, CropStage::MATURE);
  waterDays(m, 4, 2, kDaysToSprout);
  REQUIRE_STAGE(m, 4, 2, CropStage::SPROUT);
  REQUIRE_STAGE(m, 3, 2, CropStage::MATURE); // unwatered since, still mature
}

// ---- Missed days -----------------------------------------------------------

TEST_CASE("growth_unwateredDaysDoNotGrowTheCrop") {
  GameModel m;
  tillAndPlant(m, 3, 2);
  advanceDays(m, 5);
  REQUIRE_STAGE(m, 3, 2, CropStage::SEED);
  waterDays(m, 3, 2, kDaysToSprout - 1); // dry days earlier count for nothing
  REQUIRE_STAGE(m, 3, 2, CropStage::SEED);
  waterAndAdvance(m, 3, 2);
  REQUIRE_STAGE(m, 3, 2, CropStage::SPROUT);
}

TEST_CASE("growth_missedDayPausesButDoesNotResetProgress") {
  GameModel m;
  tillAndPlant(m, 3, 2);
  waterDays(m, 3, 2, 2); // watered days 1-2
  REQUIRE_STAGE(m, 3, 2, CropStage::SEED);

  m.advanceDay(); // missed day: progress must stay at 2
  REQUIRE_STAGE(m, 3, 2, CropStage::SEED);

  waterAndAdvance(m, 3, 2);                  // watered day 3
  REQUIRE_STAGE(m, 3, 2, CropStage::SPROUT); // would still be SEED if reset

  advanceDays(m, 2); // two missed days: a sprout does not revert to a seed
  REQUIRE_STAGE(m, 3, 2, CropStage::SPROUT);

  waterDays(m, 3, 2, 2); // watered days 4-5
  REQUIRE_STAGE(m, 3, 2, CropStage::SPROUT);

  m.advanceDay(); // another missed day
  REQUIRE_STAGE(m, 3, 2, CropStage::SPROUT);

  waterAndAdvance(m, 3, 2); // watered day 6: ten days passed, six watered
  REQUIRE_STAGE(m, 3, 2, CropStage::MATURE);
}

// ---- Existing behaviour stays unchanged -----------------------------------

TEST_CASE("advanceDay_leavesTerrainTillingPlantingAndPlayerUnchanged") {
  GameModel m;
  tillAndPlant(m, 3, 2);
  MOVE(m, Direction::RIGHT, 1); // (2,1), facing RIGHT
  REQUIRE(m.tryWater(3, 2));
  advanceDays(m, 2);
  REQUIRE_POS(m, 2, 1);
  REQUIRE_FACING(m, Direction::RIGHT);
  REQUIRE_TILE(m, 3, 2, TileType::GROUND);
  REQUIRE(m.isTilled(3, 2));
  REQUIRE(m.isPlanted(3, 2));
  REQUIRE(countTilled(m) == 1);
  REQUIRE(countPlanted(m) == 1);
}

TEST_CASE("growth_playerCanStillWalkOverGrownCrops") {
  GameModel m;
  tillAndPlant(m, 2, 1);
  tillAndPlant(m, 3, 1);
  tillAndPlant(m, 4, 1);
  for (int day = 0; day < kDaysToMature; ++day) {
    REQUIRE(m.tryWater(2, 1));
    REQUIRE(m.tryWater(3, 1));
    REQUIRE(m.tryWater(4, 1));
    m.advanceDay();
  }
  REQUIRE_STAGE(m, 2, 1, CropStage::MATURE);
  REQUIRE_STAGE(m, 3, 1, CropStage::MATURE);
  REQUIRE_STAGE(m, 4, 1, CropStage::MATURE);
  MOVE(m, Direction::RIGHT, 3); // (4,1), over three mature crops
  REQUIRE_POS(m, 4, 1);
  MOVE(m, Direction::LEFT, 3);
  REQUIRE_POS(m, 1, 1);
  REQUIRE_STAGE(m, 3, 1, CropStage::MATURE); // walking does not harm them
}

// The way the SDL layer will use it: target = tileInFront().
TEST_CASE("watering_worksWithTileInFront") {
  GameModel m; // (1,1), facing DOWN
  const TilePosition front = m.tileInFront();
  REQUIRE(m.tryTill(front.column, front.row));
  REQUIRE(m.tryPlant(front.column, front.row));
  REQUIRE(m.tryWater(front.column, front.row));
  m.advanceDay();
  REQUIRE_STAGE(m, 1, 2, CropStage::SEED); // 1 of 3 days
  waterDays(m, front.column, front.row, kDaysToSprout - 1);
  REQUIRE_STAGE(m, 1, 2, CropStage::SPROUT);

  GameModel dry; // front tile is plain ground: nothing to water
  const TilePosition bare = dry.tileInFront();
  REQUIRE(!dry.tryWater(bare.column, bare.row));
}

TEST_CASE("watering_eachModelHasItsOwnState") {
  GameModel a;
  GameModel b;
  tillAndPlant(a, 3, 2);
  tillAndPlant(b, 3, 2);
  waterDays(a, 3, 2, kDaysToSprout);
  REQUIRE_STAGE(a, 3, 2, CropStage::SPROUT);
  REQUIRE_STAGE(b, 3, 2, CropStage::SEED);
  REQUIRE(!b.isWateredToday(3, 2));
  REQUIRE(b.tryWater(3, 2)); // b has not been watered yet today
}

// ---------------------------------------------------------------------------
// Harvesting
// A MATURE crop can be harvested: the crop is removed (stage NONE, growth and
// watering state gone), the soil stays tilled and walkable, and the produce
// counter goes up by one. Anything else is refused and changes nothing.
// ---------------------------------------------------------------------------

// Plant on fresh soil and water it until it is mature.
static void growToMature(GameModel &m, int column, int row) {
  tillAndPlant(m, column, row);
  waterDays(m, column, row, kDaysToMature);
  REQUIRE_STAGE(m, column, row, CropStage::MATURE);
}

TEST_CASE("harvest_produceCountStartsAtZero") {
  GameModel m;
  REQUIRE(m.produceCount() == 0);
  tillAndPlant(m, 3, 2);
  m.advanceDay();
  REQUIRE(m.produceCount() == 0); // planting and growing alone collect nothing
}

TEST_CASE("harvest_matureCropSucceedsAndCountsOne") {
  GameModel m;
  growToMature(m, 3, 2);
  REQUIRE(m.tryHarvest(3, 2));
  REQUIRE(m.produceCount() == 1);
  REQUIRE_STAGE(m, 3, 2, CropStage::NONE);
  REQUIRE(!m.isPlanted(3, 2));
}

TEST_CASE("harvest_keepsTheSoilTilledAndTheTerrainGround") {
  GameModel m;
  growToMature(m, 3, 2);
  REQUIRE(m.tryHarvest(3, 2));
  REQUIRE(m.isTilled(3, 2));
  REQUIRE_TILE(m, 3, 2, TileType::GROUND);
}

// The boundary matters: five watered days is still a sprout, six is mature.
TEST_CASE("harvest_seedAndSproutAreRefusedUntilTheSixthWateredDay") {
  GameModel m;
  tillAndPlant(m, 3, 2);
  REQUIRE(!m.tryHarvest(3, 2)); // seed
  REQUIRE_STAGE(m, 3, 2, CropStage::SEED);

  waterDays(m, 3, 2, kDaysToSprout);
  REQUIRE_STAGE(m, 3, 2, CropStage::SPROUT);
  REQUIRE(!m.tryHarvest(3, 2)); // sprout

  waterDays(m, 3, 2, kDaysToMature - kDaysToSprout - 1);
  REQUIRE_STAGE(m, 3, 2, CropStage::SPROUT); // five watered days
  REQUIRE(!m.tryHarvest(3, 2));
  REQUIRE(m.produceCount() == 0);
  REQUIRE(m.isPlanted(3, 2)); // refused harvests leave the crop alone

  waterAndAdvance(m, 3, 2); // sixth watered day
  REQUIRE_STAGE(m, 3, 2, CropStage::MATURE);
  REQUIRE(m.tryHarvest(3, 2));
  REQUIRE(m.produceCount() == 1);
}

TEST_CASE("harvest_soilWithoutACropIsRefused") {
  GameModel m;
  REQUIRE(m.tryTill(3, 2)); // tilled but empty
  REQUIRE(!m.tryHarvest(3, 2));
  REQUIRE(!m.tryHarvest(6, 2)); // ordinary untilled ground
  REQUIRE(m.produceCount() == 0);
  REQUIRE(m.isTilled(3, 2));
}

TEST_CASE("harvest_everyObstacleIsRefused") {
  GameModel m;
  int obstaclesTried = 0;
  for (int row = 0; row < m.mapHeight(); ++row) {
    for (int column = 0; column < m.mapWidth(); ++column) {
      if (m.tileAt(column, row) == TileType::OBSTACLE) {
        REQUIRE(!m.tryHarvest(column, row));
        ++obstaclesTried;
      }
    }
  }
  REQUIRE(obstaclesTried == 23);
  REQUIRE(m.produceCount() == 0);
}

TEST_CASE("harvest_outOfBoundsIsRefused") {
  GameModel m;
  REQUIRE(!m.tryHarvest(-1, 0));
  REQUIRE(!m.tryHarvest(0, -1));
  REQUIRE(!m.tryHarvest(-1, -1));
  REQUIRE(!m.tryHarvest(GameModel::kMapWidth, 0));
  REQUIRE(!m.tryHarvest(0, GameModel::kMapHeight));
  REQUIRE(!m.tryHarvest(GameModel::kMapWidth, GameModel::kMapHeight));
  REQUIRE(m.produceCount() == 0);
}

// Flat-array trap: (20,0) is index 20 = tile (0,1); (-1,1) is index 19 = tile
// (19,0). Both hold MATURE crops here, so a missing bounds check would harvest
// them instead of rejecting the off-map attempt.
TEST_CASE("harvest_outOfBoundsNeverTouchesAnotherCrop") {
  GameModel m;
  growToMature(m, 0, 1);
  growToMature(m, 19, 0);
  REQUIRE(!m.tryHarvest(GameModel::kMapWidth, 0));
  REQUIRE(!m.tryHarvest(-1, 1));
  REQUIRE_STAGE(m, 0, 1, CropStage::MATURE);
  REQUIRE_STAGE(m, 19, 0, CropStage::MATURE);
  REQUIRE(m.produceCount() == 0);
}

TEST_CASE("harvest_sameTileCannotBeHarvestedTwice") {
  GameModel m;
  growToMature(m, 3, 2);
  REQUIRE(m.tryHarvest(3, 2));
  REQUIRE(!m.tryHarvest(3, 2));
  REQUIRE(!m.tryHarvest(3, 2));
  REQUIRE(m.produceCount() == 1); // one crop, one unit
}

TEST_CASE("harvest_failedAttemptsDoNotChangeState") {
  GameModel m;
  growToMature(m, 3, 2);
  tillAndPlant(m, 4, 2);                           // a seed beside it
  REQUIRE(!m.tryHarvest(4, 2));                    // seed
  REQUIRE(!m.tryHarvest(6, 2));                    // untilled ground
  REQUIRE(!m.tryHarvest(5, 1));                    // obstacle
  REQUIRE(!m.tryHarvest(-1, 0));                   // off the map
  REQUIRE(!m.tryHarvest(GameModel::kMapWidth, 0)); // off the map
  REQUIRE(m.produceCount() == 0);
  REQUIRE_STAGE(m, 3, 2, CropStage::MATURE);
  REQUIRE_STAGE(m, 4, 2, CropStage::SEED);
}

TEST_CASE("harvest_countAccumulatesAcrossCrops") {
  GameModel m;
  tillAndPlant(m, 3, 2);
  tillAndPlant(m, 4, 2);
  tillAndPlant(m, 3, 3);
  for (int day = 0; day < kDaysToMature; ++day) {
    REQUIRE(m.tryWater(3, 2));
    REQUIRE(m.tryWater(4, 2));
    REQUIRE(m.tryWater(3, 3));
    m.advanceDay();
  }
  REQUIRE(m.tryHarvest(3, 2));
  REQUIRE(m.produceCount() == 1);
  REQUIRE(m.tryHarvest(4, 2));
  REQUIRE(m.produceCount() == 2);
  REQUIRE(m.tryHarvest(3, 3));
  REQUIRE(m.produceCount() == 3);
}

TEST_CASE("harvest_onlyTheTargetedCropIsRemoved") {
  GameModel m;
  tillAndPlant(m, 3, 2);
  tillAndPlant(m, 4, 2);
  for (int day = 0; day < kDaysToMature; ++day) {
    REQUIRE(m.tryWater(3, 2));
    REQUIRE(m.tryWater(4, 2));
    m.advanceDay();
  }
  REQUIRE(m.tryHarvest(3, 2));
  REQUIRE_STAGE(m, 3, 2, CropStage::NONE);
  REQUIRE_STAGE(m, 4, 2, CropStage::MATURE); // the neighbour is untouched
  REQUIRE(countPlanted(m) == 1);
  REQUIRE(!m.isPlanted(2, 3)); // what a swapped (column,row) index would hit
  REQUIRE(m.produceCount() == 1);
}

TEST_CASE("harvest_emptiedTileCannotBeWateredOrHarvested") {
  GameModel m;
  growToMature(m, 3, 2);
  REQUIRE(m.tryHarvest(3, 2));
  REQUIRE(!m.tryWater(3, 2));
  REQUIRE(!m.isWateredToday(3, 2));
  REQUIRE(!m.tryHarvest(3, 2));
  REQUIRE(m.produceCount() == 1);
}

// Removing the crop must also remove its "watered today" state.
TEST_CASE("harvest_removedCropLeavesNoWateredStateBehind") {
  GameModel m;
  growToMature(m, 3, 2);
  m.tryWater(3, 2); // watered today (result not asserted: see growth tests)
  REQUIRE(m.tryHarvest(3, 2));
  REQUIRE(!m.isWateredToday(3, 2));
  REQUIRE(m.tryPlant(3, 2)); // soil is still tilled
  REQUIRE(!m.isWateredToday(3, 2));
  REQUIRE(m.tryWater(3, 2)); // the NEW crop is dry and can be watered
}

TEST_CASE("harvest_replantedCropStartsAsAFreshSeed") {
  GameModel m;
  growToMature(m, 3, 2);
  REQUIRE(m.tryHarvest(3, 2));
  REQUIRE(m.tryPlant(3, 2)); // no tryTill needed: the soil stayed tilled
  REQUIRE_STAGE(m, 3, 2, CropStage::SEED);

  waterDays(m, 3, 2, kDaysToSprout - 1);
  REQUIRE_STAGE(m, 3, 2, CropStage::SEED);
  waterAndAdvance(m, 3, 2);
  REQUIRE_STAGE(m, 3, 2, CropStage::SPROUT);
  waterDays(m, 3, 2, kDaysToMature - kDaysToSprout - 1);
  REQUIRE_STAGE(m, 3, 2, CropStage::SPROUT);
  waterAndAdvance(m, 3, 2);
  REQUIRE_STAGE(m, 3, 2, CropStage::MATURE); // full 3 + 3 days again

  REQUIRE(m.tryHarvest(3, 2));
  REQUIRE(m.produceCount() == 2);
}

TEST_CASE("harvest_playerCanWalkOverHarvestedSoil") {
  GameModel m;
  growToMature(m, 2, 1);
  growToMature(m, 3, 1);
  growToMature(m, 4, 1);
  REQUIRE(m.tryHarvest(2, 1));
  REQUIRE(m.tryHarvest(3, 1));
  REQUIRE(m.tryHarvest(4, 1));
  MOVE(m, Direction::RIGHT, 3); // (4,1), over three emptied tiles
  REQUIRE_POS(m, 4, 1);
  MOVE(m, Direction::LEFT, 3);
  REQUIRE_POS(m, 1, 1);
  REQUIRE(m.produceCount() == 3);
  REQUIRE(m.isTilled(3, 1)); // walking does not undo the soil
}

TEST_CASE("harvest_doesNotMoveOrTurnThePlayer") {
  GameModel m;
  growToMature(m, 3, 2);
  MOVE(m, Direction::RIGHT, 1); // (2,1), facing RIGHT
  REQUIRE(m.tryHarvest(3, 2));
  REQUIRE_POS(m, 2, 1);
  REQUIRE_FACING(m, Direction::RIGHT);
}

TEST_CASE("harvest_advanceDayDoesNotChangeTheProduceCount") {
  GameModel m;
  growToMature(m, 3, 2);
  REQUIRE(m.tryHarvest(3, 2));
  advanceDays(m, 5);
  REQUIRE(m.produceCount() == 1);
  REQUIRE_STAGE(m, 3, 2, CropStage::NONE); // no crop regrows by itself
}

// The way the SDL layer will use it: target = tileInFront().
TEST_CASE("harvest_worksWithTileInFront") {
  GameModel m; // (1,1), facing DOWN
  const TilePosition front = m.tileInFront();
  tillAndPlant(m, front.column, front.row);
  waterDays(m, front.column, front.row, kDaysToMature);
  REQUIRE(m.tryHarvest(front.column, front.row));
  REQUIRE(m.produceCount() == 1);
  REQUIRE(m.isTilled(1, 2));

  GameModel bare; // front tile is plain ground: nothing to harvest
  const TilePosition target = bare.tileInFront();
  REQUIRE(!bare.tryHarvest(target.column, target.row));
  REQUIRE(bare.produceCount() == 0);
}

TEST_CASE("harvest_eachModelHasItsOwnCount") {
  GameModel a;
  GameModel b;
  growToMature(a, 3, 2);
  REQUIRE(a.tryHarvest(3, 2));
  REQUIRE(a.produceCount() == 1);
  REQUIRE(b.produceCount() == 0);
}

// ---------------------------------------------------------------------------
