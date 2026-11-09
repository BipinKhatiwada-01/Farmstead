#include "farmstead/game_model.hpp"
#include "catch2/catch_test_macros.hpp"

#include <catch2/catch_all.hpp>

using namespace farmstead;

namespace {

GameSaveData validSaveData() { return GameModel{}.exportSaveData(); }

void requireSaveDataEqual(const GameSaveData &actual,
                          const GameSaveData &expected) {
  REQUIRE(actual.day == expected.day);
  REQUIRE(actual.playerPosition == expected.playerPosition);
  REQUIRE(actual.facing == expected.facing);
  REQUIRE(actual.tilledTiles == expected.tilledTiles);
  REQUIRE(actual.crops.size() == expected.crops.size());
  for (std::size_t index = 0; index < actual.crops.size(); ++index) {
    REQUIRE(actual.crops[index].position == expected.crops[index].position);
    REQUIRE(actual.crops[index].growthPoints ==
            expected.crops[index].growthPoints);
    REQUIRE(actual.crops[index].wateredToday ==
            expected.crops[index].wateredToday);
  }
  REQUIRE(actual.produce == expected.produce);
  REQUIRE(actual.coins == expected.coins);
  REQUIRE(actual.totalCoinsEarned == expected.totalCoinsEarned);
}

void requireRestoreRejected(const GameSaveData &data) {
  GameModel model;
  REQUIRE_FALSE(model.restoreSaveData(data));
}

GameSaveData populatedSaveData() {
  GameSaveData data = validSaveData();
  data.day = 9;
  data.playerPosition = {8, 8};
  data.facing = Direction::RIGHT;
  data.tilledTiles = {{3, 2}, {4, 2}, {6, 2}};
  data.crops = {{ {3, 2}, 0, true},
                { {4, 2}, 3, false},
                { {6, 2}, 7, true}};
  data.produce = 12;
  data.coins = 40;
  data.totalCoinsEarned = 105;
  return data;
}

} // namespace

TEST_CASE("exportSaveData captures the initial game state") {
  GameModel model;

  const GameSaveData data = model.exportSaveData();

  REQUIRE(data.day == 1);
  REQUIRE(data.playerPosition == TilePosition{1, 1});
  REQUIRE(data.facing == Direction::DOWN);
  REQUIRE(data.tilledTiles.empty());
  REQUIRE(data.crops.empty());
  REQUIRE(data.produce == 0);
  REQUIRE(data.coins == 0);
  REQUIRE(data.totalCoinsEarned == 0);
}

TEST_CASE("exportSaveData captures farming and progression without mutation") {
  GameModel model;
  const TilePosition matureCropA{3, 2};
  const TilePosition matureCropB{4, 2};
  const TilePosition emptyTilledTile{6, 2};
  const TilePosition sproutCrop{7, 2};
  const TilePosition seedCrop{8, 2};

  for (const TilePosition position : {matureCropA, matureCropB,
                                      emptyTilledTile, sproutCrop, seedCrop}) {
    REQUIRE(model.tryTill(position.column, position.row));
  }
  for (const TilePosition position :
       {matureCropA, matureCropB, sproutCrop, seedCrop}) {
    REQUIRE(model.tryPlant(position.column, position.row));
  }

  for (int day = 0; day < 6; ++day) {
    REQUIRE(model.tryWater(matureCropA.column, matureCropA.row));
    REQUIRE(model.tryWater(matureCropB.column, matureCropB.row));
    if (day < 3) {
      REQUIRE(model.tryWater(sproutCrop.column, sproutCrop.row));
    }
    model.advanceDay();
  }
  REQUIRE(model.tryWater(sproutCrop.column, sproutCrop.row));
  REQUIRE(model.tryWater(seedCrop.column, seedCrop.row));
  REQUIRE(model.tryHarvest(matureCropA.column, matureCropA.row));
  REQUIRE(model.tryHarvest(matureCropB.column, matureCropB.row));

  for (int step = 0; step < 4; ++step) {
    REQUIRE(model.tryMove(Direction::DOWN));
  }
  for (int step = 0; step < 9; ++step) {
    REQUIRE(model.tryMove(Direction::RIGHT));
  }
  REQUIRE_FALSE(model.tryMove(Direction::UP));
  REQUIRE(model.trySellProduce(1));

  const GameSaveData beforeExport = model.exportSaveData();
  const GameSaveData data = model.exportSaveData();

  REQUIRE(data.day == 7);
  REQUIRE(data.playerPosition == TilePosition{10, 5});
  REQUIRE(data.facing == Direction::UP);
  REQUIRE(data.tilledTiles ==
          std::vector<TilePosition>{{3, 2}, {4, 2}, {6, 2}, {7, 2}, {8, 2}});
  REQUIRE(data.crops.size() == 2);
  REQUIRE(data.crops[0].position == sproutCrop);
  REQUIRE(data.crops[0].growthPoints == 3);
  REQUIRE(data.crops[0].wateredToday);
  REQUIRE(data.crops[1].position == seedCrop);
  REQUIRE(data.crops[1].growthPoints == 0);
  REQUIRE(data.crops[1].wateredToday);
  REQUIRE(data.produce == 1);
  REQUIRE(data.coins == 5);
  REQUIRE(data.totalCoinsEarned == 5);

  requireSaveDataEqual(model.exportSaveData(), beforeExport);
  REQUIRE(model.currentDay() == 7);
  REQUIRE(model.playerPosition() == TilePosition{10, 5});
  REQUIRE(model.facingDirection() == Direction::UP);
  REQUIRE(model.isTilled(emptyTilledTile.column, emptyTilledTile.row));
  REQUIRE(model.isPlanted(sproutCrop.column, sproutCrop.row));
  REQUIRE(model.isWateredToday(seedCrop.column, seedCrop.row));
}

