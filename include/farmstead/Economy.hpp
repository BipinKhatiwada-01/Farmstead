#pragma once

class Economy {
public:
  int coins() const;

  int produceSellPrice() const;
  int calculateSaleValue(int quantity) const;

  bool addCoins(int amount);

  bool canAddCoins(int amount) const;

  bool restoreState(int currency);

private:
  int currency{0};

  static constexpr int kProduceSellPrice = 5;
};
