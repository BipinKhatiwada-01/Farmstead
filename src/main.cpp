#include <SDL3/SDL.h>

#include <iostream>

#include "../include/farmstead/game_model.hpp"

namespace {

constexpr int kWindowWidth = 800;
constexpr int kWindowHeight = 648;
constexpr float kTileSize = 40.0f;
constexpr float kMapOffsetX = 0.0f;
constexpr float kMapOffsetY = 48.0f;
constexpr float kHudHeight = 48.0f;

void fillRect(SDL_Renderer *renderer, const SDL_FRect &rect,
              Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha = 255) {
    SDL_SetRenderDrawColor(renderer, red, green, blue, alpha);
    SDL_RenderFillRect(renderer, &rect);
}

SDL_FRect makeTileRect(int column, int row) {
    return SDL_FRect{
        kMapOffsetX + static_cast<float>(column) * kTileSize,
        kMapOffsetY + static_cast<float>(row) * kTileSize,
        kTileSize,
        kTileSize
    };
}

void drawCrop(SDL_Renderer *renderer, int column, int row,
              farmstead::CropStage stage, bool wateredToday) {
    const float tileX = kMapOffsetX + static_cast<float>(column) * kTileSize;
    const float tileY = kMapOffsetY + static_cast<float>(row) * kTileSize;

    switch (stage) {
    case farmstead::CropStage::SEED: {
        // A small golden seed in the centre of the soil.
        const SDL_FRect seed{
            tileX + 15.0f, tileY + 15.0f, 10.0f, 10.0f
        };
        fillRect(renderer, seed, 205, 166, 75);
        break;
    }

    case farmstead::CropStage::SPROUT: {
        // Stem first, then two small leaves.
        const SDL_FRect stem{
            tileX + 18.0f, tileY + 17.0f, 4.0f, 15.0f
        };
        const SDL_FRect leftLeaf{
            tileX + 9.0f, tileY + 15.0f, 12.0f, 6.0f
        };
        const SDL_FRect rightLeaf{
            tileX + 19.0f, tileY + 11.0f, 12.0f, 6.0f
        };
        fillRect(renderer, stem, 65, 115, 48);
        fillRect(renderer, leftLeaf, 75, 175, 75);
        fillRect(renderer, rightLeaf, 95, 195, 85);
        break;
    }

    case farmstead::CropStage::MATURE: {
        // A larger, blocky plant so the mature state is easy to distinguish.
        const SDL_FRect stem{
            tileX + 18.0f, tileY + 17.0f, 5.0f, 18.0f
        };
        const SDL_FRect leftLeaf{
            tileX + 5.0f, tileY + 17.0f, 16.0f, 10.0f
        };
        const SDL_FRect rightLeaf{
            tileX + 20.0f, tileY + 13.0f, 15.0f, 11.0f
        };
        const SDL_FRect topLeaf{
            tileX + 13.0f, tileY + 5.0f, 14.0f, 13.0f
        };
        const SDL_FRect produce{
            tileX + 24.0f, tileY + 27.0f, 8.0f, 8.0f
        };
        fillRect(renderer, stem, 65, 110, 45);
        fillRect(renderer, leftLeaf, 65, 155, 65);
        fillRect(renderer, rightLeaf, 85, 180, 70);
        fillRect(renderer, topLeaf, 105, 195, 75);
        fillRect(renderer, produce, 220, 175, 65);
        break;
    }

    case farmstead::CropStage::NONE:
    default:
        break;
    }

    if (wateredToday) {
        // Small blue marker in the top-right corner of a watered crop.
        const SDL_FRect waterMarker{
            tileX + kTileSize - 9.0f, tileY + 3.0f, 6.0f, 6.0f
        };
        fillRect(renderer, waterMarker, 65, 165, 235);
    }
}


void drawPlayer(SDL_Renderer *renderer, farmstead::TilePosition position,
                farmstead::Direction facing) {
    const float tileX = kMapOffsetX + static_cast<float>(position.column) * kTileSize;
    const float tileY = kMapOffsetY + static_cast<float>(position.row) * kTileSize;

    // Draw the character from small, deliberately layered rectangles. All
    // coordinates below are local to the 40x40 tile.
    const auto part = [&](float x, float y, float width, float height,
                          Uint8 red, Uint8 green, Uint8 blue) {
        const SDL_FRect rect{tileX + x, tileY + y, width, height};
        fillRect(renderer, rect, red, green, blue);
    };

    // A soft-looking stepped shadow grounds the character on the tile.
    part(10.0f, 33.0f, 20.0f, 3.0f, 23, 56, 38);
    part(7.0f, 31.0f, 26.0f, 3.0f, 29, 67, 43);

    // Backpack is visible from behind when the player faces upward.
    if (facing == farmstead::Direction::UP) {
        part(23.0f, 18.0f, 9.0f, 10.0f, 48, 39, 32);
        part(24.0f, 19.0f, 7.0f, 8.0f, 145, 86, 48);
        part(25.0f, 20.0f, 5.0f, 2.0f, 190, 128, 70);
    }

    // Trousers and sturdy brown boots.
    part(11.0f, 24.0f, 8.0f, 8.0f, 39, 54, 65);
    part(21.0f, 24.0f, 8.0f, 8.0f, 39, 54, 65);
    part(10.0f, 29.0f, 9.0f, 6.0f, 54, 39, 31);
    part(21.0f, 29.0f, 9.0f, 6.0f, 54, 39, 31);
    part(11.0f, 30.0f, 6.0f, 2.0f, 120, 83, 52);
    part(22.0f, 30.0f, 6.0f, 2.0f, 120, 83, 52);

    // Jacket silhouette, sleeves, and the lighter front panel.
    part(8.0f, 16.0f, 24.0f, 12.0f, 35, 50, 48);
    part(10.0f, 17.0f, 20.0f, 10.0f, 55, 112, 101);
    part(7.0f, 19.0f, 5.0f, 7.0f, 48, 91, 82);
    part(28.0f, 19.0f, 5.0f, 7.0f, 48, 91, 82);
    part(13.0f, 18.0f, 14.0f, 7.0f, 78, 139, 119);
    part(18.0f, 18.0f, 4.0f, 2.0f, 218, 190, 113); // collar
    part(19.0f, 21.0f, 2.0f, 2.0f, 232, 205, 143); // button

    // Head, hair and face. The hat brim overlaps the top of the face.
    part(12.0f, 7.0f, 16.0f, 14.0f, 48, 38, 34);   // hair outline
    if (facing == farmstead::Direction::UP) {
        part(13.0f, 10.0f, 14.0f, 10.0f, 83, 52, 39); // back of hair
        part(15.0f, 12.0f, 10.0f, 5.0f, 105, 66, 46);
    } else {
        part(13.0f, 10.0f, 14.0f, 10.0f, 226, 174, 132); // face
        part(12.0f, 11.0f, 3.0f, 6.0f, 83, 52, 39);      // side hair
        part(25.0f, 11.0f, 3.0f, 6.0f, 83, 52, 39);
    }
    part(12.0f, 7.0f, 16.0f, 4.0f, 65, 43, 34); // hair under hat

    // Straw hat: brim, crown, highlight and green band.
    part(8.0f, 8.0f, 24.0f, 5.0f, 126, 87, 43);
    part(10.0f, 5.0f, 20.0f, 6.0f, 211, 166, 83);
    part(13.0f, 3.0f, 14.0f, 3.0f, 231, 197, 112);
    part(13.0f, 9.0f, 14.0f, 2.0f, 89, 119, 65);
    part(10.0f, 11.0f, 20.0f, 2.0f, 172, 126, 61);

    // Tiny facial details change with facing direction.
    if (facing == farmstead::Direction::DOWN) {
        part(16.0f, 15.0f, 2.0f, 2.0f, 54, 43, 38);
        part(22.0f, 15.0f, 2.0f, 2.0f, 54, 43, 38);
        part(19.0f, 18.0f, 2.0f, 1.0f, 171, 104, 84);
    } else if (facing == farmstead::Direction::LEFT) {
        part(15.0f, 15.0f, 2.0f, 2.0f, 54, 43, 38);
        part(13.0f, 18.0f, 2.0f, 2.0f, 171, 104, 84);
    } else if (facing == farmstead::Direction::RIGHT) {
        part(23.0f, 15.0f, 2.0f, 2.0f, 54, 43, 38);
        part(25.0f, 18.0f, 2.0f, 2.0f, 171, 104, 84);
    }
}

void renderWorld(SDL_Renderer *renderer, const farmstead::GameModel &model,
                 int currentDay) {
    // Background behind the world and HUD.
    SDL_SetRenderDrawColor(renderer, 42, 61, 76, 255);
    SDL_RenderClear(renderer);

    // HUD panel.
    const SDL_FRect hudRect{0.0f, 0.0f,
                            static_cast<float>(kWindowWidth), kHudHeight};
    fillRect(renderer, hudRect, 25, 38, 48);

    // This is temporary prototype text. SDL_RenderDebugTextFormat uses an
    // 8x8 debug font; replace it with a proper font system during polish.
    SDL_SetRenderDrawColor(renderer, 240, 240, 225, 255);
    SDL_RenderDebugTextFormat(renderer, 12.0f, 7.0f,
                              "DAY: %d    PRODUCE: %d",
                              currentDay, model.produceCount());
    SDL_RenderDebugText(renderer, 12.0f, 25.0f,
                        "ARROWS: MOVE  T: TILL  P: PLANT  W: WATER  N: NEXT DAY  H: HARVEST  E: INSPECT");

    // Draw the map backgrounds first, then the crops on top of their soil.
    for (int row = 0; row < model.mapHeight(); ++row) {
        for (int column = 0; column < model.mapWidth(); ++column) {
            const farmstead::TileType type = model.tileAt(column, row);

            if (type == farmstead::TileType::OBSTACLE) {
                fillRect(renderer, makeTileRect(column, row), 10, 30, 20);
            }
            else if (model.isTilled(column, row)) {
                fillRect(renderer, makeTileRect(column, row), 130, 85, 50);
            }
            else {
                fillRect(renderer, makeTileRect(column, row), 30, 150, 80);
            }

            const farmstead::CropStage stage = model.cropStageAt(column, row);
            if (stage != farmstead::CropStage::NONE) {
                drawCrop(renderer, column, row, stage,
                         model.isWateredToday(column, row));
            }
        }
    }

    // Highlight the tile the player is currently facing.
    const farmstead::TilePosition target = model.tileInFront();
    if (model.isInBounds(target.column, target.row)) {
        SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255);
        const SDL_FRect targetRect = makeTileRect(target.column, target.row);
        SDL_RenderRect(renderer, &targetRect);
    }

    // Draw the player last so the character appears above the world and highlight.
    drawPlayer(renderer, model.playerPosition(), model.facingDirection());

    SDL_RenderPresent(renderer);
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

        renderWorld(renderPointer, model, currentDay);
    }

    SDL_DestroyRenderer(renderPointer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
