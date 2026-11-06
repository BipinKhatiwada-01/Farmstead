#include "farmstead/renderer.hpp"

#include <algorithm>
#include <cmath>

namespace farmstead::render {
namespace {

constexpr int kWindowWidth = 800;
constexpr float kTileSize = 40.0f;
constexpr float kMapOffsetX = 0.0f;
constexpr float kMapOffsetY = 64.0f;
constexpr float kHudHeight = 64.0f;

void fillRect(SDL_Renderer* renderer, const SDL_FRect& rect,
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

bool isPathTile(int column, int row) {
    // A small worn track from the starting area towards the market.
    return (column == 1 && row >= 1 && row <= 5) ||
           (row == 5 && column >= 1 && column <= 9);
}

void drawGrassDetails(SDL_Renderer* renderer, int column, int row) {
    const float x = kMapOffsetX + static_cast<float>(column) * kTileSize;
    const float y = kMapOffsetY + static_cast<float>(row) * kTileSize;
    const int pattern = (column * 7 + row * 13) % 13;

    // Small, deterministic details: they never flicker between frames.
    if (pattern == 0 || pattern == 4) {
        const SDL_FRect bladeA{x + 9.0f, y + 25.0f, 2.0f, 5.0f};
        const SDL_FRect bladeB{x + 13.0f, y + 22.0f, 2.0f, 8.0f};
        const SDL_FRect bladeC{x + 17.0f, y + 26.0f, 2.0f, 4.0f};
        fillRect(renderer, bladeA, 66, 160, 83);
        fillRect(renderer, bladeB, 78, 174, 91);
        fillRect(renderer, bladeC, 57, 143, 73);
    } else if (pattern == 8) {
        const SDL_FRect flowerStem{x + 25.0f, y + 22.0f, 2.0f, 7.0f};
        const SDL_FRect flower{x + 23.0f, y + 19.0f, 6.0f, 5.0f};
        const SDL_FRect flowerCenter{x + 25.0f, y + 20.0f, 2.0f, 2.0f};
        fillRect(renderer, flowerStem, 57, 126, 66);
        fillRect(renderer, flower, 238, 218, 166);
        fillRect(renderer, flowerCenter, 220, 177, 76);
    }
}

void drawTree(SDL_Renderer* renderer, int column, int row) {
    const float x = kMapOffsetX + static_cast<float>(column) * kTileSize;
    const float y = kMapOffsetY + static_cast<float>(row) * kTileSize;
    const int variation = (column * 11 + row * 7) % 3;

    Uint8 canopyR = 35;
    Uint8 canopyG = 105;
    Uint8 canopyB = 57;
    if (variation == 1) {
        canopyR = 42; canopyG = 119; canopyB = 62;
    } else if (variation == 2) {
        canopyR = 29; canopyG = 91; canopyB = 53;
    }

    // Ground shadow and trunk.
    const SDL_FRect shadow{x + 6.0f, y + 31.0f, 28.0f, 6.0f};
    const SDL_FRect trunk{x + 17.0f, y + 23.0f, 7.0f, 13.0f};
    const SDL_FRect trunkLight{x + 18.0f, y + 25.0f, 2.0f, 8.0f};
    fillRect(renderer, shadow, 25, 71, 43);
    fillRect(renderer, trunk, 91, 59, 36);
    fillRect(renderer, trunkLight, 139, 91, 51);

    // Layered, stepped canopy instead of a featureless black collision tile.
    const SDL_FRect canopyOutline{x + 4.0f, y + 8.0f, 32.0f, 24.0f};
    const SDL_FRect canopyTop{x + 9.0f, y + 4.0f, 22.0f, 27.0f};
    const SDL_FRect canopyLeft{x + 6.0f, y + 13.0f, 28.0f, 16.0f};
    const SDL_FRect canopyMain{x + 8.0f, y + 8.0f, 24.0f, 22.0f};
    fillRect(renderer, canopyOutline, 20, 65, 39);
    fillRect(renderer, canopyTop, 26, 82, 45);
    fillRect(renderer, canopyLeft, canopyR, canopyG, canopyB);
    fillRect(renderer, canopyMain, canopyR, canopyG, canopyB);

    const SDL_FRect highlightA{x + 12.0f, y + 7.0f, 12.0f, 5.0f};
    const SDL_FRect highlightB{x + 8.0f, y + 14.0f, 5.0f, 7.0f};
    const SDL_FRect highlightC{x + 23.0f, y + 12.0f, 6.0f, 5.0f};
    fillRect(renderer, highlightA, 75, 151, 78);
    fillRect(renderer, highlightB, 57, 132, 68);
    fillRect(renderer, highlightC, 66, 143, 73);
}

void drawGroundTile(SDL_Renderer* renderer, int column, int row,
                    bool tilled, TileType type) {
    const SDL_FRect tile = makeTileRect(column, row);

    if (type == TileType::MARKET) {
        fillRect(renderer, tile, 126, 119, 91);
        const SDL_FRect edge{tile.x + 2.0f, tile.y + 2.0f, tile.w - 4.0f, 2.0f};
        fillRect(renderer, edge, 157, 147, 111);
        return;
    }

    if (tilled) {
        fillRect(renderer, tile, 130, 85, 50);
        const SDL_FRect furrowA{tile.x + 5.0f, tile.y + 11.0f, 30.0f, 2.0f};
        const SDL_FRect furrowB{tile.x + 5.0f, tile.y + 25.0f, 30.0f, 2.0f};
        fillRect(renderer, furrowA, 151, 103, 62);
        fillRect(renderer, furrowB, 107, 70, 43);
        return;
    }

    if (isPathTile(column, row)) {
        fillRect(renderer, tile, 151, 116, 76);
        const SDL_FRect wornCenter{tile.x + 2.0f, tile.y + 5.0f, 36.0f, 29.0f};
        fillRect(renderer, wornCenter, 160, 125, 83);
        const int speckle = (column * 5 + row * 3) % 4;
        const SDL_FRect stone{tile.x + 8.0f + 5.0f * static_cast<float>(speckle),
                              tile.y + 13.0f + 3.0f * static_cast<float>(speckle % 2), 3.0f, 2.0f};
        fillRect(renderer, stone, 190, 157, 111);
        return;
    }

    const int variation = (column * 5 + row * 3) % 4;
    static constexpr Uint8 greens[4][3] = {
        {43, 145, 79}, {39, 139, 75}, {47, 150, 82}, {36, 135, 72}
    };
    fillRect(renderer, tile, greens[variation][0], greens[variation][1], greens[variation][2]);
    drawGrassDetails(renderer, column, row);
}

void drawMarket(SDL_Renderer* renderer, int column, int row) {
    // The model uses one tile as the interaction point. The visible stall is
    // deliberately larger: three tiles wide and two tiles tall, positioned
    // immediately behind that customer-facing interaction tile.
    const float interactionX = kMapOffsetX + static_cast<float>(column) * kTileSize;
    const float interactionY = kMapOffsetY + static_cast<float>(row) * kTileSize;
    const float x = interactionX - kTileSize;
    const float y = interactionY - kTileSize;

    const auto part = [&](float px, float py, float width, float height,
                          Uint8 red, Uint8 green, Uint8 blue) {
        const SDL_FRect rect{x + px, y + py, width, height};
        fillRect(renderer, rect, red, green, blue);
    };

    // Broad shadow and wooden platform.
    part(7.0f, 72.0f, 106.0f, 5.0f, 31, 43, 31);
    part(4.0f, 63.0f, 112.0f, 10.0f, 83, 57, 39);
    part(8.0f, 64.0f, 104.0f, 2.0f, 145, 101, 63);

    // Sturdy timber frame.
    part(12.0f, 25.0f, 6.0f, 40.0f, 76, 48, 32);
    part(102.0f, 25.0f, 6.0f, 40.0f, 76, 48, 32);
    part(14.0f, 27.0f, 2.0f, 36.0f, 151, 99, 57);
    part(104.0f, 27.0f, 2.0f, 36.0f, 151, 99, 57);

    // Large red-and-cream striped awning.
    part(2.0f, 5.0f, 116.0f, 28.0f, 83, 47, 39);
    part(5.0f, 7.0f, 110.0f, 23.0f, 166, 76, 62);
    part(17.0f, 7.0f, 10.0f, 23.0f, 231, 205, 157);
    part(39.0f, 7.0f, 10.0f, 23.0f, 231, 205, 157);
    part(61.0f, 7.0f, 10.0f, 23.0f, 231, 205, 157);
    part(83.0f, 7.0f, 10.0f, 23.0f, 231, 205, 157);
    part(103.0f, 7.0f, 10.0f, 23.0f, 231, 205, 157);
    part(5.0f, 28.0f, 110.0f, 4.0f, 112, 55, 46);

    // Sign hanging below the canopy.
    part(39.0f, 34.0f, 42.0f, 13.0f, 73, 45, 31);
    part(42.0f, 36.0f, 36.0f, 9.0f, 224, 190, 124);
    part(48.0f, 39.0f, 24.0f, 2.0f, 115, 75, 42); // simple sign lettering

    // Wide wooden counter and its front panel.
    part(7.0f, 49.0f, 106.0f, 8.0f, 194, 145, 82);
    part(7.0f, 49.0f, 106.0f, 2.0f, 226, 183, 120);
    part(11.0f, 57.0f, 98.0f, 9.0f, 111, 69, 43);
    part(15.0f, 58.0f, 90.0f, 2.0f, 145, 91, 55);

    // Produce baskets across the counter.
    part(19.0f, 42.0f, 19.0f, 7.0f, 111, 69, 43);
    part(50.0f, 41.0f, 19.0f, 8.0f, 111, 69, 43);
    part(81.0f, 42.0f, 19.0f, 7.0f, 111, 69, 43);
    part(21.0f, 39.0f, 6.0f, 5.0f, 177, 61, 53);
    part(29.0f, 38.0f, 6.0f, 6.0f, 105, 145, 68);
    part(35.0f, 40.0f, 4.0f, 4.0f, 224, 181, 74);
    part(53.0f, 37.0f, 6.0f, 7.0f, 105, 145, 68);
    part(61.0f, 39.0f, 6.0f, 5.0f, 177, 61, 53);
    part(69.0f, 38.0f, 5.0f, 6.0f, 224, 181, 74);
    part(84.0f, 39.0f, 6.0f, 5.0f, 224, 181, 74);
    part(92.0f, 38.0f, 6.0f, 6.0f, 177, 61, 53);
}

void drawCrop(SDL_Renderer* renderer, int column, int row,
              CropStage stage, bool wateredToday) {
    const float tileX = kMapOffsetX + static_cast<float>(column) * kTileSize;
    const float tileY = kMapOffsetY + static_cast<float>(row) * kTileSize;

    switch (stage) {
    case CropStage::SEED: {
        // A small golden seed in the centre of the soil.
        const SDL_FRect seed{tileX + 15.0f, tileY + 15.0f, 10.0f, 10.0f};
        fillRect(renderer, seed, 205, 166, 75);
        break;
    }

    case CropStage::SPROUT: {
        // Stem first, then two small leaves.
        const SDL_FRect stem{tileX + 18.0f, tileY + 17.0f, 4.0f, 15.0f};
        const SDL_FRect leftLeaf{tileX + 9.0f, tileY + 15.0f, 12.0f, 6.0f};
        const SDL_FRect rightLeaf{tileX + 19.0f, tileY + 11.0f, 12.0f, 6.0f};
        fillRect(renderer, stem, 65, 115, 48);
        fillRect(renderer, leftLeaf, 75, 175, 75);
        fillRect(renderer, rightLeaf, 95, 195, 85);
        break;
    }

    case CropStage::MATURE: {
        // A larger, blocky plant so the mature state is easy to distinguish.
        const SDL_FRect stem{tileX + 18.0f, tileY + 17.0f, 5.0f, 18.0f};
        const SDL_FRect leftLeaf{tileX + 5.0f, tileY + 17.0f, 16.0f, 10.0f};
        const SDL_FRect rightLeaf{tileX + 20.0f, tileY + 13.0f, 15.0f, 11.0f};
        const SDL_FRect topLeaf{tileX + 13.0f, tileY + 5.0f, 14.0f, 13.0f};
        const SDL_FRect produce{tileX + 24.0f, tileY + 27.0f, 8.0f, 8.0f};
        fillRect(renderer, stem, 65, 110, 45);
        fillRect(renderer, leftLeaf, 65, 155, 65);
        fillRect(renderer, rightLeaf, 85, 180, 70);
        fillRect(renderer, topLeaf, 105, 195, 75);
        fillRect(renderer, produce, 220, 175, 65);
        break;
    }

    case CropStage::NONE:
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

void drawPlayer(SDL_Renderer* renderer, TilePosition position, Direction facing) {
    const float tileX = kMapOffsetX + static_cast<float>(position.column) * kTileSize;
    const float tileY = kMapOffsetY + static_cast<float>(position.row) * kTileSize;

    // Scale the character around the centre of the tile: slightly taller and slimmer.
    constexpr float kCenter = 20.0f;
    constexpr float kWidthScale = 0.82f;
    constexpr float kHeightScale = 1.12f;

    const auto part = [&](float x, float y, float width, float height,
                          Uint8 red, Uint8 green, Uint8 blue) {
        const float scaledX = std::round(kCenter + (x - kCenter) * kWidthScale);
        const float scaledY = std::round(kCenter + (y - kCenter) * kHeightScale);
        const SDL_FRect rect{
            tileX + scaledX,
            tileY + scaledY,
            std::round(width * kWidthScale),
            std::round(height * kHeightScale)
        };
        fillRect(renderer, rect, red, green, blue);
    };

    // Keep the shadow independent of the character scaling.
    const SDL_FRect shadowBack{tileX + 9.0f, tileY + 36.0f, 22.0f, 2.0f};
    const SDL_FRect shadowFront{tileX + 8.0f, tileY + 34.0f, 24.0f, 2.0f};
    fillRect(renderer, shadowBack, 23, 56, 38);
    fillRect(renderer, shadowFront, 29, 67, 43);

    // Backpack, visible from behind.
    if (facing == Direction::UP) {
        part(23.0f, 18.0f, 9.0f, 10.0f, 48, 39, 32);
        part(24.0f, 19.0f, 7.0f, 8.0f, 145, 86, 48);
        part(25.0f, 20.0f, 5.0f, 2.0f, 190, 128, 70);
    }

    // Trousers and boots.
    part(11.0f, 24.0f, 8.0f, 8.0f, 39, 54, 65);
    part(21.0f, 24.0f, 8.0f, 8.0f, 39, 54, 65);
    part(10.0f, 29.0f, 9.0f, 6.0f, 54, 39, 31);
    part(21.0f, 29.0f, 9.0f, 6.0f, 54, 39, 31);
    part(11.0f, 30.0f, 6.0f, 2.0f, 120, 83, 52);
    part(22.0f, 30.0f, 6.0f, 2.0f, 120, 83, 52);

    // Jacket silhouette and sleeves.
    part(8.0f, 16.0f, 24.0f, 12.0f, 35, 50, 48);
    part(10.0f, 17.0f, 20.0f, 10.0f, 55, 112, 101);
    part(7.0f, 19.0f, 5.0f, 7.0f, 48, 91, 82);
    part(28.0f, 19.0f, 5.0f, 7.0f, 48, 91, 82);
    part(13.0f, 18.0f, 14.0f, 7.0f, 78, 139, 119);
    part(18.0f, 18.0f, 4.0f, 2.0f, 218, 190, 113); // collar
    part(19.0f, 21.0f, 2.0f, 2.0f, 232, 205, 143); // button

    // Head, hair and face.
    part(12.0f, 7.0f, 16.0f, 14.0f, 48, 38, 34);
    if (facing == Direction::UP) {
        part(13.0f, 10.0f, 14.0f, 10.0f, 83, 52, 39);
        part(15.0f, 12.0f, 10.0f, 5.0f, 105, 66, 46);
    } else {
        part(13.0f, 10.0f, 14.0f, 10.0f, 226, 174, 132);
        part(12.0f, 11.0f, 3.0f, 6.0f, 83, 52, 39);
        part(25.0f, 11.0f, 3.0f, 6.0f, 83, 52, 39);
    }
    part(12.0f, 7.0f, 16.0f, 4.0f, 65, 43, 34);

    // Straw hat.
    part(8.0f, 8.0f, 24.0f, 5.0f, 126, 87, 43);
    part(10.0f, 5.0f, 20.0f, 6.0f, 211, 166, 83);
    part(13.0f, 3.0f, 14.0f, 3.0f, 231, 197, 112);
    part(13.0f, 9.0f, 14.0f, 2.0f, 89, 119, 65);
    part(10.0f, 11.0f, 20.0f, 2.0f, 172, 126, 61);

    // Facial details.
    if (facing == Direction::DOWN) {
        part(16.0f, 15.0f, 2.0f, 2.0f, 54, 43, 38);
        part(22.0f, 15.0f, 2.0f, 2.0f, 54, 43, 38);
        part(19.0f, 18.0f, 2.0f, 1.0f, 171, 104, 84);
    } else if (facing == Direction::LEFT) {
        part(15.0f, 15.0f, 2.0f, 2.0f, 54, 43, 38);
        part(13.0f, 18.0f, 2.0f, 2.0f, 171, 104, 84);
    } else if (facing == Direction::RIGHT) {
        part(23.0f, 15.0f, 2.0f, 2.0f, 54, 43, 38);
        part(25.0f, 18.0f, 2.0f, 2.0f, 171, 104, 84);
    }
}

} // namespace

void renderWorld(SDL_Renderer* renderer, const GameModel& model, int currentDay) {
    // Background and a two-tier HUD.
    SDL_SetRenderDrawColor(renderer, 31, 47, 43, 255);
    SDL_RenderClear(renderer);

    const SDL_FRect hudBackground{0.0f, 0.0f,
                                  static_cast<float>(kWindowWidth), kHudHeight};
    fillRect(renderer, hudBackground, 29, 43, 43);

    // Warm accent line gives the HUD a clear boundary from the world.
    const SDL_FRect hudAccent{0.0f, 61.0f, static_cast<float>(kWindowWidth), 3.0f};
    fillRect(renderer, hudAccent, 190, 143, 84);

    // Small farm mark and title.
    const SDL_FRect logoDark{10.0f, 8.0f, 20.0f, 20.0f};
    const SDL_FRect logoLeafA{14.0f, 10.0f, 6.0f, 11.0f};
    const SDL_FRect logoLeafB{19.0f, 13.0f, 6.0f, 8.0f};
    fillRect(renderer, logoDark, 57, 91, 62);
    fillRect(renderer, logoLeafA, 112, 176, 93);
    fillRect(renderer, logoLeafB, 76, 145, 78);

    SDL_SetRenderDrawColor(renderer, 239, 226, 193, 255);
    SDL_RenderDebugText(renderer, 36.0f, 14.0f, "FARMSTEAD");

    // Status cards.
    const SDL_FRect dayCard{124.0f, 7.0f, 72.0f, 23.0f};
    const SDL_FRect produceCard{202.0f, 7.0f, 118.0f, 23.0f};
    const SDL_FRect coinsCard{326.0f, 7.0f, 96.0f, 23.0f};
    const SDL_FRect goalCard{428.0f, 7.0f, 362.0f, 23.0f};
    fillRect(renderer, dayCard, 45, 62, 57);
    fillRect(renderer, produceCard, 45, 62, 57);
    fillRect(renderer, coinsCard, 45, 62, 57);
    fillRect(renderer, goalCard, 45, 62, 57);

    SDL_SetRenderDrawColor(renderer, 240, 230, 205, 255);
    SDL_RenderDebugTextFormat(renderer, 132.0f, 15.0f, "DAY %02d", currentDay);
    SDL_RenderDebugTextFormat(renderer, 210.0f, 15.0f,
                              "PRODUCE %d", model.produceCount());
    SDL_RenderDebugTextFormat(renderer, 334.0f, 15.0f,
                              "COINS %d", model.coins());
    SDL_RenderDebugText(renderer, 436.0f, 15.0f, "GOAL");

    // Objective progress bar and readable numeric progress.
    const int target = model.objectiveTarget();
    const int earned = model.coinsEarned();
    const float progress = target > 0
        ? std::clamp(static_cast<float>(earned) / static_cast<float>(target), 0.0f, 1.0f)
        : 0.0f;
    const SDL_FRect progressTrack{480.0f, 15.0f, 190.0f, 7.0f};
    const SDL_FRect progressFill{480.0f, 15.0f, 190.0f * progress, 7.0f};
    fillRect(renderer, progressTrack, 25, 35, 34);
    if (progress > 0.0f) {
        fillRect(renderer, progressFill, 207, 169, 91);
    }
    SDL_SetRenderDrawColor(renderer, 240, 230, 205, 255);
    if (model.objectiveCompleted()) {
        SDL_RenderDebugText(renderer, 681.0f, 15.0f, "COMPLETE");
    } else {
        SDL_RenderDebugTextFormat(renderer, 681.0f, 15.0f,
                                  "%d/%d", earned, target);
    }

    // Compact control strip. Text remains the built-in debug font; the paneling
    // and spacing do the visual work without introducing another dependency.
    const SDL_FRect controlsPanel{10.0f, 36.0f, 780.0f, 19.0f};
    fillRect(renderer, controlsPanel, 36, 52, 49);
    SDL_SetRenderDrawColor(renderer, 215, 210, 190, 255);
    SDL_RenderDebugText(renderer, 18.0f, 42.0f,
                        "ARROWS MOVE   T TILL   P PLANT   W WATER   N NEXT DAY   H HARVEST   S SELL   E INSPECT");

    // First pass: paint all terrain. Obstacles receive grass underneath their
    // tree artwork, so no solid black collision tiles remain visible.
    for (int row = 0; row < model.mapHeight(); ++row) {
        for (int column = 0; column < model.mapWidth(); ++column) {
            const TileType type = model.tileAt(column, row);
            if (type == TileType::OBSTACLE) {
                drawGroundTile(renderer, column, row, false, TileType::GROUND);
            } else {
                drawGroundTile(renderer, column, row,
                               model.isTilled(column, row), type);
            }
        }
    }

    // Second pass: trees and crops sit above the terrain.
    for (int row = 0; row < model.mapHeight(); ++row) {
        for (int column = 0; column < model.mapWidth(); ++column) {
            const TileType type = model.tileAt(column, row);
            if (type == TileType::OBSTACLE) {
                drawTree(renderer, column, row);
            }

            const CropStage stage = model.cropStageAt(column, row);
            if (stage != CropStage::NONE) {
                drawCrop(renderer, column, row, stage,
                         model.isWateredToday(column, row));
            }
        }
    }

    // Draw the market last because its canopy and counter span several tiles.
    const TilePosition marketPosition =
    GameModel::kMarketInteractionPosition;
    drawMarket(renderer, marketPosition.column, marketPosition.row);

    // Highlight the tile the player is currently facing.
    const TilePosition targetTile = model.tileInFront();
    if (model.isInBounds(targetTile.column, targetTile.row)) {
        SDL_SetRenderDrawColor(renderer, 246, 224, 160, 255);
        const SDL_FRect targetRect = makeTileRect(targetTile.column, targetTile.row);
        SDL_RenderRect(renderer, &targetRect);
    }

    drawPlayer(renderer, model.playerPosition(), model.facingDirection());
    SDL_RenderPresent(renderer);
}

} // namespace farmstead::render