TEST_CASE("restoreSaveData restores the complete game state") {
  GameModel model;
  const GameSaveData data = populatedSaveData();

  REQUIRE(model.restoreSaveData(data));

  REQUIRE(model.currentDay() == 9);
  REQUIRE(model.playerPosition() == TilePosition{8, 8});
  REQUIRE(model.facingDirection() == Direction::RIGHT);
  REQUIRE(model.isTilled(3, 2));
  REQUIRE(model.isTilled(4, 2));
  REQUIRE(model.isTilled(6, 2));
  REQUIRE(model.isPlanted(3, 2));
  REQUIRE(model.isPlanted(4, 2));
  REQUIRE(model.isPlanted(6, 2));
  REQUIRE(model.cropStageAt(3, 2) == CropStage::SEED);
  REQUIRE(model.cropStageAt(4, 2) == CropStage::SPROUT);
  REQUIRE(model.cropStageAt(6, 2) == CropStage::MATURE);
  REQUIRE(model.isWateredToday(3, 2));
  REQUIRE_FALSE(model.isWateredToday(4, 2));
  REQUIRE(model.isWateredToday(6, 2));
  REQUIRE(model.produceCount() == 12);
  REQUIRE(model.coins() == 40);
  REQUIRE(model.coinsEarned() == 105);
  REQUIRE(model.objectiveTarget() == 100);
  REQUIRE(model.objectiveCompleted());
}

TEST_CASE("restoring and exporting preserves the complete save data") {
  const GameSaveData expected = populatedSaveData();
  GameModel model;

  REQUIRE(model.restoreSaveData(expected));

  requireSaveDataEqual(model.exportSaveData(), expected);
}

TEST_CASE("restoreSaveData rejects invalid day, player, and direction") {
  SECTION("day must be at least one") {
    GameSaveData data = validSaveData();
    data.day = 0;
    requireRestoreRejected(data);
  }

  SECTION("player position must be within map bounds") {
    GameSaveData data = validSaveData();
    data.playerPosition = {-1, 1};
    requireRestoreRejected(data);
  }

  SECTION("player cannot be on an obstacle, market, or farmhouse") {
    for (const TilePosition position :
         {TilePosition{5, 1}, TilePosition{10, 3}, TilePosition{15, 5}}) {
      GameSaveData data = validSaveData();
      data.playerPosition = position;
      REQUIRE_FALSE(GameModel{}.restoreSaveData(data));
    }
  }

  SECTION("direction enum must be valid") {
    GameSaveData data = validSaveData();
    data.facing = static_cast<Direction>(99);
    requireRestoreRejected(data);
  }
}

