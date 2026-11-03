#include "../include/farmstead/Progression.hpp"
#include "catch2/catch_test_macros.hpp"

TEST_CASE("check initial values") {
  Progression progression;
  REQUIRE(progression.coinsEarned() == 0);
  REQUIRE(progression.objectiveTarget() == 100);
  REQUIRE_FALSE(progression.objectiveCompleted());
}

TEST_CASE("Earn 25 coin") {
  Progression progression;
  progression.onCoinsEarned(25);
  REQUIRE(progression.coinsEarned() == 25);
}

TEST_CASE("Earn coins multiple times") {
  Progression progression;
  progression.onCoinsEarned(10);
  progression.onCoinsEarned(10);
  REQUIRE(progression.coinsEarned() == 20);
  progression.onCoinsEarned(20);
  progression.onCoinsEarned(20);
  REQUIRE(progression.coinsEarned() == 60);
}

TEST_CASE("Earn exactly 100 coins completes objectives") {
  Progression progression;
  progression.onCoinsEarned(100);
  REQUIRE(progression.objectiveCompleted());
}

TEST_CASE("Earn more than 100 coins completes objectives") {
  Progression progression;
  progression.onCoinsEarned(120);
  REQUIRE(progression.objectiveCompleted());
}

TEST_CASE("Earn 50 + 50 coins") {
  Progression progression;
  progression.onCoinsEarned(50);
  progression.onCoinsEarned(50);
  REQUIRE(progression.coinsEarned() == progression.objectiveTarget());
  REQUIRE(progression.objectiveCompleted());
}

TEST_CASE("Earn zero and negative coins doesn't change value") {
  Progression progression;
  progression.onCoinsEarned(10);
  REQUIRE(progression.coinsEarned() == 10);
  progression.onCoinsEarned(0);
  REQUIRE(progression.coinsEarned() == 10);
  progression.onCoinsEarned(-20);
  REQUIRE(progression.coinsEarned() == 10);
}

TEST_CASE("Integer overflow protection") {
  Progression progression;
  progression.onCoinsEarned(0);
  progression.onCoinsEarned(std::numeric_limits<int>::max());
  progression.onCoinsEarned(0);
}

TEST_CASE("Cumulative Progression works") {
  Progression progression;
  progression.onCoinsEarned(20);
  REQUIRE_FALSE(progression.objectiveCompleted());
  progression.onCoinsEarned(30);
  REQUIRE_FALSE(progression.objectiveCompleted());
  progression.onCoinsEarned(30);
  REQUIRE_FALSE(progression.objectiveCompleted());
  progression.onCoinsEarned(20);
  REQUIRE(progression.objectiveCompleted());
}
