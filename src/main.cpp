#include <SDL3/SDL.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>

#include <iostream>
#include "../include/farmstead/game_model.hpp"

int main(){

    // Initialize SDL's video subsystem before creating a window or renderer.
    if(!SDL_Init(SDL_INIT_VIDEO)){
        std::cerr << "Unable to initialize video : " 
                << SDL_GetError() << '\n';
        return 1;
    }

    // Create the application window.
    SDL_Window* window = SDL_CreateWindow("Farmstead", 800, 600, 0);

    // ... window failure check ...
    if(window == nullptr){
        std::cerr << "Window failed to initialize : "
                << SDL_GetError() << '\n';
        
        SDL_Quit();
        return 1;
    }

    // Create a renderer associated with the window.
    SDL_Renderer* renderPointer = SDL_CreateRenderer(window, nullptr);

    // Create a renderer associated with the window.
    if(renderPointer == nullptr){
        std::cerr << "Unable to render : "
                << SDL_GetError() << '\n';

        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    if (!SDL_SetRenderVSync(renderPointer, 1)) {
        std::cerr << "VSync unable to be enabled : "
            << SDL_GetError() << '\n';
    }

    constexpr float tileSize = 40.0f;

    farmstead::GameModel model;
    bool running = true;


    // Main application loop. Continues until a quit event tells the application to stop.
    while(running){

        SDL_Event event;

        // Process all pending events before rendering the next frame.
        while(SDL_PollEvent(&event)){
            if(event.type == SDL_EVENT_QUIT){
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
                    default:
                        break;                
                }
            
            }
        } 

        // Clear the previous frame and present the current frame using the selected background color.
        SDL_SetRenderDrawColor(renderPointer, 50, 100, 160, 255);
        SDL_RenderClear(renderPointer);

        for (int row =0; row < model.mapHeight() ; row++) {
            for (int col = 0; col < model.mapWidth(); col++) {

               farmstead::TileType type = model.tileAt(col, row);

                if (type == farmstead::TileType::GROUND) {
                    SDL_SetRenderDrawColor(renderPointer, 30, 150, 80, 255);
                }
                else {
                    SDL_SetRenderDrawColor(renderPointer, 10, 30, 20, 255);                
                }

                SDL_FRect tileRect{col * tileSize, row * tileSize, tileSize, tileSize};
                SDL_RenderFillRect(renderPointer, &tileRect);

            }
        }

        farmstead::TilePosition playerPos = model.playerPosition();

        float playerSize = tileSize * 0.6f;
        float padding = (tileSize - playerSize ) / 2.0f;

        SDL_FRect playerRect{
            playerPos.column * tileSize + padding,
            playerPos.row * tileSize + padding,
            playerSize,
            playerSize
        };

        // Choose a colour for the player.
        SDL_SetRenderDrawColor(renderPointer, 220, 60, 60, 255);

        // Draw the player over the map.
        SDL_RenderFillRect(renderPointer, &playerRect);

        SDL_RenderPresent(renderPointer);

    }

    // Release resources in reverse order of dependency.
    SDL_DestroyRenderer(renderPointer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}