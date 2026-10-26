#include <SDL3/SDL.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>

#include <iostream>

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

    bool running = true;


    // Main application loop. Continues until a quit event tells the application to stop.
    while(running){

        SDL_Event event;

        // Process all pending events before rendering the next frame.
        while(SDL_PollEvent(&event)){
            if(event.type == SDL_EVENT_QUIT){
                running = false;
            }
        } 

        // Clear the previous frame and present the current frame using the selected background color.
        SDL_SetRenderDrawColor(renderPointer, 50, 100, 160, 255);
        SDL_RenderClear(renderPointer);
        SDL_RenderPresent(renderPointer);

    }

    // Release resources in reverse order of dependency.
    SDL_DestroyRenderer(renderPointer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}