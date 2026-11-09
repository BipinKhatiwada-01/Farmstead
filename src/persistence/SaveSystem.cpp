#include "../../include/farmstead/SaveSystem.hpp"
#include "../../include/farmstead/GameSaveData.hpp"

#include <exception>
#include <filesystem>
#include <limits>
#include <nlohmann/json.hpp>

#include <fstream>
#include <stdexcept>
#include <system_error>
#include <utility>

namespace farmstead {

namespace {

using json = nlohmann::json;

int getInt(const json &value) {
  if (value.is_number_unsigned()) {
    const auto number = value.get<json::number_unsigned_t>();
    if (number > static_cast<json::number_unsigned_t>(
                     std::numeric_limits<int>::max())) {
      throw std::invalid_argument("Integer value is out of range");
    }
    return static_cast<int>(number);
  }

  if (!value.is_number_integer()) {
    throw std::invalid_argument("Expected an integer value");
  }

  const auto number = value.get<json::number_integer_t>();
  if (number < std::numeric_limits<int>::min() ||
      number > std::numeric_limits<int>::max()) {
    throw std::invalid_argument("Integer value is out of range");
  }
  return static_cast<int>(number);
}

class TemporaryFileCleanup {
public:
  explicit TemporaryFileCleanup(std::filesystem::path path)
      : path(std::move(path)) {}

  ~TemporaryFileCleanup() {
    std::error_code error;
    std::filesystem::remove(path, error);
  }

  TemporaryFileCleanup(const TemporaryFileCleanup &) = delete;
  TemporaryFileCleanup &operator=(const TemporaryFileCleanup &) = delete;

private:
  std::filesystem::path path;
};

std::string directionToString(Direction direction) {
  switch (direction) {
  case Direction::UP:
    return "UP";
  case Direction::DOWN:
    return "DOWN";
  case Direction::LEFT:
    return "LEFT";
  case Direction::RIGHT:
    return "RIGHT";
  }

  throw std::invalid_argument("Invalid direction");
}

Direction directionFromString(const std::string &value) {
  if (value == "UP")
    return Direction::UP;
  if (value == "DOWN")
    return Direction::DOWN;
  if (value == "LEFT")
    return Direction::LEFT;
  if (value == "RIGHT")
    return Direction::RIGHT;

  throw std::invalid_argument("Invalid direction");
}

json serialize(const GameSaveData &data) {
  json result;

  result["version"] = 1;
  result["day"] = data.day;

  result["player"] = json::object({{"column", data.playerPosition.column},
                                   {"row", data.playerPosition.row},
                                   {"facing", directionToString(data.facing)}});

  result["tilledTiles"] = json::array();

  for (const auto &tile : data.tilledTiles) {
    result["tilledTiles"].push_back(
        {{"column", tile.column}, {"row", tile.row}});
  }

  result["crops"] = json::array();

  for (const auto &crop : data.crops) {
    result["crops"].push_back({{"column", crop.position.column},
                               {"row", crop.position.row},
                               {"growthPoints", crop.growthPoints},
                               {"wateredToday", crop.wateredToday}});
  }

    result["inventory"] = json::object({{"produce", data.produce}});
    result["economy"] = json::object({{"coins", data.coins}});
  result["progression"] =
      json::object({{"totalCoinsEarned", data.totalCoinsEarned}});

  return result;
}

GameSaveData deserialize(const json &data) {
  GameSaveData result;

  if (getInt(data.at("version")) != 1) {
    throw std::invalid_argument("Unsupported save version");
  }

  result.day = getInt(data.at("day"));

  const auto &player = data.at("player");

  result.playerPosition = {getInt(player.at("column")),
                           getInt(player.at("row"))};

  result.facing = directionFromString(player.at("facing").get<std::string>());

  const auto &tilledTiles = data.at("tilledTiles");
  if (!tilledTiles.is_array()) {
    throw std::invalid_argument("Expected tilledTiles to be an array");
  }
  for (const auto &tile : tilledTiles) {
    result.tilledTiles.push_back({getInt(tile.at("column")),
                                  getInt(tile.at("row"))});
  }

  const auto &crops = data.at("crops");
  if (!crops.is_array()) {
    throw std::invalid_argument("Expected crops to be an array");
  }
  for (const auto &crop : crops) {
    result.crops.push_back(
        {{getInt(crop.at("column")), getInt(crop.at("row"))},
         getInt(crop.at("growthPoints")),
         crop.at("wateredToday").get<bool>()});
  }

  result.produce = getInt(data.at("inventory").at("produce"));
  result.coins = getInt(data.at("economy").at("coins"));

  result.totalCoinsEarned =
      getInt(data.at("progression").at("totalCoinsEarned"));

  return result;
}

} // namespace

bool SaveSystem::save(const GameModel &game,
                      const std::filesystem::path &path) {

  try {
    GameSaveData snapshot = game.exportSaveData();

    json data = serialize(snapshot);

    std::filesystem::path tempPath = path;
    tempPath += ".tmp";
    TemporaryFileCleanup cleanup(tempPath);

    {
      std::ofstream file(tempPath);

      if (!file.is_open()) {
        return false;
      }

      file << data.dump(4);
      file.flush();

      if (!file.good()) {
        return false;
      }

      file.close();
      if (file.fail()) {
        return false;
      }
    }
    std::filesystem::rename(tempPath, path);

    return true;
  } catch (const std::exception &) {
    return false;
  }
}

bool SaveSystem::load(GameModel &game, const std::filesystem::path &path) {
  try {
    std::ifstream file(path);

    if (!file.is_open()) {
      return false;
    }

    json data = json::parse(file);
    GameSaveData snapshot = deserialize(data);

    return game.restoreSaveData(snapshot);
  } catch (const std::exception &) {
    return false;
  }
}

} // namespace farmstead
