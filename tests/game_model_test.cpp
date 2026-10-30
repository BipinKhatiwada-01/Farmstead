// tests/game_model_checks.cpp
//
// Behaviour checks for farmstead::GameModel. No SDL, no test framework.
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

#include "../include/farmstead/game_model.hpp"

#include <cstdint>
#include <cstdlib>
#include <iostream>

using namespace farmstead;

// ---------------------------------------------------------------------------
// Tiny test helpers
// ---------------------------------------------------------------------------

static int g_failures = 0;

#define CHECK(cond)                                                            \
  do {                                                                         \
    if (!(cond)) {                                                             \
      std::cerr << "  FAIL line " << __LINE__ << ": " #cond "\n";              \
      ++g_failures;                                                            \
    }                                                                          \
  } while (0)

static void checkPositionImpl(const GameModel &m, int column, int row,
                              int line) {
  const TilePosition p = m.playerPosition();
  if (p.column != column || p.row != row) {
    std::cerr << "  FAIL line " << line << ": expected player at (" << column
              << "," << row << ") but was at (" << p.column << "," << p.row
              << ")\n";
    ++g_failures;
  }
}
#define CHECK_POS(model, column, row)                                          \
  checkPositionImpl((model), (column), (row), __LINE__)

static const char *tileName(TileType t) {
  return t == TileType::GROUND ? "GROUND" : "OBSTACLE";
}

static void checkTileImpl(const GameModel &m, int column, int row,
                          TileType expected, int line) {
  const TileType actual = m.tileAt(column, row);
  if (actual != expected) {
    std::cerr << "  FAIL line " << line << ": tile (" << column << "," << row
              << ") expected " << tileName(expected) << " but was "
              << tileName(actual) << "\n";
    ++g_failures;
  }
}
#define CHECK_TILE(model, column, row, expected)                               \
  checkTileImpl((model), (column), (row), (expected), __LINE__)

// Moves exactly `steps` times. Every step MUST succeed; if one is blocked the
// failure is reported at the caller's line, with the position where it stopped.
static void moveExactlyImpl(GameModel &m, Direction d, int steps, int line) {
  for (int i = 0; i < steps; ++i) {
    if (!m.tryMove(d)) {
      std::cerr << "  FAIL line " << line << ": step " << (i + 1) << " of "
                << steps << " was blocked at (" << m.playerPosition().column
                << "," << m.playerPosition().row << ")\n";
      ++g_failures;
      return;
    }
  }
}
#define MOVE(model, dir, steps)                                                \
  moveExactlyImpl((model), (dir), (steps), __LINE__)

// ---------------------------------------------------------------------------
// Map dimensions
// ---------------------------------------------------------------------------

static void mapDimensionsAre20By15() {
  GameModel m;
  CHECK(m.mapWidth() == 20);
  CHECK(m.mapHeight() == 15);
  CHECK(m.mapWidth() == GameModel::kMapWidth);
  CHECK(m.mapHeight() == GameModel::kMapHeight);
}

// ---------------------------------------------------------------------------
// isInBounds
// ---------------------------------------------------------------------------

static void isInBounds_cornersAreInside() {
  GameModel m;
  CHECK(m.isInBounds(0, 0));
  CHECK(m.isInBounds(19, 0));
  CHECK(m.isInBounds(0, 14));
  CHECK(m.isInBounds(19, 14));
}

static void isInBounds_negativeCoordinatesAreOutside() {
  GameModel m;
  CHECK(!m.isInBounds(-1, 0));
  CHECK(!m.isInBounds(0, -1));
  CHECK(!m.isInBounds(-1, -1));
  CHECK(!m.isInBounds(-100, 5));
}

static void isInBounds_pastTheFarEdgeIsOutside() {
  GameModel m;
  CHECK(!m.isInBounds(20, 0));
  CHECK(!m.isInBounds(0, 15));
  CHECK(!m.isInBounds(20, 15));
  CHECK(!m.isInBounds(100, 5));
}

// ---------------------------------------------------------------------------
// tileAt
// ---------------------------------------------------------------------------

static void tileAt_openGroundIsGround() {
  GameModel m;
  CHECK_TILE(m, 0, 0, TileType::GROUND);
  CHECK_TILE(m, 1, 1, TileType::GROUND);
  CHECK_TILE(m, 10, 5, TileType::GROUND);
  CHECK_TILE(m, 19, 14, TileType::GROUND);
}

static void tileAt_wallTilesAreObstacles() {
  GameModel m;
  const TilePosition walls[] = {
      {5, 1},  {6, 1},   {7, 1},   {5, 2},   {5, 3},   {13, 3},
      {14, 3}, {12, 4},  {13, 4},  {2, 6},   {3, 6},   {2, 7},
      {3, 7},  {13, 7},  {14, 8},  {6, 10},  {7, 10},  {8, 10},
      {9, 10}, {10, 11}, {10, 12}, {16, 12}, {17, 12},
  };
  for (const TilePosition &w : walls) {
    CHECK_TILE(m, w.column, w.row, TileType::OBSTACLE);
  }
}

static void tileAt_tilesBesideWallsAreGround() {
  GameModel m;
  CHECK_TILE(m, 4, 1, TileType::GROUND);  // left of the row-1 wall
  CHECK_TILE(m, 8, 1, TileType::GROUND);  // right of the row-1 wall
  CHECK_TILE(m, 6, 2, TileType::GROUND);  // inside the "C" shape
  CHECK_TILE(m, 14, 7, TileType::GROUND); // above isolated wall (14,8)
  CHECK_TILE(m, 14, 9, TileType::GROUND); // below it
  CHECK_TILE(m, 13, 8, TileType::GROUND); // left of it
  CHECK_TILE(m, 15, 8, TileType::GROUND); // right of it
}

// Together with the wall list above, this proves there are no EXTRA obstacles.
static void tileAt_totalObstacleCountIs23() {
  GameModel m;
  int obstacles = 0;
  for (int row = 0; row < m.mapHeight(); ++row) {
    for (int column = 0; column < m.mapWidth(); ++column) {
      if (m.tileAt(column, row) == TileType::OBSTACLE) {
        ++obstacles;
      }
    }
  }
  CHECK(obstacles == 23);
}

