#pragma once

class Inventory {
public:
  bool addProduce(int quantity);
  bool removeProduce(int quantity);

  bool canRemoveProduce(int quantity) const;

  int produceCount() const;

private:
  int produce{0};
};
