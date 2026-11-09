#include "catch2/catch_test_macros.hpp"
#include "farmstead/SaveSystem.hpp"

#include <catch2/catch_all.hpp>
#include <nlohmann/json.hpp>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <system_error>

using namespace farmstead;

namespace {

using json = nlohmann::json;

class TemporaryDirectory {
public:
  TemporaryDirectory() {
    static std::atomic<unsigned long long> nextId{0};
    const auto root = std::filesystem::temp_directory_path();

    for (int attempt = 0; attempt < 100; ++attempt) {
      const auto timestamp =
          std::chrono::steady_clock::now().time_since_epoch().count();
      directory = root / ("farmstead-save-tests-" + std::to_string(timestamp) +
                          "-" + std::to_string(nextId.fetch_add(1)));

      std::error_code error;
      if (std::filesystem::create_directory(directory, error)) {
        return;
      }
      if (error && error != std::errc::file_exists) {
        throw std::filesystem::filesystem_error(
            "Could not create temporary test directory", directory, error);
      }
    }

    throw std::runtime_error("Could not create a unique temporary directory");
  }

  ~TemporaryDirectory() {
    std::error_code error;
    std::filesystem::remove_all(directory, error);
  }

  TemporaryDirectory(const TemporaryDirectory &) = delete;
  TemporaryDirectory &operator=(const TemporaryDirectory &) = delete;

  std::filesystem::path file(const std::string &name) const {
    return directory / name;
  }

private:
  std::filesystem::path directory;
};

GameSaveData populatedSaveData() {
  GameSaveData data = GameModel{}.exportSaveData();
  data.day = 14;
  data.playerPosition = {8, 8};
  data.facing = Direction::LEFT;
  data.tilledTiles = {{3, 2}, {4, 2}, {6, 2}, {7, 2}};
  data.crops = {{{3, 2}, 2, true}, {{4, 2}, 3, false}, {{6, 2}, 6, true}};
  data.produce = 8;
  data.coins = 55;
  data.totalCoinsEarned = 130;
  return data;
}

GameModel populatedGameModel() {
  GameModel model;
  REQUIRE(model.restoreSaveData(populatedSaveData()));
  return model;
}

void requireSaveDataEqual(const GameSaveData &actual,
                          const GameSaveData &expected) {
  REQUIRE(actual.day == expected.day);
  REQUIRE(actual.playerPosition == expected.playerPosition);
  REQUIRE(actual.facing == expected.facing);
  REQUIRE(actual.tilledTiles == expected.tilledTiles);
  REQUIRE(actual.crops.size() == expected.crops.size());
  for (std::size_t index = 0; index < actual.crops.size(); ++index) {
    REQUIRE(actual.crops[index].position == expected.crops[index].position);
    REQUIRE(actual.crops[index].growthPoints ==
            expected.crops[index].growthPoints);
    REQUIRE(actual.crops[index].wateredToday ==
            expected.crops[index].wateredToday);
  }
  REQUIRE(actual.produce == expected.produce);
  REQUIRE(actual.coins == expected.coins);
  REQUIRE(actual.totalCoinsEarned == expected.totalCoinsEarned);
}

bool writeText(const std::filesystem::path &path, const std::string &contents) {
  std::ofstream file(path, std::ios::binary);
  if (!file.is_open()) {
    return false;
  }
  file << contents;
  file.close();
  return !file.fail();
}

bool writeJson(const std::filesystem::path &path, const json &contents) {
  return writeText(path, contents.dump(2));
}

json readJson(const std::filesystem::path &path) {
  std::ifstream file(path);
  return json::parse(file);
}

json validLoadJson() {
  const GameSaveData save = populatedSaveData();
  json data;
  data["version"] = 1;
  data["day"] = save.day;
  data["player"] = {{"column", save.playerPosition.column},
                    {"row", save.playerPosition.row},
                    {"facing", "LEFT"}};
  data["tilledTiles"] = json::array();
  for (const TilePosition &tile : save.tilledTiles) {
    data["tilledTiles"].push_back({{"column", tile.column}, {"row", tile.row}});
  }
  data["crops"] = json::array();
  for (const CropSaveData &crop : save.crops) {
    data["crops"].push_back({{"column", crop.position.column},
                             {"row", crop.position.row},
                             {"growthPoints", crop.growthPoints},
                             {"wateredToday", crop.wateredToday}});
  }
  data["inventory"] = {{"produce", save.produce}};
  data["economy"] = {{"coins", save.coins}};
  data["progression"] = {{"totalCoinsEarned", save.totalCoinsEarned}};
  return data;
}

void requireLoadFailsUnchanged(GameModel &model,
                               const std::filesystem::path &path) {
  const GameSaveData beforeLoad = model.exportSaveData();
  REQUIRE_FALSE(SaveSystem::load(model, path));
  requireSaveDataEqual(model.exportSaveData(), beforeLoad);
}

} // namespace

