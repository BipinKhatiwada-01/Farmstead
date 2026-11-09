#pragma once

class Progression {
public:
  void onCoinsEarned(int amount);

  int coinsEarned() const;

  int objectiveTarget() const;

  bool objectiveCompleted() const;

  bool canRecordCoins(int amount) const;

  bool restoreState(int totalCoinsEarned);

private:
  int totalCoinsEarned{0};

  static constexpr int kObjectiveTarget = 100;
};
