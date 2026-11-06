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

void renderWorld(SDL_Renderer* renderer, const GameModel& model,
                 std::string_view message, HudMessageTone messageTone) {
    SDL_SetRenderDrawColor(renderer, 31, 47, 43, 255);
    SDL_RenderClear(renderer);

    drawHud(renderer, model, message, messageTone);

    // First pass: terrain. Market tiles have a timber base; tree tiles retain
    // grass beneath their canopy and trunk artwork.
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