TEST_CASE("SaveSystem saves a default game as version one JSON") {
  TemporaryDirectory directory;
  const auto path = directory.file("default-save.json");
  const GameModel model;

  REQUIRE(SaveSystem::save(model, path));
  REQUIRE(std::filesystem::exists(path));
  REQUIRE(std::filesystem::is_regular_file(path));

  const json data = readJson(path);
  REQUIRE(data.at("version") == 1);
  REQUIRE(data.at("day") == 1);
  REQUIRE(data.at("player").at("column") == 1);
  REQUIRE(data.at("player").at("row") == 1);
  REQUIRE(data.at("player").at("facing") == "DOWN");
  REQUIRE(data.at("tilledTiles").is_array());
  REQUIRE(data.at("tilledTiles").empty());
  REQUIRE(data.at("crops").is_array());
  REQUIRE(data.at("crops").empty());
  CHECK(data.at("inventory").is_object());
  if (data.at("inventory").is_object()) {
    CHECK(data.at("inventory").at("produce") == 0);
  }
  CHECK(data.at("economy").is_object());
  if (data.at("economy").is_object()) {
    CHECK(data.at("economy").at("coins") == 0);
  }
  CHECK(data.at("progression").is_object());
  if (data.at("progression").is_object()) {
    CHECK(data.at("progression").at("totalCoinsEarned") == 0);
  }
}

TEST_CASE("SaveSystem saves modified gameplay and farming state") {
  TemporaryDirectory directory;
  const auto path = directory.file("populated-save.json");
  const GameModel model = populatedGameModel();

  REQUIRE(SaveSystem::save(model, path));
  REQUIRE(std::filesystem::is_regular_file(path));

  const json data = readJson(path);
  REQUIRE(data.at("version") == 1);
  REQUIRE(data.at("day") == 14);
  REQUIRE(data.at("player").at("column") == 8);
  REQUIRE(data.at("player").at("row") == 8);
  REQUIRE(data.at("player").at("facing") == "LEFT");
  REQUIRE(data.at("tilledTiles").size() == 4);
  REQUIRE(data.at("crops").size() == 3);
  REQUIRE(data.at("crops")[0].at("growthPoints") == 2);
  REQUIRE(data.at("crops")[0].at("wateredToday"));
  REQUIRE(data.at("crops")[1].at("growthPoints") == 3);
  REQUIRE_FALSE(data.at("crops")[1].at("wateredToday").get<bool>());
  REQUIRE(data.at("crops")[2].at("growthPoints") == 6);
  REQUIRE(data.at("crops")[2].at("wateredToday"));
  CHECK(data.at("inventory").is_object());
  if (data.at("inventory").is_object()) {
    CHECK(data.at("inventory").at("produce") == 8);
  }
  CHECK(data.at("economy").is_object());
  if (data.at("economy").is_object()) {
    CHECK(data.at("economy").at("coins") == 55);
  }
  CHECK(data.at("progression").is_object());
  if (data.at("progression").is_object()) {
    CHECK(data.at("progression").at("totalCoinsEarned") == 130);
  }
}

TEST_CASE("SaveSystem overwrites an existing save file") {
  TemporaryDirectory directory;
  const auto path = directory.file("overwrite-save.json");
  const GameModel defaultModel;
  const GameModel updatedModel = populatedGameModel();

  REQUIRE(SaveSystem::save(defaultModel, path));
  REQUIRE(readJson(path).at("day") == 1);
  REQUIRE(SaveSystem::save(updatedModel, path));

  const json data = readJson(path);
  REQUIRE(data.at("day") == 14);
  REQUIRE(data.at("player").at("column") == 8);
  CHECK(data.at("inventory").is_object());
  if (data.at("inventory").is_object()) {
    CHECK(data.at("inventory").at("produce") == 8);
  }
}

TEST_CASE("SaveSystem cleans up its temporary file when replacing fails") {
  TemporaryDirectory directory;
  const auto path = directory.file("save-target");
  REQUIRE(std::filesystem::create_directory(path));
  const GameModel model;

  REQUIRE_FALSE(SaveSystem::save(model, path));

  REQUIRE(std::filesystem::is_directory(path));
  for (const auto &entry : std::filesystem::directory_iterator(directory.file("."))) {
    REQUIRE(entry.path().filename() == path.filename());
  }
}

