#pragma once

#include "../../include/farmstead/game_model.hpp"
#include <filesystem>

namespace farmstead {

/// Handles saving and loading game state using JSON files.
///
/// The application/SDL layer should use this class whenever the player
/// wants to save or load a game. JSON serialization and filesystem
/// operations are handled internally.
///
/// Example:
///   SaveSystem::save(game, "saves/save.json");
///   SaveSystem::load(game, "saves/save.json");
class SaveSystem {
public:
  /// Saves the current GameModel state to the specified file.
  ///
  /// The parent directory is expected to exist unless SaveSystem
  /// explicitly handles directory creation.
  ///
  /// Returns true if the save was successfully written.
  /// Returns false if serialization or file operations fail.
  ///
  /// The GameModel is not modified.
  static bool save(const GameModel &game, const std::filesystem::path &path);

  /// Loads game state from the specified save file.
  ///
  /// The existing GameModel is replaced only if the save file is
  /// successfully read, parsed, and validated.
  ///
  /// Returns true if loading succeeds.
  /// Returns false if the file is missing, invalid, unsupported,
  /// or cannot be read.
  ///
  /// On failure, the existing GameModel remains unchanged.
  static bool load(GameModel &game, const std::filesystem::path &path);
};
} // namespace farmstead
