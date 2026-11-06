#include "farmstead/renderer.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <string_view>

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
           (row == 5 && column >= 1 && column <= 10);
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
        const SDL_FRect flowerTop{x + 24.0f, y + 18.0f, 4.0f, 3.0f};
        const SDL_FRect flowerLeft{x + 21.0f, y + 20.0f, 4.0f, 3.0f};
        const SDL_FRect flowerRight{x + 27.0f, y + 20.0f, 4.0f, 3.0f};
        const SDL_FRect flowerCenter{x + 25.0f, y + 20.0f, 2.0f, 2.0f};
        fillRect(renderer, flowerStem, 57, 126, 66);
        fillRect(renderer, flowerTop, 238, 218, 166);
        fillRect(renderer, flowerLeft, 238, 218, 166);
        fillRect(renderer, flowerRight, 238, 218, 166);
        fillRect(renderer, flowerCenter, 220, 177, 76);
    } else if (pattern == 10) {
        const SDL_FRect cloverStem{x + 13.0f, y + 25.0f, 2.0f, 6.0f};
        const SDL_FRect cloverLeft{x + 8.0f, y + 20.0f, 6.0f, 5.0f};
        const SDL_FRect cloverRight{x + 15.0f, y + 19.0f, 6.0f, 5.0f};
        const SDL_FRect cloverTop{x + 12.0f, y + 16.0f, 5.0f, 5.0f};
        fillRect(renderer, cloverStem, 47, 112, 58);
        fillRect(renderer, cloverLeft, 78, 154, 83);
        fillRect(renderer, cloverRight, 91, 170, 91);
        fillRect(renderer, cloverTop, 108, 184, 98);
    } else if (pattern == 12) {
        const SDL_FRect pebbleShadow{x + 25.0f, y + 28.0f, 7.0f, 3.0f};
        const SDL_FRect pebble{x + 24.0f, y + 26.0f, 6.0f, 3.0f};
        fillRect(renderer, pebbleShadow, 31, 105, 59);
        fillRect(renderer, pebble, 105, 151, 101);
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

    if (type == TileType::FARMHOUSE) {
        // Warm stone foundation beneath the farmhouse footprint.
        fillRect(renderer, tile, 119, 91, 66);
        const SDL_FRect inset{tile.x + 3.0f, tile.y + 3.0f,
                              tile.w - 6.0f, tile.h - 6.0f};
        fillRect(renderer, inset, 143, 111, 79);
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


void drawFarmhouse(SDL_Renderer* renderer, int column, int row) {
    // The model exposes the top-left footprint tile. The exterior is drawn as
    // one 3x3-tile structure; the bottom-middle footprint tile is its door target.
    const float x = kMapOffsetX + static_cast<float>(column) * kTileSize;
    const float y = kMapOffsetY + static_cast<float>(row) * kTileSize;

    const auto part = [&](float px, float py, float width, float height,
                          Uint8 red, Uint8 green, Uint8 blue) {
        const SDL_FRect rect{x + px, y + py, width, height};
        fillRect(renderer, rect, red, green, blue);
    };

    // Ground shadow and foundation.
    part(5.0f, 108.0f, 110.0f, 9.0f, 32, 54, 38);
    part(8.0f, 103.0f, 104.0f, 12.0f, 91, 70, 52);
    part(12.0f, 105.0f, 96.0f, 3.0f, 158, 123, 82);

    // Chimney sits behind the roof.
    part(87.0f, 0.0f, 19.0f, 36.0f, 60, 43, 36);
    part(90.0f, 3.0f, 13.0f, 31.0f, 139, 91, 67);
    part(87.0f, 0.0f, 19.0f, 5.0f, 79, 54, 43);
    part(92.0f, 10.0f, 9.0f, 2.0f, 174, 119, 84);

    // Main roof silhouette: stepped edges create a compact shingled profile.
    part(-2.0f, 22.0f, 124.0f, 39.0f, 57, 36, 31);
    part(7.0f, 13.0f, 106.0f, 15.0f, 57, 36, 31);
    part(17.0f, 5.0f, 86.0f, 14.0f, 57, 36, 31);

    part(1.0f, 23.0f, 118.0f, 32.0f, 132, 67, 52);
    part(9.0f, 15.0f, 102.0f, 12.0f, 148, 76, 57);
    part(20.0f, 7.0f, 80.0f, 11.0f, 157, 83, 61);

    // Roof ridge and shingle bands.
    part(28.0f, 8.0f, 63.0f, 3.0f, 187, 111, 76);
    part(12.0f, 19.0f, 95.0f, 2.0f, 181, 98, 70);
    part(5.0f, 35.0f, 110.0f, 2.0f, 103, 51, 43);
    part(5.0f, 52.0f, 110.0f, 7.0f, 84, 49, 39);
    part(8.0f, 53.0f, 104.0f, 2.0f, 187, 119, 82);

    // Warm timber walls, with a darker outline and corner posts.
    part(12.0f, 56.0f, 96.0f, 49.0f, 91, 57, 39);
    part(16.0f, 59.0f, 88.0f, 43.0f, 202, 159, 105);
    part(16.0f, 59.0f, 88.0f, 4.0f, 220, 181, 127);
    part(18.0f, 65.0f, 84.0f, 2.0f, 173, 126, 82);
    part(18.0f, 94.0f, 84.0f, 2.0f, 173, 126, 82);
    part(16.0f, 59.0f, 5.0f, 43.0f, 169, 119, 76);
    part(99.0f, 59.0f, 5.0f, 43.0f, 169, 119, 76);

    // Left window: cool glass with a warm wooden frame.
    part(22.0f, 70.0f, 25.0f, 23.0f, 83, 51, 37);
    part(25.0f, 73.0f, 19.0f, 17.0f, 76, 112, 116);
    part(26.0f, 74.0f, 17.0f, 5.0f, 117, 157, 151);
    part(33.0f, 73.0f, 3.0f, 17.0f, 184, 146, 99);
    part(21.0f, 69.0f, 27.0f, 3.0f, 226, 188, 131);
    part(21.0f, 92.0f, 27.0f, 3.0f, 112, 73, 49);

    // Right window: a tiny glimpse of the warm bedroom and bed inside.
    part(73.0f, 70.0f, 25.0f, 23.0f, 83, 51, 37);
    part(76.0f, 73.0f, 19.0f, 17.0f, 66, 82, 79);
    part(78.0f, 78.0f, 15.0f, 9.0f, 193, 157, 111); // bed frame / blanket
    part(78.0f, 76.0f, 6.0f, 5.0f, 239, 222, 184);  // pillow
    part(86.0f, 79.0f, 7.0f, 8.0f, 121, 151, 111);  // folded cover
    part(84.0f, 73.0f, 3.0f, 17.0f, 184, 146, 99);
    part(72.0f, 69.0f, 27.0f, 3.0f, 226, 188, 131);
    part(72.0f, 92.0f, 27.0f, 3.0f, 112, 73, 49);

    // Central front door aligns with the model's (16,7) interaction tile.
    part(49.0f, 70.0f, 22.0f, 35.0f, 76, 46, 33);
    part(52.0f, 73.0f, 16.0f, 29.0f, 126, 78, 48);
    part(54.0f, 76.0f, 12.0f, 2.0f, 163, 111, 67);
    part(54.0f, 82.0f, 12.0f, 2.0f, 163, 111, 67);
    part(54.0f, 88.0f, 12.0f, 2.0f, 163, 111, 67);
    part(64.0f, 87.0f, 2.0f, 2.0f, 222, 185, 116); // brass handle
    part(47.0f, 69.0f, 26.0f, 3.0f, 222, 185, 116);

    // A short stone doorstep below the door.
    part(43.0f, 105.0f, 34.0f, 7.0f, 149, 127, 101);
    part(46.0f, 106.0f, 28.0f, 2.0f, 193, 171, 137);
}

void drawCrop(SDL_Renderer* renderer, int column, int row,
              CropStage stage, bool wateredToday) {
    const float tileX = kMapOffsetX + static_cast<float>(column) * kTileSize;
    const float tileY = kMapOffsetY + static_cast<float>(row) * kTileSize;

    const auto part = [&](float x, float y, float width, float height,
                          Uint8 red, Uint8 green, Uint8 blue) {
        const SDL_FRect rect{tileX + x, tileY + y, width, height};
        fillRect(renderer, rect, red, green, blue);
    };

    switch (stage) {
    case CropStage::SEED: {
        // A dark seed shadow and a warm highlight give the tiny state more depth.
        part(14.0f, 18.0f, 12.0f, 6.0f, 85, 55, 39);
        part(16.0f, 16.0f, 9.0f, 7.0f, 181, 129, 62);
        part(18.0f, 16.0f, 5.0f, 3.0f, 230, 190, 105);
        break;
    }

    case CropStage::SPROUT: {
        // A small, readable seedling with separate leaf highlights.
        part(17.0f, 26.0f, 7.0f, 7.0f, 53, 91, 46);
        part(19.0f, 16.0f, 3.0f, 15.0f, 57, 112, 53);
        part(10.0f, 16.0f, 11.0f, 6.0f, 57, 132, 63);
        part(11.0f, 16.0f, 7.0f, 3.0f, 113, 186, 90);
        part(20.0f, 11.0f, 11.0f, 7.0f, 71, 153, 70);
        part(23.0f, 11.0f, 7.0f, 3.0f, 139, 201, 104);
        part(19.0f, 17.0f, 2.0f, 10.0f, 108, 160, 76);
        break;
    }

    case CropStage::MATURE: {
        // Layered leaves and a visible fruit make the harvest-ready state distinct.
        part(16.0f, 26.0f, 9.0f, 9.0f, 45, 86, 42);
        part(6.0f, 18.0f, 16.0f, 11.0f, 49, 119, 54);
        part(19.0f, 15.0f, 16.0f, 13.0f, 58, 139, 62);
        part(12.0f, 7.0f, 16.0f, 15.0f, 72, 157, 69);
        part(14.0f, 8.0f, 10.0f, 5.0f, 127, 190, 91);
        part(8.0f, 19.0f, 9.0f, 4.0f, 100, 171, 77);
        part(26.0f, 17.0f, 7.0f, 4.0f, 119, 188, 83);
        part(20.0f, 14.0f, 3.0f, 17.0f, 101, 160, 67);
        // Golden produce, with a short green stem and one bright pixel.
        part(25.0f, 27.0f, 9.0f, 8.0f, 167, 108, 39);
        part(26.0f, 25.0f, 7.0f, 8.0f, 226, 173, 65);
        part(28.0f, 25.0f, 3.0f, 2.0f, 247, 211, 116);
        part(28.0f, 23.0f, 3.0f, 3.0f, 71, 125, 51);
        break;
    }

    case CropStage::NONE:
    default:
        break;
    }

    if (wateredToday) {
        // Small pixel-art water droplet, more legible than a plain blue square.
        part(31.0f, 3.0f, 4.0f, 2.0f, 35, 103, 153);
        part(30.0f, 5.0f, 6.0f, 3.0f, 45, 142, 195);
        part(31.0f, 8.0f, 4.0f, 2.0f, 45, 142, 195);
        part(32.0f, 5.0f, 2.0f, 2.0f, 154, 217, 237);
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

void drawHud(SDL_Renderer* renderer, const GameModel& model,
             std::string_view message, HudMessageTone messageTone) {
    // Warm walnut, oak and parchment palette. The green remains only as a
    // restrained gameplay accent rather than the dominant HUD colour.
    constexpr Uint8 kWalnut[3] = {76, 48, 34};
    constexpr Uint8 kDarkWood[3] = {48, 31, 23};
    constexpr Uint8 kPanelWood[3] = {112, 72, 47};
    constexpr Uint8 kPanelBorder[3] = {177, 132, 82};
    constexpr Uint8 kWoodHighlight[3] = {190, 148, 99};
    constexpr Uint8 kParchment[3] = {245, 229, 198};
    constexpr Uint8 kBrass[3] = {204, 161, 91};

    const SDL_FRect background{0.0f, 0.0f, 800.0f, kHudHeight};
    fillRect(renderer, background, kWalnut[0], kWalnut[1], kWalnut[2]);

    // Subtle horizontal plank seams: enough to suggest timber without
    // competing with the information or turning the HUD into a texture.
    const SDL_FRect plankSeamA{0.0f, 32.0f, 800.0f, 1.0f};
    const SDL_FRect plankSeamB{0.0f, 33.0f, 800.0f, 1.0f};
    const SDL_FRect plankHighlight{0.0f, 34.0f, 800.0f, 1.0f};
    fillRect(renderer, plankSeamA, 62, 39, 28);
    fillRect(renderer, plankSeamB, 91, 58, 39);
    fillRect(renderer, plankHighlight, 94, 62, 41);

    const SDL_FRect lowerRail{0.0f, 60.0f, 800.0f, 4.0f};
    fillRect(renderer, lowerRail, kDarkWood[0], kDarkWood[1], kDarkWood[2]);
    const SDL_FRect brassEdge{0.0f, 60.0f, 800.0f, 1.0f};
    fillRect(renderer, brassEdge, kBrass[0], kBrass[1], kBrass[2]);

    const auto panel = [&](const SDL_FRect& rect) {
        const SDL_FRect shadow{rect.x + 1.0f, rect.y + 2.0f,
                               rect.w, rect.h};
        fillRect(renderer, shadow, kDarkWood[0], kDarkWood[1], kDarkWood[2]);
        fillRect(renderer, rect, kPanelWood[0], kPanelWood[1], kPanelWood[2]);
        SDL_SetRenderDrawColor(renderer, kPanelBorder[0], kPanelBorder[1],
                               kPanelBorder[2], 255);
        SDL_RenderRect(renderer, &rect);

        const SDL_FRect topEdge{rect.x + 2.0f, rect.y + 2.0f,
                                rect.w - 4.0f, 1.0f};
        const SDL_FRect bottomEdge{rect.x + 2.0f, rect.y + rect.h - 3.0f,
                                   rect.w - 4.0f, 1.0f};
        fillRect(renderer, topEdge, kWoodHighlight[0],
                 kWoodHighlight[1], kWoodHighlight[2]);
        fillRect(renderer, bottomEdge, 74, 47, 32);
    };

    // Title plaque: a deeper mahogany shade distinguishes the game identity.
    const SDL_FRect titlePlaque{10.0f, 7.0f, 116.0f, 25.0f};
    panel(titlePlaque);
    const SDL_FRect titleInset{12.0f, 9.0f, 112.0f, 21.0f};
    fillRect(renderer, titleInset, 83, 51, 33);
    const SDL_FRect logoStem{21.0f, 14.0f, 2.0f, 12.0f};
    const SDL_FRect logoLeafA{14.0f, 11.0f, 8.0f, 5.0f};
    const SDL_FRect logoLeafB{22.0f, 14.0f, 7.0f, 5.0f};
    fillRect(renderer, logoStem, 183, 157, 103);
    fillRect(renderer, logoLeafA, 121, 174, 104);
    fillRect(renderer, logoLeafB, 83, 143, 78);
    SDL_SetRenderDrawColor(renderer, kParchment[0], kParchment[1],
                           kParchment[2], 255);
    SDL_RenderDebugText(renderer, 35.0f, 16.0f, "FARMSTEAD");

    // Status panels share a consistent oak finish and parchment lettering.
    const SDL_FRect dayPanel{132.0f, 7.0f, 67.0f, 25.0f};
    const SDL_FRect bagPanel{205.0f, 7.0f, 91.0f, 25.0f};
    const SDL_FRect coinPanel{302.0f, 7.0f, 94.0f, 25.0f};
    const SDL_FRect goalPanel{402.0f, 7.0f, 388.0f, 25.0f};
    panel(dayPanel);
    panel(bagPanel);
    panel(coinPanel);
    panel(goalPanel);

    SDL_SetRenderDrawColor(renderer, kParchment[0], kParchment[1],
                           kParchment[2], 255);
    SDL_RenderDebugTextFormat(renderer, 140.0f, 16.0f, "DAY %02d", model.currentDay());
    SDL_RenderDebugTextFormat(renderer, 213.0f, 16.0f, "BAG %d", model.produceCount());
    SDL_RenderDebugTextFormat(renderer, 310.0f, 16.0f, "COINS %d", model.coins());
    SDL_RenderDebugText(renderer, 411.0f, 16.0f, "GOAL");

    const int target = model.objectiveTarget();
    const int earned = model.coinsEarned();
    const float progress = target > 0
        ? std::clamp(static_cast<float>(earned) / static_cast<float>(target), 0.0f, 1.0f)
        : 0.0f;

    const SDL_FRect progressTrack{454.0f, 16.0f, 235.0f, 7.0f};
    const SDL_FRect progressFill{454.0f, 16.0f, 235.0f * progress, 7.0f};
    fillRect(renderer, progressTrack, 51, 33, 24);
    // A warm harvest-gold progress fill; completion gets a muted leaf-green accent.
    if (progress > 0.0f) {
        fillRect(renderer, progressFill,
                 model.objectiveCompleted() ? 132 : 218,
                 model.objectiveCompleted() ? 166 : 166,
                 model.objectiveCompleted() ? 105 : 83);
    }
    SDL_SetRenderDrawColor(renderer, kParchment[0], kParchment[1],
                           kParchment[2], 255);
    SDL_RenderDebugTextFormat(renderer, 700.0f, 16.0f, "%d/%d", earned, target);

    // Message rail: darker stained wood with a tone-coloured status marker.
    const SDL_FRect messagePanel{10.0f, 37.0f, 780.0f, 18.0f};
    fillRect(renderer, messagePanel, 83, 53, 37);
    SDL_SetRenderDrawColor(renderer, 159, 116, 73, 255);
    SDL_RenderRect(renderer, &messagePanel);

    std::string visibleMessage;
    if (message.empty()) {
        if (model.isFacingMarket()) {
            visibleMessage = model.produceCount() > 0
                ? "MARKET COUNTER  -  S: SELL ALL PRODUCE"
                : "MARKET COUNTER  -  YOUR BAG IS EMPTY";
        } else if (model.isFacingFarmhouse()) {
            visibleMessage = "FARMHOUSE FRONT DOOR  -  B: REST FOR THE NIGHT";
        } else {
            visibleMessage = "ARROWS MOVE   T TILL   P PLANT   W WATER   H HARVEST   S SELL   B SLEEP   E INSPECT";
        }
    } else {
        visibleMessage.assign(message);
    }

    constexpr std::size_t kMaxMessageLength = 92;
    if (visibleMessage.size() > kMaxMessageLength) {
        visibleMessage.resize(kMaxMessageLength);
    }

    Uint8 messageR = kParchment[0];
    Uint8 messageG = kParchment[1];
    Uint8 messageB = kParchment[2];
    switch (messageTone) {
    case HudMessageTone::SUCCESS:
        messageR = 177; messageG = 211; messageB = 143;
        break;
    case HudMessageTone::WARNING:
        messageR = 235; messageG = 157; messageB = 116;
        break;
    case HudMessageTone::INFO:
    default:
        break;
    }

    const SDL_FRect statusMark{17.0f, 42.0f, 3.0f, 8.0f};
    fillRect(renderer, statusMark, messageR, messageG, messageB);
    SDL_SetRenderDrawColor(renderer, messageR, messageG, messageB, 255);
    SDL_RenderDebugText(renderer, 27.0f, 42.0f, visibleMessage.c_str());
}

} // namespace

namespace {

void drawWorldFrame(SDL_Renderer* renderer, const GameModel& model,
                    std::string_view message, HudMessageTone messageTone) {
    SDL_SetRenderDrawColor(renderer, 31, 47, 43, 255);
    SDL_RenderClear(renderer);

    drawHud(renderer, model, message, messageTone);

    // First pass: terrain. Market and farmhouse tiles use their own foundations.
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

    // Draw each multi-tile building once; their model footprints handle collision.
    const TilePosition marketPosition = GameModel::kMarketInteractionPosition;
    drawMarket(renderer, marketPosition.column, marketPosition.row);

    const TilePosition farmhouseTopLeft = GameModel::kFarmhouseFootprint.front();
    drawFarmhouse(renderer, farmhouseTopLeft.column, farmhouseTopLeft.row);

    // The selection outline sits under the player sprite.
    const TilePosition playerTile = model.playerPosition();
    if (model.isInBounds(playerTile.column, playerTile.row)) {
        SDL_SetRenderDrawColor(renderer, 246, 224, 160, 255);
        const SDL_FRect playerTileRect = makeTileRect(playerTile.column, playerTile.row);
        SDL_RenderRect(renderer, &playerTileRect);
    }

    drawPlayer(renderer, playerTile, model.facingDirection());
}

void drawWoodPanel(SDL_Renderer* renderer, const SDL_FRect& rect) {
    const SDL_FRect shadow{rect.x + 5.0f, rect.y + 6.0f, rect.w, rect.h};
    const SDL_FRect inset{rect.x + 5.0f, rect.y + 5.0f,
                          rect.w - 10.0f, rect.h - 10.0f};
    const SDL_FRect topHighlight{rect.x + 8.0f, rect.y + 7.0f,
                                 rect.w - 16.0f, 2.0f};
    const SDL_FRect innerBorder{rect.x + 9.0f, rect.y + 9.0f,
                                rect.w - 18.0f, rect.h - 18.0f};

    fillRect(renderer, shadow, 20, 19, 16);
    fillRect(renderer, rect, 43, 28, 21);
    fillRect(renderer, inset, 105, 67, 43);
    SDL_SetRenderDrawColor(renderer, 213, 174, 112, 255);
    SDL_RenderRect(renderer, &rect);
    SDL_SetRenderDrawColor(renderer, 148, 99, 61, 255);
    SDL_RenderRect(renderer, &innerBorder);
    fillRect(renderer, topHighlight, 207, 165, 109);

    // Fine plank lines make the panel feel like timber without adding noise.
    const SDL_FRect grainA{rect.x + 14.0f, rect.y + rect.h * 0.35f,
                           rect.w - 28.0f, 1.0f};
    const SDL_FRect grainB{rect.x + 14.0f, rect.y + rect.h * 0.72f,
                           rect.w - 28.0f, 1.0f};
    fillRect(renderer, grainA, 113, 73, 47);
    fillRect(renderer, grainB, 119, 77, 49);

    const SDL_FRect lowerRail{rect.x + 8.0f, rect.y + rect.h - 11.0f,
                              rect.w - 16.0f, 2.0f};
    fillRect(renderer, lowerRail, 69, 43, 29);

    // Four small brass pegs reinforce the handcrafted plaque aesthetic.
    const SDL_FRect pegTL{rect.x + 5.0f, rect.y + 5.0f, 3.0f, 3.0f};
    const SDL_FRect pegTR{rect.x + rect.w - 8.0f, rect.y + 5.0f, 3.0f, 3.0f};
    const SDL_FRect pegBL{rect.x + 5.0f, rect.y + rect.h - 8.0f, 3.0f, 3.0f};
    const SDL_FRect pegBR{rect.x + rect.w - 8.0f, rect.y + rect.h - 8.0f, 3.0f, 3.0f};
    fillRect(renderer, pegTL, 224, 188, 127);
    fillRect(renderer, pegTR, 224, 188, 127);
    fillRect(renderer, pegBL, 167, 124, 78);
    fillRect(renderer, pegBR, 167, 124, 78);
}

void drawMenuOption(SDL_Renderer* renderer, const SDL_FRect& rect,
                    const char* label, bool selected) {
    if (selected) {
        const SDL_FRect outerGlow{rect.x - 2.0f, rect.y - 2.0f,
                                  rect.w + 4.0f, rect.h + 4.0f};
        fillRect(renderer, outerGlow, 207, 165, 104);
        fillRect(renderer, rect, 71, 43, 29);
        const SDL_FRect inner{rect.x + 2.0f, rect.y + 2.0f,
                              rect.w - 4.0f, rect.h - 4.0f};
        fillRect(renderer, inner, 135, 83, 47);
        const SDL_FRect topEdge{rect.x + 4.0f, rect.y + 3.0f,
                                rect.w - 8.0f, 2.0f};
        fillRect(renderer, topEdge, 180, 120, 69);
        SDL_SetRenderDrawColor(renderer, 247, 218, 169, 255);
        SDL_RenderRect(renderer, &rect);
        const SDL_FRect markerStem{rect.x + 11.0f, rect.y + 9.0f, 2.0f, 13.0f};
        const SDL_FRect markerLeaf{rect.x + 8.0f, rect.y + 8.0f, 5.0f, 4.0f};
        fillRect(renderer, markerStem, 231, 199, 132);
        fillRect(renderer, markerLeaf, 157, 190, 116);
        SDL_SetRenderDrawColor(renderer, 255, 239, 208, 255);
    } else {
        fillRect(renderer, rect, 76, 49, 33);
        const SDL_FRect topEdge{rect.x + 3.0f, rect.y + 2.0f,
                                rect.w - 6.0f, 1.0f};
        fillRect(renderer, topEdge, 122, 82, 53);
        SDL_SetRenderDrawColor(renderer, 187, 155, 119, 255);
        SDL_RenderRect(renderer, &rect);
        SDL_SetRenderDrawColor(renderer, 236, 216, 184, 255);
    }

    SDL_RenderDebugText(renderer, rect.x + 27.0f, rect.y + 10.0f, label);
}

void drawTitleCloud(SDL_Renderer* renderer, float x, float y, float scale) {
    // Tiny block clouds; intentionally low contrast so the title plaque remains focal.
    const auto cloudPart = [&](float px, float py, float width, float height,
                               Uint8 red, Uint8 green, Uint8 blue) {
        const SDL_FRect rect{x + px * scale, y + py * scale,
                             width * scale, height * scale};
        fillRect(renderer, rect, red, green, blue);
    };
    cloudPart(4.0f, 7.0f, 42.0f, 11.0f, 106, 129, 105);
    cloudPart(13.0f, 1.0f, 25.0f, 13.0f, 117, 139, 112);
    cloudPart(23.0f, 6.0f, 34.0f, 12.0f, 106, 129, 105);
    cloudPart(10.0f, 16.0f, 45.0f, 3.0f, 91, 117, 91);
}

void drawTitleFence(SDL_Renderer* renderer, float x, float y, int posts) {
    const SDL_FRect upperRail{x, y + 8.0f, static_cast<float>(posts - 1) * 25.0f, 3.0f};
    const SDL_FRect lowerRail{x, y + 22.0f, static_cast<float>(posts - 1) * 25.0f, 3.0f};
    fillRect(renderer, upperRail, 119, 79, 48);
    fillRect(renderer, lowerRail, 93, 61, 39);
    for (int i = 0; i < posts; ++i) {
        const float postX = x + static_cast<float>(i) * 25.0f;
        const SDL_FRect post{postX, y, 5.0f, 31.0f};
        const SDL_FRect cap{postX - 1.0f, y - 2.0f, 7.0f, 4.0f};
        fillRect(renderer, post, 126, 84, 51);
        fillRect(renderer, cap, 157, 108, 68);
        const SDL_FRect grain{postX + 1.0f, y + 7.0f, 1.0f, 17.0f};
        fillRect(renderer, grain, 166, 119, 77);
    }
}

void drawTitleFlowerPatch(SDL_Renderer* renderer, float x, float y) {
    const SDL_FRect grassA{x, y + 7.0f, 30.0f, 3.0f};
    const SDL_FRect grassB{x + 8.0f, y + 4.0f, 2.0f, 6.0f};
    const SDL_FRect grassC{x + 18.0f, y + 2.0f, 2.0f, 8.0f};
    fillRect(renderer, grassA, 45, 107, 57);
    fillRect(renderer, grassB, 72, 153, 78);
    fillRect(renderer, grassC, 72, 153, 78);
    const SDL_FRect flowerA{x + 5.0f, y, 4.0f, 3.0f};
    const SDL_FRect flowerB{x + 19.0f, y - 1.0f, 4.0f, 3.0f};
    fillRect(renderer, flowerA, 236, 211, 156);
    fillRect(renderer, flowerB, 231, 197, 142);
}

void drawDimOverlay(SDL_Renderer* renderer) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    const SDL_FRect veil{0.0f, 0.0f, 800.0f, 664.0f};
    fillRect(renderer, veil, 20, 23, 19, 178);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

} // namespace

void renderWorld(SDL_Renderer* renderer, const GameModel& model,
                 std::string_view message, HudMessageTone messageTone) {
    drawWorldFrame(renderer, model, message, messageTone);
    SDL_RenderPresent(renderer);
}

void renderTitleScreen(SDL_Renderer* renderer, int selectedOption) {
    SDL_SetRenderDrawColor(renderer, 30, 49, 42, 255);
    SDL_RenderClear(renderer);

    // Layered dusk sky, low hills and a cultivated foreground.
    const SDL_FRect sky{0.0f, 0.0f, 800.0f, 360.0f};
    const SDL_FRect skyBandA{0.0f, 155.0f, 800.0f, 85.0f};
    const SDL_FRect skyBandB{0.0f, 240.0f, 800.0f, 82.0f};
    const SDL_FRect distantHill{0.0f, 265.0f, 800.0f, 120.0f};
    const SDL_FRect middleHill{0.0f, 305.0f, 800.0f, 110.0f};
    const SDL_FRect field{0.0f, 350.0f, 800.0f, 314.0f};
    fillRect(renderer, sky, 61, 89, 76);
    fillRect(renderer, skyBandA, 76, 103, 82);
    fillRect(renderer, skyBandB, 88, 111, 79);
    fillRect(renderer, distantHill, 83, 116, 75);
    fillRect(renderer, middleHill, 60, 96, 64);
    fillRect(renderer, field, 43, 125, 66);

    // Warm sun and broken cloud shapes establish a quiet late-afternoon mood.
    const SDL_FRect sunHalo{604.0f, 62.0f, 88.0f, 88.0f};
    const SDL_FRect sunOuter{620.0f, 78.0f, 56.0f, 56.0f};
    const SDL_FRect sunInner{627.0f, 85.0f, 42.0f, 42.0f};
    fillRect(renderer, sunHalo, 107, 119, 81);
    fillRect(renderer, sunOuter, 213, 169, 100);
    fillRect(renderer, sunInner, 239, 211, 153);
    drawTitleCloud(renderer, 78.0f, 74.0f, 1.0f);
    drawTitleCloud(renderer, 488.0f, 185.0f, 0.72f);

    // Layered field rows, narrow furrows and small planted patches.
    for (int row = 0; row < 5; ++row) {
        const float y = 390.0f + static_cast<float>(row) * 49.0f;
        const SDL_FRect band{0.0f, y, 800.0f, 2.0f};
        const SDL_FRect shadowBand{0.0f, y + 3.0f, 800.0f, 1.0f};
        fillRect(renderer, band, 58, 144, 75);
        fillRect(renderer, shadowBand, 36, 108, 57);
        for (int column = 0; column < 10; ++column) {
            const float x = static_cast<float>(column) * 86.0f +
                            static_cast<float>((row % 2) * 30);
            const SDL_FRect furrow{x, y + 13.0f, 58.0f, 3.0f};
            const SDL_FRect furrowHighlight{x + 3.0f, y + 13.0f, 48.0f, 1.0f};
            fillRect(renderer, furrow, 34, 105, 57);
            fillRect(renderer, furrowHighlight, 64, 135, 73);
        }
    }

    // Background landmarks and a small fence ground the scene in the same world.
    drawTree(renderer, 1, 7);
    drawTree(renderer, 3, 8);
    drawTree(renderer, 18, 7);
    drawFarmhouse(renderer, 13, 6);
    drawTitleFence(renderer, 45.0f, 458.0f, 5);
    drawTitleFlowerPatch(renderer, 665.0f, 495.0f);
    drawTitleFlowerPatch(renderer, 722.0f, 557.0f);

    // A subtle vignette-like frame gives the scene a finished edge.
    const SDL_FRect topFrame{0.0f, 0.0f, 800.0f, 5.0f};
    const SDL_FRect leftFrame{0.0f, 0.0f, 5.0f, 664.0f};
    const SDL_FRect rightFrame{795.0f, 0.0f, 5.0f, 664.0f};
    const SDL_FRect bottomFrame{0.0f, 659.0f, 800.0f, 5.0f};
    fillRect(renderer, topFrame, 34, 48, 39);
    fillRect(renderer, leftFrame, 34, 48, 39);
    fillRect(renderer, rightFrame, 34, 48, 39);
    fillRect(renderer, bottomFrame, 34, 48, 39);

    // Main title plaque.
    const SDL_FRect titlePanel{174.0f, 73.0f, 452.0f, 139.0f};
    drawWoodPanel(renderer, titlePanel);
    const SDL_FRect titleInner{190.0f, 89.0f, 420.0f, 107.0f};
    fillRect(renderer, titleInner, 77, 47, 32);
    const SDL_FRect titleInnerLine{204.0f, 101.0f, 392.0f, 1.0f};
    const SDL_FRect titleBottomLine{204.0f, 184.0f, 392.0f, 1.0f};
    fillRect(renderer, titleInnerLine, 128, 82, 51);
    fillRect(renderer, titleBottomLine, 128, 82, 51);

    // Small sprout mark, paired with the title and subtitle.
    const SDL_FRect sproutStem{270.0f, 122.0f, 4.0f, 31.0f};
    const SDL_FRect sproutLeafLeft{249.0f, 112.0f, 22.0f, 12.0f};
    const SDL_FRect sproutLeafRight{274.0f, 119.0f, 24.0f, 13.0f};
    const SDL_FRect sproutVeinLeft{257.0f, 117.0f, 11.0f, 2.0f};
    const SDL_FRect sproutVeinRight{277.0f, 124.0f, 12.0f, 2.0f};
    fillRect(renderer, sproutStem, 211, 180, 119);
    fillRect(renderer, sproutLeafLeft, 133, 173, 104);
    fillRect(renderer, sproutLeafRight, 94, 145, 81);
    fillRect(renderer, sproutVeinLeft, 172, 204, 129);
    fillRect(renderer, sproutVeinRight, 132, 177, 106);

    SDL_SetRenderDrawColor(renderer, 250, 234, 204, 255);
    SDL_RenderDebugText(renderer, 319.0f, 117.0f, "F A R M S T E A D");
    SDL_SetRenderDrawColor(renderer, 220, 194, 155, 255);
    SDL_RenderDebugText(renderer, 293.0f, 151.0f,
                        "A SMALL LIFE ON THE LAND");

    // Menu panel with clearly separated heading, actions and keyboard hint.
    const SDL_FRect menuPanel{270.0f, 333.0f, 260.0f, 166.0f};
    drawWoodPanel(renderer, menuPanel);
    SDL_SetRenderDrawColor(renderer, 249, 231, 198, 255);
    SDL_RenderDebugText(renderer, 347.0f, 354.0f, "BEGIN YOUR STORY");
    const SDL_FRect divider{292.0f, 371.0f, 216.0f, 1.0f};
    fillRect(renderer, divider, 158, 112, 75);

    const SDL_FRect newFarmOption{300.0f, 382.0f, 200.0f, 34.0f};
    const SDL_FRect quitOption{300.0f, 427.0f, 200.0f, 34.0f};
    drawMenuOption(renderer, newFarmOption, "NEW FARM", selectedOption == 0);
    drawMenuOption(renderer, quitOption, "QUIT GAME", selectedOption == 1);

    SDL_SetRenderDrawColor(renderer, 244, 225, 191, 255);
    SDL_RenderDebugText(renderer, 286.0f, 535.0f,
                        "ARROWS SELECT     ENTER CONFIRM");
    SDL_SetRenderDrawColor(renderer, 190, 169, 136, 255);
    SDL_RenderDebugText(renderer, 326.0f, 618.0f,
                        "FARMSTEAD  -  A LITTLE PLACE TO CALL HOME");

    SDL_RenderPresent(renderer);
}

void renderPauseMenu(SDL_Renderer* renderer, const GameModel& model,
                     int selectedOption, bool confirming,
                     std::string_view confirmationMessage,
                     int confirmationSelection,
                     std::string_view hudMessage,
                     HudMessageTone messageTone) {
    drawWorldFrame(renderer, model, hudMessage, messageTone);
    drawDimOverlay(renderer);

    if (confirming) {
        const SDL_FRect panel{188.0f, 201.0f, 424.0f, 252.0f};
        drawWoodPanel(renderer, panel);
        SDL_SetRenderDrawColor(renderer, 248, 231, 197, 255);
        SDL_RenderDebugText(renderer, 309.0f, 230.0f, "ARE YOU SURE?");

        std::string message(confirmationMessage);
        SDL_SetRenderDrawColor(renderer, 250, 225, 188, 255);
        SDL_RenderDebugText(renderer, 220.0f, 274.0f, message.c_str());
        SDL_SetRenderDrawColor(renderer, 220, 190, 151, 255);
        SDL_RenderDebugText(renderer, 270.0f, 307.0f,
                            "UNSAVED PROGRESS WILL BE LOST");

        const SDL_FRect yesOption{266.0f, 355.0f, 112.0f, 36.0f};
        const SDL_FRect noOption{422.0f, 355.0f, 112.0f, 36.0f};
        drawMenuOption(renderer, yesOption, "YES", confirmationSelection == 0);
        drawMenuOption(renderer, noOption, "NO", confirmationSelection == 1);

        SDL_SetRenderDrawColor(renderer, 220, 190, 151, 255);
        SDL_RenderDebugText(renderer, 278.0f, 420.0f,
                            "LEFT/RIGHT SELECT     ENTER CONFIRM");
    } else {
        const SDL_FRect panel{235.0f, 153.0f, 330.0f, 365.0f};
        drawWoodPanel(renderer, panel);
        SDL_SetRenderDrawColor(renderer, 248, 231, 197, 255);
        SDL_RenderDebugText(renderer, 365.0f, 181.0f, "PAUSED");
        SDL_SetRenderDrawColor(renderer, 214, 187, 148, 255);
        SDL_RenderDebugText(renderer, 295.0f, 207.0f,
                            "TAKE A BREATHER, FARMER");

        const SDL_FRect resumeOption{275.0f, 246.0f, 250.0f, 36.0f};
        const SDL_FRect restartOption{275.0f, 291.0f, 250.0f, 36.0f};
        const SDL_FRect titleOption{275.0f, 336.0f, 250.0f, 36.0f};
        const SDL_FRect quitOption{275.0f, 381.0f, 250.0f, 36.0f};
        drawMenuOption(renderer, resumeOption, "RESUME", selectedOption == 0);
        drawMenuOption(renderer, restartOption, "RESTART FARM", selectedOption == 1);
        drawMenuOption(renderer, titleOption, "RETURN TO TITLE", selectedOption == 2);
        drawMenuOption(renderer, quitOption, "QUIT GAME", selectedOption == 3);

        SDL_SetRenderDrawColor(renderer, 220, 190, 151, 255);
        SDL_RenderDebugText(renderer, 298.0f, 455.0f,
                            "UP/DOWN SELECT     ENTER CONFIRM");
        SDL_RenderDebugText(renderer, 349.0f, 473.0f, "ESC RESUME");
    }

    SDL_RenderPresent(renderer);
}

void renderVictoryOverlay(SDL_Renderer* renderer, const GameModel& model,
                          std::string_view message) {
    drawWorldFrame(renderer, model, message, HudMessageTone::SUCCESS);
    drawDimOverlay(renderer);

    const SDL_FRect panel{174.0f, 192.0f, 452.0f, 270.0f};
    drawWoodPanel(renderer, panel);
    const SDL_FRect inner{190.0f, 208.0f, 420.0f, 238.0f};
    fillRect(renderer, inner, 91, 61, 39, 255);

    // A small harvest emblem above the completion heading.
    const SDL_FRect stem{396.0f, 231.0f, 4.0f, 21.0f};
    const SDL_FRect leafLeft{380.0f, 232.0f, 17.0f, 9.0f};
    const SDL_FRect leafRight{400.0f, 237.0f, 17.0f, 9.0f};
    fillRect(renderer, stem, 209, 177, 115, 255);
    fillRect(renderer, leafLeft, 145, 182, 111, 255);
    fillRect(renderer, leafRight, 101, 151, 83, 255);

    SDL_SetRenderDrawColor(renderer, 250, 234, 204, 255);
    SDL_RenderDebugText(renderer, 323.0f, 271.0f, "OBJECTIVE COMPLETE");
    SDL_SetRenderDrawColor(renderer, 243, 213, 155, 255);
    SDL_RenderDebugTextFormat(renderer, 301.0f, 309.0f,
                              "YOU EARNED %d COINS", model.coinsEarned());
    SDL_SetRenderDrawColor(renderer, 222, 198, 165, 255);
    SDL_RenderDebugText(renderer, 282.0f, 351.0f,
                        "YOUR LITTLE FARM HAS PAID OFF");

    const SDL_FRect continueOption{276.0f, 390.0f, 248.0f, 34.0f};
    drawMenuOption(renderer, continueOption, "CONTINUE FARMING", true);
    SDL_SetRenderDrawColor(renderer, 218, 190, 151, 255);
    SDL_RenderDebugText(renderer, 316.0f, 438.0f,
                        "PRESS ENTER TO CONTINUE");

    SDL_RenderPresent(renderer);
}

} // namespace farmstead::render
