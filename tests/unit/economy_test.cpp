#include "farmstead/Economy.hpp"
#include "catch2/catch_test_macros.hpp"

#include <catch2/catch_all.hpp>

TEST_CASE("Initially currency is zero") {
  Economy economy;
  REQUIRE(economy.coins() == 0);
}

TEST_CASE("Selling Price is 5") {
  Economy economy;
  REQUIRE(economy.produceSellPrice() == 5);
}

TEST_CASE("Calculate Sale Value") {
  Economy economy;
  REQUIRE(economy.calculateSaleValue(1) == 5);
  REQUIRE(economy.calculateSaleValue(3) == 15);
  REQUIRE(economy.calculateSaleValue(10) == 50);
}

TEST_CASE("Calculate Sale Value for zero and negative") {
  Economy economy;
  REQUIRE(economy.calculateSaleValue(0) == 0);
  REQUIRE(economy.calculateSaleValue(-5) == 0);
}

TEST_CASE("Add 50 cois") {
  Economy economy;
  REQUIRE(economy.addCoins(50) == true);
  REQUIRE(economy.coins() == 50);
}

TEST_CASE("Add coins multiple times") {
  Economy economy;
  economy.addCoins(10);
  economy.addCoins(10);
  REQUIRE(economy.coins() == 20);
  economy.addCoins(20);
  economy.addCoins(40);
  REQUIRE(economy.coins() == 80);
}

TEST_CASE("Add zero coin fails") {
  Economy economy;
  economy.addCoins(10);
  REQUIRE(economy.coins() == 10);
  REQUIRE(economy.addCoins(0) == false);
  REQUIRE(economy.coins() == 10);
}

TEST_CASE("Protects against currency overflow") {
  Economy economy;
  economy.addCoins(10);
  REQUIRE(economy.coins() == 10);
  REQUIRE(economy.addCoins(std::numeric_limits<int>::max()) == false);
  REQUIRE(economy.coins() == 10);
}

TEST_CASE("Calling calculateSaleValue should not change currency") {
  Economy economy;
  economy.addCoins(10);
  REQUIRE(economy.coins() == 10);
  economy.calculateSaleValue(5);
  REQUIRE(economy.coins() == 10);
}
