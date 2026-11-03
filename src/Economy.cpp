#include "../include/farmstead/Economy.hpp"

#include <limits>

int Economy::coins() const { return currency; }

int Economy::produceSellPrice() const { return kProduceSellPrice; }

int Economy::calculateSaleValue(int quantity) const {
  if (quantity <= 0) {
    return 0;
  }

  // prevent overflow of int
  if (quantity > std::numeric_limits<int>::max() / kProduceSellPrice) {
    return 0;
  }

  return quantity * kProduceSellPrice;
}

bool Economy::addCoins(int amount) {
  if (amount <= 0) {
    return false;
  }

  if (currency > std::numeric_limits<int>::max() - currency) {
    return false;
  }

  currency += amount;
  return true;
}
