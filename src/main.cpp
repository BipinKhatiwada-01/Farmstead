#include <SDL3/SDL.h>

#include <iostream>

#include "../include/farmstead/game_model.hpp"
#include "../include/farmstead/renderer.hpp"

namespace {

constexpr int kWindowWidth = 800;
constexpr int kWindowHeight = 648;

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

    farmstead::GameModel model;
    int currentDay = 1;
    bool running = true;

    while (running) {
        SDL_Event event;

        // Handle all pending events before drawing the next frame.
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
            else if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat) {
                switch (event.key.key) {
                case SDLK_RIGHT:
                    model.tryMove(farmstead::Direction::RIGHT);
                    break;
                case SDLK_LEFT:
                    model.tryMove(farmstead::Direction::LEFT);
                    break;
                case SDLK_UP:
                    model.tryMove(farmstead::Direction::UP);
                    break;
                case SDLK_DOWN:
                    model.tryMove(farmstead::Direction::DOWN);
                    break;

                case SDLK_E: {
                    const farmstead::TilePosition target = model.tileInFront();
                    if (!model.isInBounds(target.column, target.row)) {
                        std::cout << "Nothing there - outside the map.\n";
                        break;
                    }

                    const farmstead::TileType type =
                        model.tileAt(target.column, target.row);
                    std::cout << "Tile (" << target.column << ", "
                              << target.row << "): ";
                    if (type == farmstead::TileType::GROUND) {
                        std::cout << "GROUND";
                        if (model.isTilled(target.column, target.row)) {
                            std::cout << ", TILLED";
                        }
                        const farmstead::CropStage stage =
                            model.cropStageAt(target.column, target.row);
                        if (stage == farmstead::CropStage::SEED) {
                            std::cout << ", SEED";
                        }
                        else if (stage == farmstead::CropStage::SPROUT) {
                            std::cout << ", SPROUT";
                        }
                        else if (stage == farmstead::CropStage::MATURE) {
                            std::cout << ", MATURE";
                        }
                        if (model.isWateredToday(target.column, target.row)) {
                            std::cout << ", WATERED TODAY";
                        }
                    }
                    else {
                        std::cout << "OBSTACLE";
                    }
                    std::cout << '\n';
                    break;
                }

                case SDLK_T: {
                    const farmstead::TilePosition target = model.tileInFront();
                    if (!model.isInBounds(target.column, target.row)) {
                        std::cout << "Cannot till: outside the map.\n";
                    }
                    else if (model.tryTill(target.column, target.row)) {
                        std::cout << "Successfully tilled tile ("
                                  << target.column << ", " << target.row << ").\n";
                    }
                    else {
                        std::cout << "Cannot till this tile. It may be an obstacle or already tilled.\n";
                    }
                    break;
                }

                case SDLK_P: {
                    const farmstead::TilePosition target = model.tileInFront();
                    if (!model.isInBounds(target.column, target.row)) {
                        std::cout << "Cannot plant: outside the map.\n";
                    }
                    else if (model.tryPlant(target.column, target.row)) {
                        std::cout << "Successfully planted a seed at ("
                                  << target.column << ", " << target.row << ").\n";
                    }
                    else {
                        std::cout << "Cannot plant here. The tile must be empty, tilled ground.\n";
                    }
                    break;
                }

                case SDLK_W: {
                    const farmstead::TilePosition target = model.tileInFront();
                    if (!model.isInBounds(target.column, target.row)) {
                        std::cout << "Cannot water: outside the map.\n";
                    }
                    else if (model.tryWater(target.column, target.row)) {
                        std::cout << "Watered crop at (" << target.column
                                  << ", " << target.row << ").\n";
                    }
                    else {
                        std::cout << "Cannot water here. There may be no crop, or it may already be watered today.\n";
                    }
                    break;
                }

                case SDLK_N:
                    model.advanceDay();
                    ++currentDay;
                    std::cout << "A new day begins. Day " << currentDay << ".\n";
                    break;

                case SDLK_H: {
                    const farmstead::TilePosition target = model.tileInFront();
                    if (!model.isInBounds(target.column, target.row)) {
                        std::cout << "Cannot harvest: outside the map.\n";
                    }
                    else if (model.tryHarvest(target.column, target.row)) {
                        std::cout << "Harvested crop! Produce: "
                                  << model.produceCount() << '\n';
                    }
                    else {
                        std::cout << "Cannot harvest. The tile needs a mature crop.\n";
                    }
                    break;
                }

                default:
                    break;
                }
            }
        }

        farmstead::render::renderWorld(renderPointer, model, currentDay);
    }

    SDL_DestroyRenderer(renderPointer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
