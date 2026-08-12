// #pragma once

// #include <SDL.h>
// #include <SDL_ttf.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string>

// class Screen_Text
// {

// public:
//     Screen_Text() = default;
//     SDL_Renderer *renderer;
//     SDL_Texture *textTexture;
//     SDL_Surface *textSurface ;
//     TTF_Font *fontL;


//     Screen_Text(std::string text, SDL_Window *_window, const std::string &fontPath, const int & widht , const int & height)
//     {
//         if (TTF_Init() != 0)
//         {
//             printf("Failed to initialize SDL_ttf: %s\n", TTF_GetError());
//             return;
//         }

//         renderer = SDL_CreateRenderer(_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
//         if (!renderer)
//         {
//             printf("Failed to create renderer: %s\n", SDL_GetError());
//             TTF_Quit();
//             return;
//         }
//         SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
//         SDL_RenderClear(renderer);

//         fontL = TTF_OpenFont(fontPath.c_str(), 64);
//         if (!fontL)
//         {
//             printf("Failed to load font: %s\n", TTF_GetError());
//             SDL_DestroyRenderer(renderer);
//             renderer = nullptr;
//             TTF_Quit();
//             return;
//         }
//         SDL_Color textColor = {0, 0, 0, 255}; 
//         textSurface = TTF_RenderText_Solid(fontL, text.c_str(), textColor);

//         if (!textSurface)
//         {
//             printf("Failed to create text surface: %s\n", TTF_GetError());
//             TTF_CloseFont(fontL);
//             return;
//         }

//         textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
//         if (!textTexture)
//         {
//             printf("Failed to create text texture: %s\n", SDL_GetError());
//             SDL_FreeSurface(textSurface);
//             TTF_CloseFont(fontL);
//             return;
//         }

//         SDL_Rect textRect = {widht, height, textSurface->w, textSurface->h};
//         SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
        
        
//     }

//     void Draw(){
//         SDL_RenderPresent(this->renderer);
//     }

//     ~Screen_Text(){
//         if (textTexture) SDL_DestroyTexture(textTexture);
//         if (textSurface) SDL_FreeSurface(textSurface);
//         if (fontL) TTF_CloseFont(fontL);
//         if (renderer) SDL_DestroyRenderer(renderer);
//         TTF_Quit();
//     }
// };