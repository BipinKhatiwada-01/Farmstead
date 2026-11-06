#include "farmstead/Progression.hpp"

#include <limits>

void Progression::onCoinsEarned(int amount) {
  if (!canRecordCoins(amount))
    return;
  totalCoinsEarned += amount;
}

int Progression::coinsEarned() const { return totalCoinsEarned; }

int Progression::objectiveTarget() const { return kObjectiveTarget; }

bool Progression::objectiveCompleted() const {
  return totalCoinsEarned >= kObjectiveTarget;
}

bool Progression::canRecordCoins(int amount) const {
  if (amount <= 0)
    return false;

  if (totalCoinsEarned > std::numeric_limits<int>::max() - amount)
    return false;

  return true;
}
