#include <SDL3/SDL.h>

#include <filesystem>
#include <iostream>
#include <string>
#include <utility>

#include "farmstead/audio_system.hpp"
#include "farmstead/game_model.hpp"
#include "farmstead/SaveSystem.hpp"
#include "farmstead/renderer.hpp"

namespace {

constexpr int kWindowWidth = 960;
constexpr int kWindowHeight = 800;

enum class ApplicationState {
    TITLE,
    PLAYING,
    PAUSED,
    VICTORY
};

enum class PauseAction {
    NONE,
    RESTART_FARM,
    RETURN_TO_TITLE,
    QUIT_GAME
};

constexpr int kTitleOptionCount = 3;
constexpr int kPauseOptionCount = 5;

// One local save slot for the current game. The directory is created when
// saving, and Continue is enabled only when this file can be loaded.
const std::filesystem::path kSavePath{"saves/slot1.json"};

int wrapSelection(int selection, int direction, int optionCount) {
    selection += direction;
    if (selection < 0) {
        selection = optionCount - 1;
    } else if (selection >= optionCount) {
        selection = 0;
    }
    return selection;
}

int wrapTitleSelection(int selection, int direction, bool hasSaveGame) {
    do {
        selection = wrapSelection(selection, direction, kTitleOptionCount);
    } while (!hasSaveGame && selection == 1); // Skip unavailable Continue.
    return selection;
}

bool isUsableSave(const std::filesystem::path& path) {
    std::error_code error;
    if (!std::filesystem::is_regular_file(path, error) || error) {
        return false;
    }

    // Validate the file before enabling Continue. Loading into a temporary
    // model prevents the title screen from mutating the active game.
    farmstead::GameModel probe;
    return farmstead::SaveSystem::load(probe, path);
}

} // namespace