static void tileAt_outOfBoundsIsObstacle() {
  GameModel m;
  CHECK_TILE(m, -1, 0, TileType::OBSTACLE);
  CHECK_TILE(m, 0, -1, TileType::OBSTACLE);
  CHECK_TILE(m, -1, -1, TileType::OBSTACLE);
  CHECK_TILE(m, GameModel::kMapWidth, 0, TileType::OBSTACLE);
  CHECK_TILE(m, 0, GameModel::kMapHeight, TileType::OBSTACLE);
  CHECK_TILE(m, GameModel::kMapWidth, GameModel::kMapHeight,
             TileType::OBSTACLE);
}

// The edge tests below rely on the border being walkable, otherwise the
// "map edge" rule could never be reached. This documents that on purpose.
static void tileAt_borderIsAllGround() {
  GameModel m;
  for (int column = 0; column < GameModel::kMapWidth; ++column) {
    CHECK_TILE(m, column, 0, TileType::GROUND);
    CHECK_TILE(m, column, GameModel::kMapHeight - 1, TileType::GROUND);
  }
  for (int row = 0; row < GameModel::kMapHeight; ++row) {
    CHECK_TILE(m, 0, row, TileType::GROUND);
    CHECK_TILE(m, GameModel::kMapWidth - 1, row, TileType::GROUND);
  }
}

// ---------------------------------------------------------------------------
// Start state
// ---------------------------------------------------------------------------

static void start_positionIsColumn1Row1() {
  GameModel m;
  CHECK_POS(m, 1, 1);
}

static void start_isInBoundsAndOnGround() {
  GameModel m;
  const TilePosition p = m.playerPosition();
  CHECK(m.isInBounds(p.column, p.row));
  CHECK_TILE(m, p.column, p.row, TileType::GROUND);
}

static void start_eachModelHasItsOwnState() {
  GameModel a;
  GameModel b;
  CHECK(a.tryMove(Direction::RIGHT));
  CHECK_POS(a, 2, 1);
  CHECK_POS(b, 1, 1); // b must not be affected by a
}

// ---------------------------------------------------------------------------
// tryMove: direction meaning (one test per direction, so a swapped axis is
// reported against exactly the direction that is wrong)
// ---------------------------------------------------------------------------

static void move_upDecreasesRowByOne() {
  GameModel m;
  CHECK(m.tryMove(Direction::UP));
  CHECK_POS(m, 1, 0);
}

static void move_downIncreasesRowByOne() {
  GameModel m;
  CHECK(m.tryMove(Direction::DOWN));
  CHECK_POS(m, 1, 2);
}

static void move_leftDecreasesColumnByOne() {
  GameModel m;
  CHECK(m.tryMove(Direction::LEFT));
  CHECK_POS(m, 0, 1);
}

static void move_rightIncreasesColumnByOne() {
  GameModel m;
  CHECK(m.tryMove(Direction::RIGHT));
  CHECK_POS(m, 2, 1);
}

static void move_oppositeMovesCancelOut() {
  GameModel m;
  CHECK(m.tryMove(Direction::RIGHT));
  CHECK(m.tryMove(Direction::LEFT));
  CHECK_POS(m, 1, 1);
  CHECK(m.tryMove(Direction::DOWN));
  CHECK(m.tryMove(Direction::UP));
  CHECK_POS(m, 1, 1);
}

static void move_squarePathReturnsToStart() {
  GameModel m;
  CHECK(m.tryMove(Direction::RIGHT)); // (2,1)
  CHECK(m.tryMove(Direction::DOWN));  // (2,2)
  CHECK(m.tryMove(Direction::LEFT));  // (1,2)
  CHECK(m.tryMove(Direction::UP));    // (1,1)
  CHECK_POS(m, 1, 1);
}

static void move_longWalkAcrossOpenGroundWorks() {
  GameModel m;
  MOVE(m, Direction::DOWN, 4);   // (1,5)   column 1 is clear
  MOVE(m, Direction::RIGHT, 18); // (19,5)  row 5 is clear
  CHECK_POS(m, 19, 5);
}

// ---------------------------------------------------------------------------
// tryMove: obstacles
// ---------------------------------------------------------------------------

static void obstacle_blocksMovingRight() {
  GameModel m;
  MOVE(m, Direction::RIGHT, 3); // (4,1); next tile (5,1) is a wall
  CHECK_POS(m, 4, 1);
  CHECK(!m.tryMove(Direction::RIGHT));
  CHECK_POS(m, 4, 1); // unchanged
}

static void obstacle_blocksMovingDown() {
  GameModel m;
  MOVE(m, Direction::DOWN, 4);   // (1,5)
  MOVE(m, Direction::RIGHT, 13); // (14,5)
  MOVE(m, Direction::DOWN, 2);   // (14,7); next tile (14,8) is a wall
  CHECK_POS(m, 14, 7);
  CHECK(!m.tryMove(Direction::DOWN));
  CHECK_POS(m, 14, 7);
}

static void obstacle_blocksMovingUp() {
  GameModel m;
  MOVE(m, Direction::DOWN, 8);   // (1,9)
  MOVE(m, Direction::RIGHT, 13); // (14,9); tile above (14,8) is a wall
  CHECK_POS(m, 14, 9);
  CHECK(!m.tryMove(Direction::UP));
  CHECK_POS(m, 14, 9);
}

static void obstacle_blocksMovingLeft() {
  GameModel m;
  MOVE(m, Direction::DOWN, 8);   // (1,9)
  MOVE(m, Direction::RIGHT, 14); // (15,9)
  MOVE(m, Direction::UP, 1);     // (15,8); tile to the left (14,8) is a wall
  CHECK_POS(m, 15, 8);
  CHECK(!m.tryMove(Direction::LEFT));
  CHECK_POS(m, 15, 8);
}

static void obstacle_blockedFromTheLeftSide() {
  GameModel m;
  MOVE(m, Direction::DOWN, 8);   // (1,9)
  MOVE(m, Direction::RIGHT, 12); // (13,9)
  MOVE(m, Direction::UP, 1);     // (13,8); tile to the right (14,8) is a wall
  CHECK_POS(m, 13, 8);
  CHECK(!m.tryMove(Direction::RIGHT));
  CHECK_POS(m, 13, 8);
}

static void obstacle_repeatedBlockedMovesNeverChangePosition() {
  GameModel m;
  MOVE(m, Direction::RIGHT, 3); // (4,1)
  for (int i = 0; i < 5; ++i) {
    CHECK(!m.tryMove(Direction::RIGHT));
    CHECK_POS(m, 4, 1);
  }
}

