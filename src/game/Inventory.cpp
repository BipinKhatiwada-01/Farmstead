#include "farmstead/Inventory.hpp"

#include <limits>

bool Inventory::addProduce(int quantity) {
  if (quantity <= 0) {
    return false;
  }

  // prevents integer overflow
  if (produce > std::numeric_limits<int>::max() - quantity) {
    return false;
  }

  produce += quantity;
  return true;
}

bool Inventory::removeProduce(int quantity) {
  if (!canRemoveProduce(quantity)) {
    return false;
  }

  produce -= quantity;
  return true;
}

int Inventory::produceCount() const { return produce; }

bool Inventory::canRemoveProduce(int quantity) const {
  return quantity > 0 && quantity <= produce;
}
