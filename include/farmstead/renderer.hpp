#pragma once

#include <SDL3/SDL.h>

#include <string_view>

#include "farmstead/game_model.hpp"

namespace farmstead::render {

enum class HudMessageTone {
    INFO,
    SUCCESS,
    WARNING
};

// Draws the HUD, world, crops, market, farmhouse, the player's tile indicator
// and player, then presents the frame. Day state is read from GameModel; feedback is supplied
// by the SDL application while game rules remain inside GameModel.
void renderWorld(SDL_Renderer* renderer, const GameModel& model,
                 std::string_view message = {},
                 HudMessageTone messageTone = HudMessageTone::INFO);

} // namespace farmstead::render
