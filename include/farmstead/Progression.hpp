#pragma once

class Progression {
public:
  void onCoinsEarned(int amount);

  int coinsEarned() const;

  int objectiveTarget() const;

  bool objectiveCompleted() const;

private:
  int totalCoinsEarned{0};

  static constexpr int kObjectiveTarget = 100;
};
