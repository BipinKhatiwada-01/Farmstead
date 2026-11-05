#include "farmstead/Progression.hpp"

#include <limits>

void Progression::onCoinsEarned(int amount) {
  if (amount <= 0)
    return;

  if (totalCoinsEarned > std::numeric_limits<int>::max() - amount)
    return;

  totalCoinsEarned += amount;
}

int Progression::coinsEarned() const { return totalCoinsEarned; }

int Progression::objectiveTarget() const { return kObjectiveTarget; }

bool Progression::objectiveCompleted() const {
  return totalCoinsEarned >= kObjectiveTarget;
}