static void obstacle_canMoveAwayAfterBeingBlocked() {
  GameModel m;
  MOVE(m, Direction::RIGHT, 3); // (4,1)
  CHECK(!m.tryMove(Direction::RIGHT));
  CHECK(m.tryMove(Direction::DOWN)); // a blocked move must not "stick"
  CHECK_POS(m, 4, 2);
}

// ---------------------------------------------------------------------------
// tryMove: map edges
// ---------------------------------------------------------------------------

static void edge_topBlocksMovingUp() {
  GameModel m;
  MOVE(m, Direction::UP, 1); // (1,0)
  CHECK_POS(m, 1, 0);
  CHECK(!m.tryMove(Direction::UP)); // would be row -1
  CHECK_POS(m, 1, 0);
}

static void edge_leftBlocksMovingLeft() {
  GameModel m;
  MOVE(m, Direction::LEFT, 1); // (0,1)
  CHECK_POS(m, 0, 1);
  CHECK(!m.tryMove(Direction::LEFT)); // would be column -1
  CHECK_POS(m, 0, 1);
}

static void edge_bottomBlocksMovingDown() {
  GameModel m;
  MOVE(m, Direction::DOWN, 13); // (1,14); column 1 is clear
  CHECK_POS(m, 1, 14);
  CHECK(!m.tryMove(Direction::DOWN)); // would be row 15
  CHECK_POS(m, 1, 14);
}

static void edge_rightBlocksMovingRight() {
  GameModel m;
  MOVE(m, Direction::UP, 1);     // (1,0)
  MOVE(m, Direction::RIGHT, 18); // (19,0); row 0 is clear
  CHECK_POS(m, 19, 0);
  CHECK(!m.tryMove(Direction::RIGHT)); // would be column 20
  CHECK_POS(m, 19, 0);
}

static void corner_topLeftBlocksUpAndLeft() {
  GameModel m;
  MOVE(m, Direction::UP, 1);
  MOVE(m, Direction::LEFT, 1);
  CHECK_POS(m, 0, 0);
  CHECK(!m.tryMove(Direction::UP));
  CHECK(!m.tryMove(Direction::LEFT));
  CHECK_POS(m, 0, 0);
}

static void corner_bottomRightBlocksDownAndRight() {
  GameModel m;
  MOVE(m, Direction::UP, 1);     // (1,0)
  MOVE(m, Direction::RIGHT, 18); // (19,0)
  MOVE(m, Direction::DOWN, 14);  // (19,14); column 19 is clear
  CHECK_POS(m, 19, 14);
  CHECK(!m.tryMove(Direction::DOWN));
  CHECK(!m.tryMove(Direction::RIGHT));
  CHECK_POS(m, 19, 14);
}

// ---------------------------------------------------------------------------
// Property check: does not depend on the layout at all.
// 20,000 pseudo-random moves (fixed seed, so it is repeatable). After EVERY
// move: the player is in bounds and on ground; a successful move changed the
// position by exactly one tile; a rejected move changed nothing.
// ---------------------------------------------------------------------------

static void randomWalk_alwaysObeysTheRules() {
  GameModel m;
  const Direction directions[4] = {Direction::UP, Direction::DOWN,
                                   Direction::LEFT, Direction::RIGHT};
  std::uint32_t state = 12345u;
  const int failuresBefore = g_failures;

  for (int i = 0; i < 20000; ++i) {
    state = state * 1664525u + 1013904223u; // simple LCG
    const Direction d = directions[(state >> 16) % 4];

    const TilePosition before = m.playerPosition();
    const bool moved = m.tryMove(d);
    const TilePosition after = m.playerPosition();

    const int distance = std::abs(after.column - before.column) +
                         std::abs(after.row - before.row);

    CHECK(m.isInBounds(after.column, after.row));
    CHECK(m.tileAt(after.column, after.row) == TileType::GROUND);
    CHECK(distance == (moved ? 1 : 0));

    if (g_failures != failuresBefore) {
      std::cerr << "  (stopped after step " << i + 1 << ")\n";
      return; // do not print 20,000 failures
    }
  }
}

// ---------------------------------------------------------------------------
// Facing direction
// The player starts facing DOWN. EVERY tryMove() call turns the player to the
// requested direction, whether the move succeeds or is blocked. An invalid
// Direction value changes nothing.
// ---------------------------------------------------------------------------

static const char *directionName(Direction d) {
  switch (d) {
  case Direction::UP:
    return "UP";
  case Direction::DOWN:
    return "DOWN";
  case Direction::LEFT:
    return "LEFT";
  case Direction::RIGHT:
    return "RIGHT";
  }
  return "?";
}

static void checkFacingImpl(const GameModel &m, Direction expected, int line) {
  const Direction actual = m.facingDirection();
  if (actual != expected) {
    std::cerr << "  FAIL line " << line << ": expected facing "
              << directionName(expected) << " but was " << directionName(actual)
              << "\n";
    ++g_failures;
  }
}
#define CHECK_FACING(model, expected)                                          \
  checkFacingImpl((model), (expected), __LINE__)

static void facing_startsDown() {
  GameModel m;
  CHECK_FACING(m, Direction::DOWN);
}

// Successful moves. Each one checks position too, so we know it really moved.

static void facing_successfulMoveUpTurnsUp() {
  GameModel m;
  CHECK(m.tryMove(Direction::UP));
  CHECK_POS(m, 1, 0);
  CHECK_FACING(m, Direction::UP);
}

static void facing_successfulMoveLeftTurnsLeft() {
  GameModel m;
  CHECK(m.tryMove(Direction::LEFT));
  CHECK_POS(m, 0, 1);
  CHECK_FACING(m, Direction::LEFT);
}

static void facing_successfulMoveRightTurnsRight() {
  GameModel m;
  CHECK(m.tryMove(Direction::RIGHT));
  CHECK_POS(m, 2, 1);
  CHECK_FACING(m, Direction::RIGHT);
}

// The default is already DOWN, so turn away first or this proves nothing.
static void facing_successfulMoveDownTurnsDown() {
  GameModel m;
  CHECK(m.tryMove(Direction::RIGHT)); // (2,1), facing RIGHT
  CHECK(m.tryMove(Direction::DOWN));  // (2,2)
  CHECK_POS(m, 2, 2);
  CHECK_FACING(m, Direction::DOWN);
}

