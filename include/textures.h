#ifndef _VITA_HB_SORTER_TEXTURES_H_
#define _VITA_HB_SORTER_TEXTURES_H_

#include <vitaGL.h>
#include <vector>

typedef struct {
    GLuint id = 0;
    int width = 0;
    int height = 0;
} Tex;

extern Tex splash;
extern std::vector<Tex> icons;

namespace Textures {
    bool Init(void);
    void Exit(void);
}

#endif
