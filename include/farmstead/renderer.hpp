#pragma once

#include <SDL3/SDL.h>

#include "game_model.hpp"

namespace farmstead::render {

// Draws the HUD, world, crops, target tile and player, then presents the frame.
void renderWorld(SDL_Renderer* renderer, const GameModel& model, int currentDay);

} // namespace farmstead::render
