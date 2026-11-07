#pragma once

#include "farmstead/game_model.hpp"
#include <filesystem>

using namespace farmstead;

class SaveSystem {
public:
  static bool save(const GameModel &game, const std::filesystem::path &path);

  static bool load(GameModel &game, const std::filesystem::path &path);
};
