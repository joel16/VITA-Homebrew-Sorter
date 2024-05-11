#pragma once

#include <SDL.h>
#include <vector>

typedef struct {
    SDL_Texture *ptr = nullptr;
    int width = 0;
    int height = 0;
} Tex;

extern std::vector<Tex> icons;

namespace Textures {
    bool Init(void);
    void Exit(void);
}
