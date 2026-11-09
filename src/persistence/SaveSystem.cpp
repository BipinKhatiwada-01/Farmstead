#include "../../include/farmstead/SaveSystem.hpp"
#include "../../include/farmstead/GameSaveData.hpp"

#include <exception>
#include <filesystem>
#include <nlohmann/json.hpp>

#include <fstream>

namespace farmstead {

namespace {

using json = nlohmann::json;

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

  result["player"] = {{"column", data.playerPosition.column},
                      {"row", data.playerPosition.row},
                      {"facing", directionToString(data.facing)}};

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

  result["inventory"] = {"produce", data.produce};
  result["economy"] = {"coins", data.coins};
  result["progression"] = {"totalCoinsEarned", data.totalCoinsEarned};

  return result;
}

GameSaveData deserialize(const json &data) {
  GameSaveData result;

  if (data.at("version").get<int>() != 1) {
    throw std::invalid_argument("Unsupported save version");
  }

  result.day = data.at("day").get<int>();

  const auto &player = data.at("player");

  result.playerPosition = {player.at("column").get<int>(),
                           player.at("row").get<int>()};

  result.facing = directionFromString(data.at("facing").get<std::string>());

  for (const auto &tile : data.at("tilledTiles")) {
    result.tilledTiles.push_back(
        {tile.at("column").get<int>(), tile.at("row").get<int>()});
  }

  for (const auto &crop : data.at("crops")) {
    result.crops.push_back(
        {{crop.at("column").get<int>(), crop.at("row").get<int>()},
         crop.at("growthPoints").get<int>(),
         crop.at("wateredToday").get<bool>()});
  }

  result.produce = data.at("inventory").at("produce").get<int>();
  result.coins = data.at("economy").at("coins").get<int>();

  result.totalCoinsEarned =
      data.at("progression").at("totalCoinsEarned").get<int>();

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

    {
      std::ofstream file(tempPath);

      if (!file.is_open()) {
        return false;
      }

      file << data.dump(4);
      file.flush();

      if (!file.good()) {
        file.close();
        std::filesystem::remove(tempPath);
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