TEST_CASE("restoreSaveData rejects invalid tilled positions") {
  SECTION("tilled positions must be within map bounds") {
    GameSaveData data = validSaveData();
    data.tilledTiles = {{GameModel::kMapWidth, 2}};
    requireRestoreRejected(data);
  }

  SECTION("tilled positions cannot be buildings or obstacles") {
    for (const TilePosition position :
         {TilePosition{5, 1}, TilePosition{10, 3}, TilePosition{15, 5}}) {
      GameSaveData data = validSaveData();
      data.tilledTiles = {position};
      REQUIRE_FALSE(GameModel{}.restoreSaveData(data));
    }
  }

  SECTION("tilled positions cannot be duplicated") {
    GameSaveData data = validSaveData();
    data.tilledTiles = {{3, 2}, {3, 2}};
    requireRestoreRejected(data);
  }
}

TEST_CASE("restoreSaveData rejects invalid crop positions and growth") {
  SECTION("crop positions must be within map bounds") {
    GameSaveData data = validSaveData();
    data.tilledTiles = {{3, 2}};
    data.crops = {{{GameModel::kMapWidth, 2}, 0, false}};
    requireRestoreRejected(data);
  }

  SECTION("crops must be on tilled tiles") {
    GameSaveData data = validSaveData();
    data.crops = {{{3, 2}, 0, false}};
    requireRestoreRejected(data);
  }

  SECTION("crops cannot be on buildings or obstacles") {
    for (const TilePosition position :
         {TilePosition{5, 1}, TilePosition{10, 3}, TilePosition{15, 5}}) {
      GameSaveData data = validSaveData();
      data.tilledTiles = {position};
      data.crops = {{position, 0, false}};
      REQUIRE_FALSE(GameModel{}.restoreSaveData(data));
    }
  }

  SECTION("crop positions cannot be duplicated") {
    GameSaveData data = validSaveData();
    data.tilledTiles = {{3, 2}};
    data.crops = {{{3, 2}, 0, false}, {{3, 2}, 1, true}};
    requireRestoreRejected(data);
  }

  SECTION("growth points cannot be negative") {
    GameSaveData data = validSaveData();
    data.tilledTiles = {{3, 2}};
    data.crops = {{{3, 2}, -1, false}};
    requireRestoreRejected(data);
  }
}

TEST_CASE("restoreSaveData rejects negative inventory and progression values") {
  SECTION("produce cannot be negative") {
    GameSaveData data = validSaveData();
    data.produce = -1;
    requireRestoreRejected(data);
  }

  SECTION("coins cannot be negative") {
    GameSaveData data = validSaveData();
    data.coins = -1;
    requireRestoreRejected(data);
  }

  SECTION("total coins earned cannot be negative") {
    GameSaveData data = validSaveData();
    data.totalCoinsEarned = -1;
    requireRestoreRejected(data);
  }
}

TEST_CASE("failed restoration leaves the existing game state unchanged") {
  GameModel model;
  const GameSaveData existingState = populatedSaveData();
  REQUIRE(model.restoreSaveData(existingState));
  const GameSaveData beforeRestore = model.exportSaveData();

  SECTION("invalid data discovered after valid farming state is unchanged") {
    GameSaveData invalidData = validSaveData();
    invalidData.day = 15;
    invalidData.playerPosition = {9, 8};
    invalidData.facing = Direction::LEFT;
    invalidData.tilledTiles = {{2, 2}, {3, 2}};
    invalidData.crops = {{{3, 2}, 4, true}};
    invalidData.produce = 2;
    invalidData.coins = 8;
    invalidData.totalCoinsEarned = 10;
    invalidData.crops.push_back({{3, 2}, 5, false});

    REQUIRE_FALSE(model.restoreSaveData(invalidData));
    requireSaveDataEqual(model.exportSaveData(), beforeRestore);
  }

  SECTION("early validation failure leaves existing state unchanged") {
    GameSaveData invalidData = validSaveData();
    invalidData.day = 0;

    REQUIRE_FALSE(model.restoreSaveData(invalidData));
    requireSaveDataEqual(model.exportSaveData(), beforeRestore);
  }

  REQUIRE(model.currentDay() == beforeRestore.day);
  REQUIRE(model.playerPosition() == beforeRestore.playerPosition);
  REQUIRE(model.facingDirection() == beforeRestore.facing);
  REQUIRE(model.isTilled(3, 2));
  REQUIRE(model.isPlanted(4, 2));
  REQUIRE(model.produceCount() == beforeRestore.produce);
  REQUIRE(model.coins() == beforeRestore.coins);
  REQUIRE(model.coinsEarned() == beforeRestore.totalCoinsEarned);
}