#include "farmstead/Inventory.hpp"
#include "catch2/catch_test_macros.hpp"

#include <catch2/catch_all.hpp>

TEST_CASE("Initially Inventory has zero produce") {
  Inventory inventory;
  REQUIRE(inventory.produceCount() == 0);
}

TEST_CASE("Add produce increases produce") {
  Inventory inventory;
  inventory.addProduce(5);
  REQUIRE(inventory.produceCount() == 5);
  inventory.addProduce(3);
  REQUIRE(inventory.produceCount() == 8);
}

TEST_CASE("Remove produce decreases produce") {
  Inventory inventory;
  inventory.addProduce(5);

  inventory.removeProduce(2);
  REQUIRE(inventory.produceCount() == 3);

  inventory.removeProduce(3);
  REQUIRE(inventory.produceCount() == 0);
}

TEST_CASE("Remove more than available fails") {
  Inventory inventory;
  inventory.addProduce(2);

  REQUIRE(inventory.removeProduce(3) == false);
}

TEST_CASE("Adding zero or negative fails") {
  Inventory inventory;
  REQUIRE(inventory.addProduce(0) == false);
  REQUIRE(inventory.addProduce(-1) == false);
}

TEST_CASE("Removing zero or negative fails") {
  Inventory inventory;
  REQUIRE(inventory.removeProduce(0) == false);
  REQUIRE(inventory.removeProduce(-1) == false);
}
