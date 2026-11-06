#include <SDL3/SDL.h>

#include <iostream>
#include <string>
#include <utility>

#include "farmstead/game_model.hpp"
#include "farmstead/renderer.hpp"

namespace {

constexpr int kWindowWidth = 960;
constexpr int kWindowHeight = 800;

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

    farmstead::GameModel model;
    bool running = true;
    std::string hudMessage;
    farmstead::render::HudMessageTone hudMessageTone =
        farmstead::render::HudMessageTone::INFO;

    const auto setHudMessage = [&](std::string message,
                                   farmstead::render::HudMessageTone tone) {
        hudMessage = std::move(message);
        hudMessageTone = tone;
    };

    while (running) {
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
            else if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat) {
                switch (event.key.key) {
                case SDLK_RIGHT:
                case SDLK_LEFT:
                case SDLK_UP:
                case SDLK_DOWN: {
                    farmstead::Direction direction = farmstead::Direction::DOWN;
                    if (event.key.key == SDLK_RIGHT) direction = farmstead::Direction::RIGHT;
                    if (event.key.key == SDLK_LEFT)  direction = farmstead::Direction::LEFT;
                    if (event.key.key == SDLK_UP)    direction = farmstead::Direction::UP;

                    const bool moved = model.tryMove(direction);
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
                    setHudMessage("DEVELOPER SHORTCUT  -  DAY " +
                                      std::to_string(model.currentDay()) + " BEGINS",
                                  farmstead::render::HudMessageTone::INFO);
                    break;

                case SDLK_B:
                    if (model.trySleep()) {
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
                                setHudMessage("OBJECTIVE COMPLETE  -  YOU EARNED " +
                                                  std::to_string(model.coinsEarned()) + " COINS",
                                              farmstead::render::HudMessageTone::SUCCESS);
                            } else {
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
            }
        }

        farmstead::render::renderWorld(
            renderPointer, model, hudMessage, hudMessageTone);
    }

    SDL_DestroyRenderer(renderPointer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}