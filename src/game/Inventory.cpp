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
  if (quantity <= 0 || quantity > produce) {
    return false;
  }

  produce -= quantity;
  return true;
}

int Inventory::produceCount() const { return produce; }
