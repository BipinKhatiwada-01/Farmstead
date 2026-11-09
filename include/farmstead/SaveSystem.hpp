#pragma once

#include "../../include/farmstead/game_model.hpp"
#include <filesystem>

namespace farmstead {

class SaveSystem {
public:
  static bool save(const GameModel &game, const std::filesystem::path &path);

  static bool load(GameModel &game, const std::filesystem::path &path);
};
} // namespace farmstead
