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

// Draw the active farm world and its HUD.
void renderWorld(SDL_Renderer* renderer, const GameModel& model,
                 std::string_view message = {},
                 HudMessageTone messageTone = HudMessageTone::INFO);

// Draw the main menu. selectedOption: 0 = New Farm, 1 = Quit.
void renderTitleScreen(SDL_Renderer* renderer, int selectedOption);

// Draw the farm with a pause overlay. When confirming is true, show a
// destructive-action confirmation instead of the pause options.
void renderPauseMenu(SDL_Renderer* renderer, const GameModel& model,
                     int selectedOption, bool confirming,
                     std::string_view confirmationMessage,
                     int confirmationSelection,
                     std::string_view hudMessage = {},
                     HudMessageTone messageTone = HudMessageTone::INFO);

// Draw the farm with the objective-completion overlay.
void renderVictoryOverlay(SDL_Renderer* renderer, const GameModel& model,
                          std::string_view message = {});

} // namespace farmstead::render
