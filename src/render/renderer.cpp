#include "../../include/farmstead/renderer.hpp"

#include <cmath>

namespace farmstead::render {
namespace {

constexpr int kWindowWidth = 800;
constexpr float kTileSize = 40.0f;
constexpr float kMapOffsetX = 0.0f;
constexpr float kMapOffsetY = 48.0f;
constexpr float kHudHeight = 48.0f;

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
    // Background behind the world and HUD.
    SDL_SetRenderDrawColor(renderer, 42, 61, 76, 255);
    SDL_RenderClear(renderer);

    // HUD panel.
    const SDL_FRect hudRect{
        0.0f, 0.0f, static_cast<float>(kWindowWidth), kHudHeight
    };
    fillRect(renderer, hudRect, 25, 38, 48);

    // Temporary prototype text; replace with a proper font system during polish.
    SDL_SetRenderDrawColor(renderer, 240, 240, 225, 255);
    SDL_RenderDebugTextFormat(renderer, 12.0f, 7.0f,
                              "DAY: %d    PRODUCE: %d",
                              currentDay, model.produceCount());
    SDL_RenderDebugText(renderer, 12.0f, 25.0f,
                        "ARROWS: MOVE  T: TILL  P: PLANT  W: WATER  N: NEXT DAY  H: HARVEST  E: INSPECT");

    // Draw the map backgrounds first, then crops on top of their soil.
    for (int row = 0; row < model.mapHeight(); ++row) {
        for (int column = 0; column < model.mapWidth(); ++column) {
            const TileType type = model.tileAt(column, row);

            if (type == TileType::OBSTACLE) {
                fillRect(renderer, makeTileRect(column, row), 10, 30, 20);
            } else if (model.isTilled(column, row)) {
                fillRect(renderer, makeTileRect(column, row), 130, 85, 50);
            } else {
                fillRect(renderer, makeTileRect(column, row), 30, 150, 80);
            }

            const CropStage stage = model.cropStageAt(column, row);
            if (stage != CropStage::NONE) {
                drawCrop(renderer, column, row, stage,
                         model.isWateredToday(column, row));
            }
        }
    }

    // Highlight the tile the player is currently facing.
    const TilePosition target = model.tileInFront();
    if (model.isInBounds(target.column, target.row)) {
        SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255);
        const SDL_FRect targetRect = makeTileRect(target.column, target.row);
        SDL_RenderRect(renderer, &targetRect);
    }

    // Draw the player last so the character appears above the world and highlight.
    drawPlayer(renderer, model.playerPosition(), model.facingDirection());

    SDL_RenderPresent(renderer);
}

} // namespace farmstead::render