static void facing_followsTheLastRequestedDirection() {
  GameModel m;
  CHECK(m.tryMove(Direction::RIGHT));
  CHECK(m.tryMove(Direction::DOWN));
  CHECK(m.tryMove(Direction::LEFT));
  CHECK(m.tryMove(Direction::UP));
  CHECK_POS(m, 1, 1);
  CHECK_FACING(m, Direction::UP);
}

// Blocked moves. Each one starts facing a DIFFERENT way than the blocked
// request, so the turn is visible.

static void facing_blockedByObstacleStillTurns() {
  GameModel m;
  MOVE(m, Direction::DOWN, 8);   // (1,9)
  MOVE(m, Direction::RIGHT, 12); // (13,9)
  MOVE(m, Direction::UP, 1); // (13,8), facing UP; wall at (14,8) to the right
  CHECK_POS(m, 13, 8);
  CHECK_FACING(m, Direction::UP);

  CHECK(!m.tryMove(Direction::RIGHT));
  CHECK_POS(m, 13, 8);               // did not move
  CHECK_FACING(m, Direction::RIGHT); // but turned
}

static void facing_blockedFromBelowStillTurnsUp() {
  GameModel m;
  MOVE(m, Direction::DOWN, 8);   // (1,9)
  MOVE(m, Direction::RIGHT, 13); // (14,9), facing RIGHT; wall at (14,8) above
  CHECK_FACING(m, Direction::RIGHT);

  CHECK(!m.tryMove(Direction::UP));
  CHECK_POS(m, 14, 9);
  CHECK_FACING(m, Direction::UP);
}

static void facing_blockedByMapEdgeStillTurns() {
  GameModel m;
  MOVE(m, Direction::UP, 1);   // (1,0)
  MOVE(m, Direction::LEFT, 1); // (0,0), facing LEFT, in the top-left corner
  CHECK_FACING(m, Direction::LEFT);

  CHECK(!m.tryMove(Direction::UP)); // off the top of the map
  CHECK_POS(m, 0, 0);
  CHECK_FACING(m, Direction::UP);
}

// (13,8) has walls above (13,7) and to the right (14,8): two different blocked
// directions from one tile, so facing must follow each attempt.
static void facing_followsEachBlockedAttemptOnTheSameTile() {
  GameModel m;
  MOVE(m, Direction::DOWN, 8);   // (1,9)
  MOVE(m, Direction::RIGHT, 12); // (13,9)
  MOVE(m, Direction::UP, 1);     // (13,8)

  CHECK(!m.tryMove(Direction::UP));
  CHECK_FACING(m, Direction::UP);
  CHECK(!m.tryMove(Direction::RIGHT));
  CHECK_FACING(m, Direction::RIGHT);
  CHECK(!m.tryMove(Direction::UP));
  CHECK_FACING(m, Direction::UP);
  CHECK_POS(m, 13, 8);
}

// Invalid / separate state

static void facing_invalidDirectionIsIgnored() {
  GameModel m;
  CHECK(!m.tryMove(static_cast<Direction>(99)));
  CHECK_FACING(m, Direction::DOWN); // untouched at the start

  CHECK(m.tryMove(Direction::RIGHT));
  CHECK(!m.tryMove(static_cast<Direction>(99)));
  CHECK_FACING(m, Direction::RIGHT); // untouched after a real move
  CHECK_POS(m, 2, 1);
}

static void facing_eachModelHasItsOwnFacing() {
  GameModel a;
  GameModel b;
  CHECK(a.tryMove(Direction::RIGHT));
  CHECK_FACING(a, Direction::RIGHT);
  CHECK_FACING(b, Direction::DOWN); // b must not be affected by a
}

// ---------------------------------------------------------------------------
// tileInFront
// The tile the player faces = playerPosition() moved one step in
// facingDirection(). It only IDENTIFIES a target: no walkability check, no
// clamping (it may be off the map), and it never changes position or facing.
// ---------------------------------------------------------------------------

static void checkFrontImpl(const GameModel &m, int column, int row, int line) {
  const TilePosition f = m.tileInFront();
  if (f.column != column || f.row != row) {
    std::cerr << "  FAIL line " << line << ": expected tile in front ("
              << column << "," << row << ") but was (" << f.column << ","
              << f.row << ")\n";
    ++g_failures;
  }
}
#define CHECK_FRONT(model, column, row)                                        \
  checkFrontImpl((model), (column), (row), __LINE__)

// Each direction. Every test checks facing first, so a wrong result is not
// confused with the player simply not facing where the test thinks it is.

static void tileInFront_atStartIsTheTileBelow() {
  GameModel m; // (1,1), facing DOWN
  CHECK_FRONT(m, 1, 2);
}

static void tileInFront_facingUpIsRowMinusOne() {
  GameModel m;
  MOVE(m, Direction::DOWN, 4); // (1,5)
  MOVE(m, Direction::UP, 1);   // (1,4), facing UP
  CHECK_FACING(m, Direction::UP);
  CHECK_FRONT(m, 1, 3);
}

static void tileInFront_facingDownIsRowPlusOne() {
  GameModel m;
  MOVE(m, Direction::RIGHT, 1); // (2,1): turn away from the default first
  MOVE(m, Direction::DOWN, 1);  // (2,2), facing DOWN
  CHECK_FACING(m, Direction::DOWN);
  CHECK_FRONT(m, 2, 3);
}

static void tileInFront_facingLeftIsColumnMinusOne() {
  GameModel m;
  MOVE(m, Direction::RIGHT, 1); // (2,1)
  MOVE(m, Direction::LEFT, 1);  // (1,1), facing LEFT
  CHECK_FACING(m, Direction::LEFT);
  CHECK_FRONT(m, 0, 1);
}

static void tileInFront_facingRightIsColumnPlusOne() {
  GameModel m;
  MOVE(m, Direction::RIGHT, 1); // (2,1), facing RIGHT
  CHECK_FACING(m, Direction::RIGHT);
  CHECK_FRONT(m, 3, 1);
}

// Off the map: returned as-is, never clamped.

static void tileInFront_pastTopEdgeIsReturnedUnclamped() {
  GameModel m;
  MOVE(m, Direction::UP, 1); // (1,0), facing UP
  CHECK_FRONT(m, 1, -1);
  const TilePosition front = m.tileInFront();
  CHECK(!m.isInBounds(front.column, front.row));
}

