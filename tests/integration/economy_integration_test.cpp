#include "farmstead/game_model.hpp"
#include "catch2/catch_test_macros.hpp"

#include <catch2/catch_all.hpp>

#include <vector>

using namespace farmstead;

namespace {

constexpr int kDaysToMature = 6;
constexpr int kProduceSellPrice = 5;

void plantAndHarvestMatureCrop(GameModel &model, int column, int row) {
  REQUIRE(model.tryTill(column, row));
  REQUIRE(model.tryPlant(column, row));

  for (int day = 0; day < kDaysToMature; ++day) {
    INFO("growth day " << day + 1 << " at (" << column << ", " << row << ")");
    REQUIRE(model.tryWater(column, row));
    model.advanceDay();
  }

  REQUIRE(model.tryHarvest(column, row));
}

void harvestProduce(GameModel &model, int quantity) {
  REQUIRE(quantity > 0);

  std::vector<TilePosition> cropPositions;
  for (int row = 0; row < model.mapHeight() &&
                    static_cast<int>(cropPositions.size()) < quantity;
       ++row) {
    for (int column = 0; column < model.mapWidth() &&
                        static_cast<int>(cropPositions.size()) < quantity;
         ++column) {
      if (model.tileAt(column, row) != TileType::GROUND ||
          !model.tryTill(column, row)) {
        continue;
      }

      INFO("preparing crop at (" << column << ", " << row << ")");
      REQUIRE(model.tryPlant(column, row));
      cropPositions.push_back({column, row});
    }
  }
  REQUIRE(static_cast<int>(cropPositions.size()) == quantity);

  for (int day = 0; day < kDaysToMature; ++day) {
    for (const TilePosition &position : cropPositions) {
      INFO("watering day " << day + 1 << " at (" << position.column << ", "
                            << position.row << ")");
      REQUIRE(model.tryWater(position.column, position.row));
    }
    model.advanceDay();
  }

  for (const TilePosition &position : cropPositions) {
    INFO("harvesting crop at (" << position.column << ", " << position.row
                                << ")");
    REQUIRE(model.tryHarvest(position.column, position.row));
  }
}

void faceMarket(GameModel &model) {
  for (int step = 0; step < 4; ++step) {
    REQUIRE(model.tryMove(Direction::DOWN));
  }
  for (int step = 0; step < 8; ++step) {
    REQUIRE(model.tryMove(Direction::RIGHT));
  }
  REQUIRE(model.tileInFront() == GameModel::kMarketPosition);
}

} // namespace

TEST_CASE("Harvesting a mature crop updates inventory and preserves soil") {
  GameModel model;

  plantAndHarvestMatureCrop(model, 3, 2);

  REQUIRE(model.produceCount() == 1);
  REQUIRE_FALSE(model.isPlanted(3, 2));
  REQUIRE(model.cropStageAt(3, 2) == CropStage::NONE);
  REQUIRE(model.isTilled(3, 2));
  REQUIRE(model.tileAt(3, 2) == TileType::GROUND);
  REQUIRE(model.tryPlant(3, 2));
  REQUIRE(model.cropStageAt(3, 2) == CropStage::SEED);
}

TEST_CASE("Immature, empty, and invalid tiles do not yield produce") {
  GameModel model;

  SECTION("an immature crop cannot be harvested") {
    REQUIRE(model.tryTill(3, 2));
    REQUIRE(model.tryPlant(3, 2));

    REQUIRE_FALSE(model.tryHarvest(3, 2));
    REQUIRE(model.produceCount() == 0);
    REQUIRE(model.isPlanted(3, 2));
  }

  SECTION("empty ground cannot be harvested") {
    REQUIRE_FALSE(model.tryHarvest(3, 2));
    REQUIRE(model.produceCount() == 0);
  }

  SECTION("an obstacle cannot be harvested") {
    REQUIRE_FALSE(model.tryHarvest(5, 1));
    REQUIRE(model.produceCount() == 0);
  }

  SECTION("an out-of-bounds tile cannot be harvested") {
    REQUIRE_FALSE(model.tryHarvest(-1, 0));
    REQUIRE(model.produceCount() == 0);
  }
}