int main() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "Unable to initialize video: " << SDL_GetError() << '\n';
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "Farmstead", kWindowWidth, kWindowHeight, 0);

    if (window == nullptr) {
        std::cerr << "Window failed to initialize: " << SDL_GetError() << '\n';
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderPointer = SDL_CreateRenderer(window, nullptr);
    if (renderPointer == nullptr) {
        std::cerr << "Unable to create renderer: " << SDL_GetError() << '\n';
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    if (!SDL_SetRenderVSync(renderPointer, 1)) {
        std::cerr << "VSync could not be enabled: " << SDL_GetError() << '\n';
    }

    // Keep the game's 800x664 logical layout and scale it to the 960x800 window.
    if (!SDL_SetRenderScale(renderPointer, 1.2f, 1.2f)) {
        std::cerr << "Unable to set render scale: " << SDL_GetError() << '\n';
        SDL_DestroyRenderer(renderPointer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    farmstead::AudioSystem audio;
    if (!audio.initialize()) {
        std::cerr << "Audio unavailable; continuing silently: " << SDL_GetError() << '\n';
    }

    farmstead::GameModel model;
    ApplicationState applicationState = ApplicationState::TITLE;
    PauseAction pendingPauseAction = PauseAction::NONE;

    bool hasSaveGame = isUsableSave(kSavePath);
    std::string titleMessage;
    int titleSelection = 0;
    int pauseSelection = 0;
    int confirmationSelection = 1; // Default to NO for destructive actions.

    bool running = true;
    std::string hudMessage;
    farmstead::render::HudMessageTone hudMessageTone =
        farmstead::render::HudMessageTone::INFO;

    const auto setHudMessage = [&](std::string message,
                                   farmstead::render::HudMessageTone tone) {
        hudMessage = std::move(message);
        hudMessageTone = tone;
    };

    const auto saveCurrentGame = [&]() {
        std::error_code directoryError;
        std::filesystem::create_directories(kSavePath.parent_path(),
                                            directoryError);
        if (directoryError) {
            setHudMessage("SAVE FAILED  -  COULD NOT CREATE SAVE DIRECTORY",
                          farmstead::render::HudMessageTone::WARNING);
            return false;
        }

        if (!farmstead::SaveSystem::save(model, kSavePath)) {
            setHudMessage("SAVE FAILED  -  YOUR PREVIOUS SAVE WAS KEPT",
                          farmstead::render::HudMessageTone::WARNING);
            return false;
        }

        hasSaveGame = true;
        setHudMessage("GAME SAVED SUCCESSFULLY",
                      farmstead::render::HudMessageTone::SUCCESS);
        return true;
    };

    const auto continueSavedGame = [&]() {
        farmstead::GameModel loadedModel;
        if (!farmstead::SaveSystem::load(loadedModel, kSavePath)) {
            hasSaveGame = false;
            titleSelection = 0;
            titleMessage = "SAVE COULD NOT BE LOADED";
            audio.play(farmstead::SoundEffect::MENU_MOVE);
            return false;
        }

        model = std::move(loadedModel);
        titleMessage.clear();
        setHudMessage("WELCOME BACK  -  DAY " +
                          std::to_string(model.currentDay()),
                      farmstead::render::HudMessageTone::SUCCESS);
        applicationState = ApplicationState::PLAYING;
        audio.play(farmstead::SoundEffect::MENU_CONFIRM);
        return true;
    };

    const auto startNewFarm = [&]() {
        model = farmstead::GameModel{};
        hudMessage.clear();
        hudMessageTone = farmstead::render::HudMessageTone::INFO;
        titleMessage.clear();
        applicationState = ApplicationState::PLAYING;
    };

    const auto beginPauseConfirmation = [&](PauseAction action) {
        pendingPauseAction = action;
        confirmationSelection = 1;
    };

    while (running) {
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
                continue;
            }

            if (event.type != SDL_EVENT_KEY_DOWN || event.key.repeat) {
                continue;
            }

            const SDL_Keycode key = event.key.key;

            switch (applicationState) {
            case ApplicationState::TITLE:
                if (key == SDLK_UP) {
                    titleSelection = wrapTitleSelection(titleSelection, -1,
                                                        hasSaveGame);
                    audio.play(farmstead::SoundEffect::MENU_MOVE);
                } else if (key == SDLK_DOWN) {
                    titleSelection = wrapTitleSelection(titleSelection, 1,
                                                        hasSaveGame);
                    audio.play(farmstead::SoundEffect::MENU_MOVE);
                } else if (key == SDLK_RETURN || key == SDLK_KP_ENTER ||
                           key == SDLK_SPACE) {
                    if (titleSelection == 0) {
                        audio.play(farmstead::SoundEffect::MENU_CONFIRM);
                        startNewFarm();
                    } else if (titleSelection == 1 && hasSaveGame) {
                        continueSavedGame();
                    } else if (titleSelection == 2) {
                        audio.play(farmstead::SoundEffect::MENU_CONFIRM);
                        running = false;
                    }
                } else if (key == SDLK_ESCAPE) {
                    audio.play(farmstead::SoundEffect::MENU_MOVE);
                    running = false;
                }
                break;

            case ApplicationState::PLAYING:
                if (key == SDLK_ESCAPE) {
                    audio.play(farmstead::SoundEffect::MENU_CONFIRM);
                    applicationState = ApplicationState::PAUSED;
                    pauseSelection = 0;
                    pendingPauseAction = PauseAction::NONE;
                    break;
                }

                switch (key) {
                case SDLK_RIGHT:
                case SDLK_LEFT:
                case SDLK_UP:
                case SDLK_DOWN: {
                    farmstead::Direction direction = farmstead::Direction::DOWN;
                    if (key == SDLK_RIGHT) direction = farmstead::Direction::RIGHT;
                    if (key == SDLK_LEFT)  direction = farmstead::Direction::LEFT;
                    if (key == SDLK_UP)    direction = farmstead::Direction::UP;

                    const bool moved = model.tryMove(direction);
                    if (moved) {
                        audio.play(farmstead::SoundEffect::FOOTSTEP);
                    }
                    if (model.isFacingMarket()) {
                        setHudMessage("MARKET COUNTER  -  PRESS S TO SELL ALL PRODUCE",
                                      farmstead::render::HudMessageTone::INFO);
                    } else if (model.isFacingFarmhouse()) {
                        setHudMessage("FRONT DOOR  -  PRESS B TO SLEEP",
                                      farmstead::render::HudMessageTone::INFO);
                    } else if (moved) {
                        hudMessage.clear();
                    } else {
                        setHudMessage("THAT WAY IS BLOCKED",
                                      farmstead::render::HudMessageTone::WARNING);
                    }
                    break;
                }

                case SDLK_E: {
                    // Inspect buildings and obstacles in front of the player.
                    // For ordinary ground, inspect the tile underfoot so the
                    // suggested farming action matches the T/P/W/H controls.
                    const farmstead::TilePosition frontTile = model.tileInFront();
                    if (model.isInBounds(frontTile.column, frontTile.row)) {
                        const farmstead::TileType frontType =
                            model.tileAt(frontTile.column, frontTile.row);
                        if (frontType == farmstead::TileType::MARKET) {
                            setHudMessage("MARKET COUNTER  -  PRESS S TO SELL ALL PRODUCE",
                                          farmstead::render::HudMessageTone::INFO);
                            break;
                        }
                        if (frontType == farmstead::TileType::FARMHOUSE) {
                            if (model.isFacingFarmhouse()) {
                                setHudMessage("FRONT DOOR  -  PRESS B TO SLEEP",
                                              farmstead::render::HudMessageTone::INFO);
                            } else {
                                setHudMessage("FARMHOUSE  -  FIND THE FRONT DOOR TO SLEEP",
                                              farmstead::render::HudMessageTone::INFO);
                            }
                            break;
                        }
                        if (frontType == farmstead::TileType::OBSTACLE) {
                            setHudMessage("A TREE BLOCKS THE WAY",
                                          farmstead::render::HudMessageTone::INFO);
                            break;
                        }
                    }

                    const farmstead::TilePosition target = model.playerPosition();
                    if (model.cropStageAt(target.column, target.row) ==
                        farmstead::CropStage::MATURE) {
                        setHudMessage("MATURE CROP  -  PRESS H TO HARVEST",
                                      farmstead::render::HudMessageTone::SUCCESS);
                    } else if (model.isPlanted(target.column, target.row)) {
                        setHudMessage(model.isWateredToday(target.column, target.row)
                                          ? "CROP WATERED TODAY"
                                          : "CROP NEEDS WATER  -  PRESS W",
                                      farmstead::render::HudMessageTone::INFO);
                    } else if (model.isTilled(target.column, target.row)) {
                        setHudMessage("TILLED SOIL  -  PRESS P TO PLANT",
                                      farmstead::render::HudMessageTone::INFO);
                    } else {
                        setHudMessage("OPEN GROUND  -  PRESS T TO TILL",
                                      farmstead::render::HudMessageTone::INFO);
                    }
                    break;
                }

                // Farming actions operate on the tile occupied by the player.
                case SDLK_T: {
                    const farmstead::TilePosition target = model.playerPosition();
                    if (model.tryTill(target.column, target.row)) {
                        audio.play(farmstead::SoundEffect::TILL);
                        setHudMessage("SOIL PREPARED  -  PRESS P TO PLANT",
                                      farmstead::render::HudMessageTone::SUCCESS);
                    } else {
                        setHudMessage("CANNOT TILL HERE",
                                      farmstead::render::HudMessageTone::WARNING);
                    }
                    break;
                }

                case SDLK_P: {
                    const farmstead::TilePosition target = model.playerPosition();
                    if (model.tryPlant(target.column, target.row)) {
                        audio.play(farmstead::SoundEffect::PLANT);
                        setHudMessage("SEED PLANTED  -  WATER IT TO HELP IT GROW",
                                      farmstead::render::HudMessageTone::SUCCESS);
                    } else {
                        setHudMessage("CANNOT PLANT HERE  -  PREPARE EMPTY SOIL FIRST",
                                      farmstead::render::HudMessageTone::WARNING);
                    }
                    break;
                }

                case SDLK_W: {
                    const farmstead::TilePosition target = model.playerPosition();
                    if (model.tryWater(target.column, target.row)) {
                        audio.play(farmstead::SoundEffect::WATER);
                        setHudMessage("CROP WATERED",
                                      farmstead::render::HudMessageTone::SUCCESS);
                    } else {
                        setHudMessage("NO WATERABLE CROP HERE",
                                      farmstead::render::HudMessageTone::WARNING);
                    }
                    break;
                }

                case SDLK_N:
                    // Development shortcut. Normal gameplay advances the day by sleeping.
                    model.advanceDay();
                    audio.play(farmstead::SoundEffect::SLEEP);
                    setHudMessage("DEVELOPER SHORTCUT  -  DAY " +
                                      std::to_string(model.currentDay()) + " BEGINS",
                                  farmstead::render::HudMessageTone::INFO);
                    break;

                case SDLK_B:
                    if (model.trySleep()) {
                        audio.play(farmstead::SoundEffect::SLEEP);
                        setHudMessage("YOU RESTED WELL  -  DAY " +
                                          std::to_string(model.currentDay()) + " BEGINS",
                                      farmstead::render::HudMessageTone::SUCCESS);
                    } else {
                        setHudMessage("FACE THE FARMHOUSE FRONT DOOR TO SLEEP",
                                      farmstead::render::HudMessageTone::WARNING);
                    }
                    break;

                case SDLK_H: {
                    const farmstead::TilePosition target = model.playerPosition();
                    if (model.tryHarvest(target.column, target.row)) {
                        audio.play(farmstead::SoundEffect::HARVEST);
                        setHudMessage("HARVESTED  -  PRODUCE ADDED TO YOUR BAG",
                                      farmstead::render::HudMessageTone::SUCCESS);
                    } else {
                        setHudMessage("CROP IS NOT READY TO HARVEST",
                                      farmstead::render::HudMessageTone::WARNING);
                    }
                    break;
                }

                case SDLK_S: {
                    const int quantity = model.produceCount();
                    if (quantity <= 0) {
                        setHudMessage("YOUR BAG IS EMPTY  -  HARVEST SOME PRODUCE FIRST",
                                      farmstead::render::HudMessageTone::WARNING);
                    } else if (!model.isFacingMarket()) {
                        setHudMessage("STAND AT THE MARKET COUNTER AND FACE THE STALL",
                                      farmstead::render::HudMessageTone::WARNING);
                    } else {
                        const int coinsBefore = model.coins();
                        if (model.trySellProduce(quantity)) {
                            const int saleValue = model.coins() - coinsBefore;
                            if (model.objectiveCompleted()) {
                                audio.play(farmstead::SoundEffect::OBJECTIVE_COMPLETE);
                                setHudMessage("OBJECTIVE COMPLETE  -  YOU EARNED " +
                                                  std::to_string(model.coinsEarned()) + " COINS",
                                              farmstead::render::HudMessageTone::SUCCESS);
                                applicationState = ApplicationState::VICTORY;
                            } else {
                                audio.play(farmstead::SoundEffect::SELL);
                                setHudMessage("SOLD " + std::to_string(quantity) +
                                                  " PRODUCE  -  EARNED " +
                                                  std::to_string(saleValue) + " COINS",
                                              farmstead::render::HudMessageTone::SUCCESS);
                            }
                        } else {
                            setHudMessage("SALE COULD NOT BE COMPLETED",
                                          farmstead::render::HudMessageTone::WARNING);
                        }
                    }
                    break;
                }

                default:
                    break;
                }
                break;

            case ApplicationState::PAUSED:
                if (pendingPauseAction != PauseAction::NONE) {
                    if (key == SDLK_ESCAPE) {
                        audio.play(farmstead::SoundEffect::MENU_MOVE);
                        pendingPauseAction = PauseAction::NONE;
                    } else if (key == SDLK_LEFT || key == SDLK_RIGHT ||
                               key == SDLK_UP || key == SDLK_DOWN) {
                        confirmationSelection = 1 - confirmationSelection;
                        audio.play(farmstead::SoundEffect::MENU_MOVE);
                    } else if (key == SDLK_RETURN || key == SDLK_KP_ENTER ||
                               key == SDLK_SPACE) {
                        audio.play(farmstead::SoundEffect::MENU_CONFIRM);
                        if (confirmationSelection == 1) {
                            pendingPauseAction = PauseAction::NONE;
                            break;
                        }

                        switch (pendingPauseAction) {
                        case PauseAction::RESTART_FARM:
                            startNewFarm();
                            pendingPauseAction = PauseAction::NONE;
                            break;
                        case PauseAction::RETURN_TO_TITLE:
                            model = farmstead::GameModel{};
                            hudMessage.clear();
                            titleSelection = 0;
                            applicationState = ApplicationState::TITLE;
                            pendingPauseAction = PauseAction::NONE;
                            break;
                        case PauseAction::QUIT_GAME:
                            running = false;
                            break;
                        case PauseAction::NONE:
                            break;
                        }
                    }
                    break;
                }

                if (key == SDLK_ESCAPE) {
                    audio.play(farmstead::SoundEffect::MENU_CONFIRM);
                    applicationState = ApplicationState::PLAYING;
                } else if (key == SDLK_UP) {
                    pauseSelection = wrapSelection(pauseSelection, -1,
                                                   kPauseOptionCount);
                    audio.play(farmstead::SoundEffect::MENU_MOVE);
                } else if (key == SDLK_DOWN) {
                    pauseSelection = wrapSelection(pauseSelection, 1,
                                                   kPauseOptionCount);
                    audio.play(farmstead::SoundEffect::MENU_MOVE);
                } else if (key == SDLK_RETURN || key == SDLK_KP_ENTER ||
                           key == SDLK_SPACE) {
                    audio.play(farmstead::SoundEffect::MENU_CONFIRM);
                    switch (pauseSelection) {
                    case 0:
                        applicationState = ApplicationState::PLAYING;
                        break;
                    case 1:
                        saveCurrentGame();
                        break;
                    case 2:
                        beginPauseConfirmation(PauseAction::RESTART_FARM);
                        break;
                    case 3:
                        beginPauseConfirmation(PauseAction::RETURN_TO_TITLE);
                        break;
                    case 4:
                        beginPauseConfirmation(PauseAction::QUIT_GAME);
                        break;
                    default:
                        break;
                    }
                }
                break;

            case ApplicationState::VICTORY:
                if (key == SDLK_RETURN || key == SDLK_KP_ENTER ||
                    key == SDLK_SPACE || key == SDLK_ESCAPE) {
                    audio.play(farmstead::SoundEffect::MENU_CONFIRM);
                    applicationState = ApplicationState::PLAYING;
                    hudMessage.clear();
                }
                break;
            }
        }

        switch (applicationState) {
        case ApplicationState::TITLE:
            audio.setAmbienceGain(0.45f);
            break;
        case ApplicationState::PLAYING:
            audio.setAmbienceGain(0.55f);
            break;
        case ApplicationState::PAUSED:
            audio.setAmbienceGain(0.27f);
            break;
        case ApplicationState::VICTORY:
            audio.setAmbienceGain(0.40f);
            break;
        }
        audio.update();

        switch (applicationState) {
        case ApplicationState::TITLE:
            farmstead::render::renderTitleScreen(
                renderPointer, titleSelection, hasSaveGame, titleMessage);
            break;

        case ApplicationState::PLAYING:
            farmstead::render::renderWorld(
                renderPointer, model, hudMessage, hudMessageTone);
            break;

        case ApplicationState::PAUSED: {
            std::string confirmationMessage;
            switch (pendingPauseAction) {
            case PauseAction::RESTART_FARM:
                confirmationMessage = "RESTART THIS FARM?";
                break;
            case PauseAction::RETURN_TO_TITLE:
                confirmationMessage = "RETURN TO THE TITLE SCREEN?";
                break;
            case PauseAction::QUIT_GAME:
                confirmationMessage = "QUIT FARMSTEAD?";
                break;
            case PauseAction::NONE:
                break;
            }

            farmstead::render::renderPauseMenu(
                renderPointer, model, pauseSelection,
                pendingPauseAction != PauseAction::NONE,
                confirmationMessage, confirmationSelection,
                hudMessage, hudMessageTone);
            break;
        }

        case ApplicationState::VICTORY:
            farmstead::render::renderVictoryOverlay(
                renderPointer, model, hudMessage);
            break;
        }
    }

    audio.shutdown();
    SDL_DestroyRenderer(renderPointer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