static void tileInFront_pastLeftEdgeIsReturnedUnclamped() {
  GameModel m;
  MOVE(m, Direction::LEFT, 1); // (0,1), facing LEFT
  CHECK_FRONT(m, -1, 1);
  const TilePosition front = m.tileInFront();
  CHECK(!m.isInBounds(front.column, front.row));
}

static void tileInFront_pastBottomEdgeIsReturnedUnclamped() {
  GameModel m;
  MOVE(m, Direction::DOWN, 13); // (1,14), facing DOWN
  CHECK_FRONT(m, 1, GameModel::kMapHeight);
  const TilePosition front = m.tileInFront();
  CHECK(!m.isInBounds(front.column, front.row));
}

static void tileInFront_pastRightEdgeIsReturnedUnclamped() {
  GameModel m;
  MOVE(m, Direction::UP, 1);     // (1,0)
  MOVE(m, Direction::RIGHT, 18); // (19,0), facing RIGHT
  CHECK_FRONT(m, GameModel::kMapWidth, 0);
  const TilePosition front = m.tileInFront();
  CHECK(!m.isInBounds(front.column, front.row));
}

// No walkability check: a wall in front is still "the tile in front".
static void tileInFront_returnsAnObstacleTileWithoutJudgingIt() {
  GameModel m;
  MOVE(m, Direction::RIGHT, 3); // (4,1), facing RIGHT; (5,1) is a wall
  CHECK_FRONT(m, 5, 1);
  CHECK_TILE(m, 5, 1, TileType::OBSTACLE);
}

// Read-only: no side effects, and callable on a const model.
static void tileInFront_doesNotChangePositionOrFacing() {
  GameModel m;
  MOVE(m, Direction::RIGHT, 1); // (2,1), facing RIGHT
  const GameModel &readOnly = m;
  for (int i = 0; i < 3; ++i) {
    CHECK_FRONT(readOnly, 3, 1);
    CHECK_POS(readOnly, 2, 1);
    CHECK_FACING(readOnly, Direction::RIGHT);
  }
}

// After movement.

static void tileInFront_followsThePlayerAfterSuccessfulMoves() {
  GameModel m;
  CHECK_FRONT(m, 1, 2);
  CHECK(m.tryMove(Direction::RIGHT)); // (2,1)
  CHECK_FRONT(m, 3, 1);
  CHECK(m.tryMove(Direction::RIGHT)); // (3,1)
  CHECK_FRONT(m, 4, 1);
  CHECK(m.tryMove(Direction::DOWN)); // (3,2), now facing DOWN
  CHECK_POS(m, 3, 2);
  CHECK_FRONT(m, 3, 3);
}

static void tileInFront_afterBlockedMoveStaysOnTheWall() {
  GameModel m;
  MOVE(m, Direction::RIGHT, 3);        // (4,1), facing RIGHT
  CHECK(!m.tryMove(Direction::RIGHT)); // blocked by the wall at (5,1)
  CHECK_POS(m, 4, 1);
  CHECK_FRONT(m, 5, 1);
}

// The blocked move turns the player, so the front tile must switch sides even
// though the position did not change.
static void tileInFront_afterBlockedMoveUsesTheNewFacing() {
  GameModel m;
  MOVE(m, Direction::DOWN, 8);   // (1,9)
  MOVE(m, Direction::RIGHT, 12); // (13,9)
  MOVE(m, Direction::UP, 1);     // (13,8), facing UP
  CHECK_FRONT(m, 13, 7);

  CHECK(!m.tryMove(Direction::RIGHT)); // wall at (14,8)
  CHECK_POS(m, 13, 8);
  CHECK_FACING(m, Direction::RIGHT);
  CHECK_FRONT(m, 14, 8);
}

static void tileInFront_afterBlockedMoveAtMapEdgeIsOffTheMap() {
  GameModel m;
  MOVE(m, Direction::UP, 1);   // (1,0)
  MOVE(m, Direction::LEFT, 1); // (0,0), facing LEFT
  CHECK_FRONT(m, -1, 0);

  CHECK(!m.tryMove(Direction::UP)); // off the top of the map
  CHECK_POS(m, 0, 0);
  CHECK_FACING(m, Direction::UP);
  CHECK_FRONT(m, 0, -1);
}

// ---------------------------------------------------------------------------
<<<<<<< HEAD
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

static void tilling_everyTileStartsUntilled() {
  GameModel m;
  CHECK(countTilled(m) == 0);
  CHECK(!m.isTilled(3, 2));
  CHECK(!m.isTilled(1, 1)); // the tile the player stands on
}

static void tilling_validGroundSucceeds() {
  GameModel m;
  CHECK(m.tryTill(3, 2));
  CHECK(m.isTilled(3, 2));
}

static void tilling_sameTileCannotBeTilledTwice() {
  GameModel m;
  CHECK(m.tryTill(3, 2));
  CHECK(!m.tryTill(3, 2));
  CHECK(!m.tryTill(3, 2));
  CHECK(m.isTilled(3, 2)); // a refused attempt must not undo it
}

static void tilling_everyObstacleRejectsTilling() {
  GameModel m;
  int obstaclesTried = 0;
  for (int row = 0; row < m.mapHeight(); ++row) {
    for (int column = 0; column < m.mapWidth(); ++column) {
      if (m.tileAt(column, row) == TileType::OBSTACLE) {
        CHECK(!m.tryTill(column, row));
        CHECK(!m.isTilled(column, row));
        ++obstaclesTried;
      }
    }
  }
  CHECK(obstaclesTried == 23);
  CHECK(countTilled(m) == 0);
}

static void tilling_outOfBoundsRejectsTilling() {
  GameModel m;
  CHECK(!m.tryTill(-1, 0));
  CHECK(!m.tryTill(0, -1));
  CHECK(!m.tryTill(-1, -1));
  CHECK(!m.tryTill(GameModel::kMapWidth, 0));
  CHECK(!m.tryTill(0, GameModel::kMapHeight));
  CHECK(!m.tryTill(GameModel::kMapWidth, GameModel::kMapHeight));
  CHECK(countTilled(m) == 0);
}