TEST_CASE("Selling one harvested produce credits its sale value") {
  GameModel model;
  harvestProduce(model, 1);
  faceMarket(model);

  REQUIRE(model.trySellProduce(1));

  REQUIRE(model.produceCount() == 0);
  REQUIRE(model.coins() == kProduceSellPrice);
  REQUIRE(model.coinsEarned() == kProduceSellPrice);
}

TEST_CASE("Selling multiple produce updates inventory and accumulates proceeds") {
  GameModel model;
  harvestProduce(model, 3);
  faceMarket(model);

  REQUIRE(model.trySellProduce(2));
  REQUIRE(model.produceCount() == 1);
  REQUIRE(model.coins() == 2 * kProduceSellPrice);
  REQUIRE(model.coinsEarned() == 2 * kProduceSellPrice);

  REQUIRE(model.trySellProduce(1));
  REQUIRE(model.produceCount() == 0);
  REQUIRE(model.coins() == 3 * kProduceSellPrice);
  REQUIRE(model.coinsEarned() == 3 * kProduceSellPrice);
}

TEST_CASE("Failed sales leave inventory, coins, and progression unchanged") {
  GameModel model;
  harvestProduce(model, 2);
  faceMarket(model);
  REQUIRE(model.trySellProduce(1));

  const int produceBefore = model.produceCount();
  const int coinsBefore = model.coins();
  const int coinsEarnedBefore = model.coinsEarned();

  SECTION("zero quantity is rejected") {
    REQUIRE_FALSE(model.trySellProduce(0));
  }

  SECTION("negative quantity is rejected") {
    REQUIRE_FALSE(model.trySellProduce(-1));
  }

  SECTION("quantity above inventory is rejected") {
    REQUIRE_FALSE(model.trySellProduce(produceBefore + 1));
  }

  REQUIRE(model.produceCount() == produceBefore);
  REQUIRE(model.coins() == coinsBefore);
  REQUIRE(model.coinsEarned() == coinsEarnedBefore);
}

TEST_CASE("Successful sales accumulate progression across transactions") {
  GameModel model;
  harvestProduce(model, 3);
  faceMarket(model);

  REQUIRE(model.trySellProduce(1));
  REQUIRE(model.trySellProduce(2));

  REQUIRE(model.coins() == 3 * kProduceSellPrice);
  REQUIRE(model.coinsEarned() == 3 * kProduceSellPrice);
  REQUIRE(model.produceCount() == 0);
  REQUIRE_FALSE(model.objectiveCompleted());
}

TEST_CASE("Earning less than the objective target leaves it incomplete") {
  GameModel model;
  REQUIRE(model.objectiveTarget() == 100);
  harvestProduce(model, 19);
  faceMarket(model);

  REQUIRE(model.trySellProduce(19));

  REQUIRE(model.coinsEarned() == 95);
  REQUIRE(model.coinsEarned() < model.objectiveTarget());
  REQUIRE_FALSE(model.objectiveCompleted());
}

TEST_CASE("Selling enough produce to reach the target completes the objective") {
  GameModel model;
  REQUIRE(model.objectiveTarget() == 100);
  harvestProduce(model, 20);
  faceMarket(model);

  REQUIRE(model.trySellProduce(20));

  REQUIRE(model.coins() == model.objectiveTarget());
  REQUIRE(model.coinsEarned() == model.objectiveTarget());
  REQUIRE(model.objectiveCompleted());
}

TEST_CASE("Sales after objective completion do not reset completion") {
  GameModel model;
  harvestProduce(model, 21);
  faceMarket(model);
  REQUIRE(model.trySellProduce(20));
  REQUIRE(model.objectiveCompleted());

  REQUIRE(model.trySellProduce(1));

  REQUIRE(model.coinsEarned() == 21 * kProduceSellPrice);
  REQUIRE(model.objectiveCompleted());
}