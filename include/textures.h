#pragma once

#include <vitaGL.h>
#include <vector>

typedef struct {
    GLuint id = 0;
    int width = 0;
    int height = 0;
} Tex;

extern std::vector<Tex> icons;

namespace Textures {
    bool Init(void);
    void Exit(void);
}