// In the flat array, an off-map coordinate can land on a REAL tile:
// (20,0) -> index 20 -> tile (0,1), and (-1,1) -> index 19 -> tile (19,0).
// A missing bounds check would till those instead of rejecting.
static void tilling_outOfBoundsNeverTouchesAnotherTile() {
  GameModel m;
  CHECK(!m.tryTill(GameModel::kMapWidth, 0));
  CHECK(!m.isTilled(0, 1));
  CHECK(!m.tryTill(-1, 1));
  CHECK(!m.isTilled(19, 0));
  CHECK(countTilled(m) == 0);
}

// Same aliasing, on the read side.
static void isTilled_outOfBoundsIsFalseBesideTilledTiles() {
  GameModel m;
  CHECK(m.tryTill(0, 1));
  CHECK(m.tryTill(19, 0));
  CHECK(!m.isTilled(GameModel::kMapWidth, 0)); // would alias (0,1)
  CHECK(!m.isTilled(-1, 1));                   // would alias (19,0)
}

static void tilling_tilledTileRemainsGround() {
  GameModel m;
  CHECK(m.tryTill(3, 2));
  CHECK_TILE(m, 3, 2, TileType::GROUND);
}

static void tilling_neverChangesTerrainOnAnyTile() {
  GameModel m;
  for (int row = 0; row < m.mapHeight(); ++row) {
    for (int column = 0; column < m.mapWidth(); ++column) {
      const TileType before = m.tileAt(column, row);
      m.tryTill(column, row);
      CHECK_TILE(m, column, row, before);
    }
  }
}

static void tilling_changesOnlyTheTargetTile() {
  GameModel m;
  CHECK(m.tryTill(3, 2));
  CHECK(countTilled(m) == 1);
  CHECK(!m.isTilled(2, 2));
  CHECK(!m.isTilled(4, 2));
  CHECK(!m.isTilled(3, 1));
  CHECK(!m.isTilled(3, 3));
  CHECK(!m.isTilled(2, 3)); // what a swapped (column,row) index would hit
}

static void tilling_failedAttemptsDoNotChangeState() {
  GameModel m;
  CHECK(m.tryTill(3, 2));
  CHECK(countTilled(m) == 1);
  CHECK(!m.tryTill(3, 2));                    // already tilled
  CHECK(!m.tryTill(5, 1));                    // obstacle
  CHECK(!m.tryTill(-1, 0));                   // off the map
  CHECK(!m.tryTill(GameModel::kMapWidth, 0)); // off the map
  CHECK(countTilled(m) == 1);
  CHECK(m.isTilled(3, 2));
}

static void tilling_playerCanWalkAcrossTilledGround() {
  GameModel m;
  CHECK(m.tryTill(2, 1));
  CHECK(m.tryTill(3, 1));
  CHECK(m.tryTill(4, 1));
  MOVE(m, Direction::RIGHT, 3); // (4,1): every step lands on tilled soil
  CHECK_POS(m, 4, 1);
  MOVE(m, Direction::LEFT, 3); // and back to (1,1)
  CHECK_POS(m, 1, 1);
  CHECK(m.isTilled(2, 1)); // walking does not undo tilling
  CHECK(m.isTilled(3, 1));
  CHECK(m.isTilled(4, 1));
}

// The way the SDL layer will use it: target = tileInFront().
static void tilling_worksWithTileInFront() {
  GameModel m; // (1,1), facing DOWN
  const TilePosition front = m.tileInFront();
  CHECK(m.tryTill(front.column, front.row));
  CHECK(m.isTilled(1, 2));
  CHECK(m.tryMove(Direction::DOWN)); // step onto the new soil
  CHECK_POS(m, 1, 2);
  CHECK(m.isTilled(1, 2));
}

static void tilling_tileInFrontThatCannotBeTilledIsRejected() {
  GameModel wall;
  MOVE(wall, Direction::RIGHT, 3); // (4,1), facing RIGHT; wall at (5,1)
  const TilePosition wallFront = wall.tileInFront();
  CHECK(!wall.tryTill(wallFront.column, wallFront.row));
  CHECK(!wall.isTilled(5, 1));

  GameModel edge;
  MOVE(edge, Direction::UP, 1); // (1,0), facing UP; front is off the map
  const TilePosition edgeFront = edge.tileInFront();
  CHECK(!edge.tryTill(edgeFront.column, edgeFront.row));
  CHECK(countTilled(edge) == 0);
}

static void tilling_eachModelHasItsOwnFarmState() {
  GameModel a;
  GameModel b;
  CHECK(a.tryTill(3, 2));
  CHECK(a.isTilled(3, 2));
  CHECK(!b.isTilled(3, 2));
  CHECK(b.tryTill(3, 2)); // b is unaffected, so it can till the same tile
}

// ---------------------------------------------------------------------------
<<<<<<< HEAD
=======
>>>>>>> origin/dev/Bipin
=======
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
  CHECK(m.tryTill(column, row));
  CHECK(m.tryPlant(column, row));
}

static void planting_everyTileStartsEmpty() {
  GameModel m;
  CHECK(countPlanted(m) == 0);
  CHECK(m.tryTill(3, 2));
  CHECK(!m.isPlanted(3, 2)); // tilling alone does not plant anything
}

static void planting_tilledEmptySoilSucceeds() {
  GameModel m;
  CHECK(m.tryTill(3, 2));
  CHECK(m.tryPlant(3, 2));
  CHECK(m.isPlanted(3, 2));
}

static void planting_untilledGroundFails() {
  GameModel m;
  CHECK(!m.tryPlant(3, 2));
  CHECK(!m.isPlanted(3, 2));
  CHECK(!m.isTilled(3, 2)); // a refused plant must not till it as a side effect
}

static void planting_everyObstacleRejects() {
  GameModel m;
  int obstaclesTried = 0;
  for (int row = 0; row < m.mapHeight(); ++row) {
    for (int column = 0; column < m.mapWidth(); ++column) {
      if (m.tileAt(column, row) == TileType::OBSTACLE) {
        CHECK(!m.tryPlant(column, row));
        CHECK(!m.isPlanted(column, row));
        ++obstaclesTried;
      }
    }
  }
  CHECK(obstaclesTried == 23);
  CHECK(countPlanted(m) == 0);
}

static void planting_sameTileCannotBePlantedTwice() {
  GameModel m;
  tillAndPlant(m, 3, 2);
  CHECK(!m.tryPlant(3, 2));
  CHECK(!m.tryPlant(3, 2));
  CHECK(m.isPlanted(3, 2)); // a refused attempt must not remove the crop
  CHECK(countPlanted(m) == 1);
}