TEST_CASE("SaveSystem loads a valid version one JSON file") {
  TemporaryDirectory directory;
  const auto path = directory.file("valid-load.json");
  const GameSaveData expected = populatedSaveData();
  REQUIRE(writeJson(path, validLoadJson()));

  for (int attempt = 0; attempt < 3; ++attempt) {
    INFO("load attempt " << attempt + 1);
    GameModel loaded;
    REQUIRE(SaveSystem::load(loaded, path));

    requireSaveDataEqual(loaded.exportSaveData(), expected);
    REQUIRE(loaded.currentDay() == expected.day);
    REQUIRE(loaded.isWateredToday(3, 2));
    REQUIRE_FALSE(loaded.isWateredToday(4, 2));
    REQUIRE(loaded.isWateredToday(6, 2));
  }
}

TEST_CASE("SaveSystem loads and repeatedly round-trips the saved game") {
  TemporaryDirectory directory;
  const auto path = directory.file("round-trip.json");
  GameModel current = populatedGameModel();
  const GameSaveData expected = current.exportSaveData();

  for (int cycle = 0; cycle < 3; ++cycle) {
    INFO("save/load cycle " << cycle + 1);
    REQUIRE(SaveSystem::save(current, path));

    GameModel loaded;
    REQUIRE(SaveSystem::load(loaded, path));

    requireSaveDataEqual(loaded.exportSaveData(), expected);
    REQUIRE(loaded.currentDay() == expected.day);
    REQUIRE(loaded.isWateredToday(3, 2));
    REQUIRE_FALSE(loaded.isWateredToday(4, 2));
    REQUIRE(loaded.isWateredToday(6, 2));
    current = loaded;
  }
}

TEST_CASE("SaveSystem rejects invalid files without changing the game") {
  TemporaryDirectory directory;
  GameModel model = populatedGameModel();

  SECTION("a nonexistent path") {
    requireLoadFailsUnchanged(model, directory.file("missing.json"));
  }

  SECTION("an empty file") {
    const auto path = directory.file("empty.json");
    REQUIRE(writeText(path, ""));
    requireLoadFailsUnchanged(model, path);
  }

  SECTION("malformed JSON") {
    const auto path = directory.file("malformed.json");
    REQUIRE(writeText(path, "{ this is not valid JSON"));
    requireLoadFailsUnchanged(model, path);
  }

  SECTION("unsupported save version") {
    const auto path = directory.file("unsupported-version.json");
    json data = validLoadJson();
    data["version"] = 2;
    REQUIRE(writeJson(path, data));
    requireLoadFailsUnchanged(model, path);
  }

  SECTION("missing required JSON field") {
    const auto path = directory.file("missing-field.json");
    json data = validLoadJson();
    data.erase("progression");
    REQUIRE(writeJson(path, data));
    requireLoadFailsUnchanged(model, path);
  }

  SECTION("incorrect JSON value type") {
    const auto path = directory.file("incorrect-type.json");
    json data = validLoadJson();
    data["day"] = "fourteen";
    REQUIRE(writeJson(path, data));
    requireLoadFailsUnchanged(model, path);
  }

  SECTION("integer fields reject floating point values") {
    const auto path = directory.file("floating-point-day.json");
    json data = validLoadJson();
    data["day"] = 14.5;
    REQUIRE(writeJson(path, data));
    requireLoadFailsUnchanged(model, path);
  }

  SECTION("tilled tile and crop collections must be arrays") {
    const auto path = directory.file("incorrect-collections.json");
    json data = validLoadJson();
    data["tilledTiles"] = json::object();
    data["crops"] = json::object();
    REQUIRE(writeJson(path, data));
    requireLoadFailsUnchanged(model, path);
  }

  SECTION("negative crop growth points") {
    const auto path = directory.file("negative-growth.json");
    json data = validLoadJson();
    data["crops"][0]["growthPoints"] = -1;
    REQUIRE(writeJson(path, data));
    requireLoadFailsUnchanged(model, path);
  }

  SECTION("invalid player position") {
    const auto path = directory.file("invalid-player.json");
    json data = validLoadJson();
    data["player"]["column"] = GameModel::kMapWidth;
    REQUIRE(writeJson(path, data));
    requireLoadFailsUnchanged(model, path);
  }
}
