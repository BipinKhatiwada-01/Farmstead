#pragma once

class Inventory {
public:
  bool addProduce(int quantity);
  bool removeProduce(int quantity);

  int produceCount() const;

private:
  int produce{0};
};