static void planting_outOfBoundsFails() {
  GameModel m;
  CHECK(!m.tryPlant(-1, 0));
  CHECK(!m.tryPlant(0, -1));
  CHECK(!m.tryPlant(-1, -1));
  CHECK(!m.tryPlant(GameModel::kMapWidth, 0));
  CHECK(!m.tryPlant(0, GameModel::kMapHeight));
  CHECK(!m.tryPlant(GameModel::kMapWidth, GameModel::kMapHeight));
  CHECK(countPlanted(m) == 0);
}

// Flat-array trap again: (20,0) is index 20 = tile (0,1); (-1,1) is index 19 =
// tile (19,0). Both are made TILLED here, so a missing bounds check would let
// the off-map attempt plant on them instead of being rejected.
static void planting_outOfBoundsNeverTouchesAnotherTile() {
  GameModel m;
  CHECK(m.tryTill(0, 1));
  CHECK(m.tryTill(19, 0));
  CHECK(!m.tryPlant(GameModel::kMapWidth, 0));
  CHECK(!m.isPlanted(0, 1));
  CHECK(!m.tryPlant(-1, 1));
  CHECK(!m.isPlanted(19, 0));
  CHECK(countPlanted(m) == 0);
}

static void isPlanted_outOfBoundsIsFalseBesidePlantedTiles() {
  GameModel m;
  tillAndPlant(m, 0, 1);
  tillAndPlant(m, 19, 0);
  CHECK(!m.isPlanted(GameModel::kMapWidth, 0)); // would alias (0,1)
  CHECK(!m.isPlanted(-1, 1));                   // would alias (19,0)
}

static void planting_tilledSoilRemainsTilled() {
  GameModel m;
  tillAndPlant(m, 3, 2);
  CHECK(m.isTilled(3, 2));
}

static void planting_terrainRemainsGround() {
  GameModel m;
  tillAndPlant(m, 3, 2);
  CHECK_TILE(m, 3, 2, TileType::GROUND);
}

static void planting_changesOnlyTheTargetTile() {
  GameModel m;
  CHECK(m.tryTill(3, 2));
  CHECK(m.tryTill(4, 2)); // tilled neighbour that stays EMPTY
  CHECK(m.tryPlant(3, 2));
  CHECK(countPlanted(m) == 1);
  CHECK(m.isTilled(4, 2));
  CHECK(!m.isPlanted(4, 2)); // tilled but empty is its own state
  CHECK(!m.isPlanted(2, 2));
  CHECK(!m.isPlanted(3, 1));
  CHECK(!m.isPlanted(3, 3));
  CHECK(!m.isPlanted(2, 3)); // what a swapped (column,row) index would hit
  CHECK(m.tryPlant(4, 2));   // the neighbour can still be planted afterwards
  CHECK(countPlanted(m) == 2);
}

static void planting_failedAttemptsDoNotChangeState() {
  GameModel m;
  tillAndPlant(m, 3, 2);
  CHECK(!m.tryPlant(3, 2));                    // already planted
  CHECK(!m.tryPlant(6, 2));                    // untilled ground
  CHECK(!m.tryPlant(5, 1));                    // obstacle
  CHECK(!m.tryPlant(-1, 0));                   // off the map
  CHECK(!m.tryPlant(GameModel::kMapWidth, 0)); // off the map
  CHECK(countPlanted(m) == 1);
  CHECK(countTilled(m) == 1); // failed plants did not till anything
  CHECK(m.isPlanted(3, 2));
}

// Tilling an already-planted tile is refused, and must not wipe the crop.
static void planting_cropSurvivesAFailedTill() {
  GameModel m;
  tillAndPlant(m, 3, 2);
  CHECK(!m.tryTill(3, 2));
  CHECK(m.isTilled(3, 2));
  CHECK(m.isPlanted(3, 2));
}

static void planting_playerCanWalkOverPlantedSoil() {
  GameModel m;
  tillAndPlant(m, 2, 1);
  tillAndPlant(m, 3, 1);
  tillAndPlant(m, 4, 1);
  MOVE(m, Direction::RIGHT, 3); // (4,1): every step lands on a crop
  CHECK_POS(m, 4, 1);
  MOVE(m, Direction::LEFT, 3); // and back to (1,1)
  CHECK_POS(m, 1, 1);
  CHECK(m.isPlanted(2, 1)); // walking does not trample anything
  CHECK(m.isPlanted(3, 1));
  CHECK(m.isPlanted(4, 1));
  CHECK(m.isTilled(3, 1));
}

// The way the SDL layer will use it: target = tileInFront().
static void planting_worksWithTileInFront() {
  GameModel m; // (1,1), facing DOWN
  const TilePosition front = m.tileInFront();
  CHECK(m.tryTill(front.column, front.row));
  CHECK(m.tryPlant(front.column, front.row));
  CHECK(m.isPlanted(1, 2));
  CHECK(m.tryMove(Direction::DOWN)); // step onto the planted tile
  CHECK_POS(m, 1, 2);
  CHECK(m.isPlanted(1, 2));
}

static void planting_tileInFrontThatCannotBePlantedIsRejected() {
  GameModel untilled; // (1,2) is ground but not tilled
  const TilePosition soil = untilled.tileInFront();
  CHECK(!untilled.tryPlant(soil.column, soil.row));

  GameModel wall;
  MOVE(wall, Direction::RIGHT, 3); // (4,1), facing RIGHT; wall at (5,1)
  const TilePosition wallFront = wall.tileInFront();
  CHECK(!wall.tryPlant(wallFront.column, wallFront.row));

  GameModel edge;
  MOVE(edge, Direction::UP, 1); // (1,0), facing UP; front is off the map
  const TilePosition edgeFront = edge.tileInFront();
  CHECK(!edge.tryPlant(edgeFront.column, edgeFront.row));

  CHECK(countPlanted(untilled) + countPlanted(wall) + countPlanted(edge) == 0);
}

static void planting_eachModelHasItsOwnCrops() {
  GameModel a;
  GameModel b;
  tillAndPlant(a, 3, 2);
  CHECK(a.isPlanted(3, 2));
  CHECK(!b.isPlanted(3, 2));
  CHECK(!b.tryPlant(3, 2)); // b's soil is untilled, so it still refuses
}

// ---------------------------------------------------------------------------
>>>>>>> origin/main
// Runner: prints one line per test so you can see exactly which one failed.
// ---------------------------------------------------------------------------

