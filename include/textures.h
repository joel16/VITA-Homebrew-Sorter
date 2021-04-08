#ifndef _VITA_HB_SORTER_TEXTURES_H_
#define _VITA_HB_SORTER_TEXTURES_H_

#include <vitaGL.h>

typedef struct {
    GLuint id = 0;
    int width = 0;
    int height = 0;
} Tex;

namespace Textures {
    void Init(void);
    void Exit(void);
}

#endif
