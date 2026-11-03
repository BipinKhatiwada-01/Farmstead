#pragma once

class Economy {
public:
  int coins() const;

  int produceSellPrice() const;
  int calculateSaleValue(int quantity) const;

  bool addCoins(int amount);

private:
  int currency{0};

  static constexpr int kProduceSellPrice = 5;
};