#define RUN(test)                                                              \
  do {                                                                         \
    const int before = g_failures;                                             \
    test();                                                                    \
    std::cout << (g_failures == before ? "[ ok ] " : "[FAIL] ") << #test       \
              << '\n';                                                         \
  } while (0)

int main() {
  RUN(mapDimensionsAre20By15);

  RUN(isInBounds_cornersAreInside);
  RUN(isInBounds_negativeCoordinatesAreOutside);
  RUN(isInBounds_pastTheFarEdgeIsOutside);

  RUN(tileAt_openGroundIsGround);
  RUN(tileAt_wallTilesAreObstacles);
  RUN(tileAt_tilesBesideWallsAreGround);
  RUN(tileAt_totalObstacleCountIs23);
  RUN(tileAt_outOfBoundsIsObstacle);
  RUN(tileAt_borderIsAllGround);

  RUN(start_positionIsColumn1Row1);
  RUN(start_isInBoundsAndOnGround);
  RUN(start_eachModelHasItsOwnState);

  RUN(move_upDecreasesRowByOne);
  RUN(move_downIncreasesRowByOne);
  RUN(move_leftDecreasesColumnByOne);
  RUN(move_rightIncreasesColumnByOne);
  RUN(move_oppositeMovesCancelOut);
  RUN(move_squarePathReturnsToStart);
  RUN(move_longWalkAcrossOpenGroundWorks);

  RUN(obstacle_blocksMovingRight);
  RUN(obstacle_blocksMovingDown);
  RUN(obstacle_blocksMovingUp);
  RUN(obstacle_blocksMovingLeft);
  RUN(obstacle_blockedFromTheLeftSide);
  RUN(obstacle_repeatedBlockedMovesNeverChangePosition);
  RUN(obstacle_canMoveAwayAfterBeingBlocked);

  RUN(edge_topBlocksMovingUp);
  RUN(edge_leftBlocksMovingLeft);
  RUN(edge_bottomBlocksMovingDown);
  RUN(edge_rightBlocksMovingRight);
  RUN(corner_topLeftBlocksUpAndLeft);
  RUN(corner_bottomRightBlocksDownAndRight);

  RUN(facing_startsDown);
  RUN(facing_successfulMoveUpTurnsUp);
  RUN(facing_successfulMoveLeftTurnsLeft);
  RUN(facing_successfulMoveRightTurnsRight);
  RUN(facing_successfulMoveDownTurnsDown);
  RUN(facing_followsTheLastRequestedDirection);
  RUN(facing_blockedByObstacleStillTurns);
  RUN(facing_blockedFromBelowStillTurnsUp);
  RUN(facing_blockedByMapEdgeStillTurns);
  RUN(facing_followsEachBlockedAttemptOnTheSameTile);
  RUN(facing_invalidDirectionIsIgnored);
  RUN(facing_eachModelHasItsOwnFacing);

  RUN(tileInFront_atStartIsTheTileBelow);
  RUN(tileInFront_facingUpIsRowMinusOne);
  RUN(tileInFront_facingDownIsRowPlusOne);
  RUN(tileInFront_facingLeftIsColumnMinusOne);
  RUN(tileInFront_facingRightIsColumnPlusOne);
  RUN(tileInFront_pastTopEdgeIsReturnedUnclamped);
  RUN(tileInFront_pastLeftEdgeIsReturnedUnclamped);
  RUN(tileInFront_pastBottomEdgeIsReturnedUnclamped);
  RUN(tileInFront_pastRightEdgeIsReturnedUnclamped);
  RUN(tileInFront_returnsAnObstacleTileWithoutJudgingIt);
  RUN(tileInFront_doesNotChangePositionOrFacing);
  RUN(tileInFront_followsThePlayerAfterSuccessfulMoves);
  RUN(tileInFront_afterBlockedMoveStaysOnTheWall);
  RUN(tileInFront_afterBlockedMoveUsesTheNewFacing);
  RUN(tileInFront_afterBlockedMoveAtMapEdgeIsOffTheMap);

<<<<<<< HEAD
  RUN(tilling_everyTileStartsUntilled);
  RUN(tilling_validGroundSucceeds);
  RUN(tilling_sameTileCannotBeTilledTwice);
  RUN(tilling_everyObstacleRejectsTilling);
  RUN(tilling_outOfBoundsRejectsTilling);
  RUN(tilling_outOfBoundsNeverTouchesAnotherTile);
  RUN(isTilled_outOfBoundsIsFalseBesideTilledTiles);
  RUN(tilling_tilledTileRemainsGround);
  RUN(tilling_neverChangesTerrainOnAnyTile);
  RUN(tilling_changesOnlyTheTargetTile);
  RUN(tilling_failedAttemptsDoNotChangeState);
  RUN(tilling_playerCanWalkAcrossTilledGround);
  RUN(tilling_worksWithTileInFront);
  RUN(tilling_tileInFrontThatCannotBeTilledIsRejected);
  RUN(tilling_eachModelHasItsOwnFarmState);

<<<<<<< HEAD
=======
>>>>>>> origin/dev/Bipin
=======
  RUN(planting_everyTileStartsEmpty);
  RUN(planting_tilledEmptySoilSucceeds);
  RUN(planting_untilledGroundFails);
  RUN(planting_everyObstacleRejects);
  RUN(planting_sameTileCannotBePlantedTwice);
  RUN(planting_outOfBoundsFails);
  RUN(planting_outOfBoundsNeverTouchesAnotherTile);
  RUN(isPlanted_outOfBoundsIsFalseBesidePlantedTiles);
  RUN(planting_tilledSoilRemainsTilled);
  RUN(planting_terrainRemainsGround);
  RUN(planting_changesOnlyTheTargetTile);
  RUN(planting_failedAttemptsDoNotChangeState);
  RUN(planting_cropSurvivesAFailedTill);
  RUN(planting_playerCanWalkOverPlantedSoil);
  RUN(planting_worksWithTileInFront);
  RUN(planting_tileInFrontThatCannotBePlantedIsRejected);
  RUN(planting_eachModelHasItsOwnCrops);

>>>>>>> origin/main
  RUN(randomWalk_alwaysObeysTheRules);

  if (g_failures == 0) {
    std::cout << "\nAll tests passed\n";
    return 0;
  }
  std::cout << "\n" << g_failures << " check(s) failed\n";
  return 1;
}
