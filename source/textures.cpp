#include <SDL_image.h>
#include <string>

#include "imgui.h"
#include "gui.h"
#include "log.h"
#include "textures.h"

std::vector<Tex> icons;

namespace Textures {
    constexpr int max_textures = 7;
    int GL_RGB = 3, GL_RGBA = 4;

    static bool LoadImage(const std::string &path, Tex &texture) {
        texture.ptr = IMG_LoadTexture(GUI::GetRenderer(), path.c_str());

        if (!texture.ptr) {
            Log::Error("Couldn't load %s: %s\n", path.c_str(), SDL_GetError());
            return false;
        }

        SDL_QueryTexture(texture.ptr, nullptr, nullptr, &texture.width, &texture.height);
        return true;
    }

    void Free(SDL_Texture *texture) {
        if (texture) {
            SDL_DestroyTexture(texture);
            texture = nullptr;
        }
    }

    bool Init(void) {
        const std::string paths[max_textures] {
            "app0:res/app.png",
            "app0:res/db.png",
            "app0:res/folder.png",
            "app0:res/page.png",
            "app0:res/trash.png",
            "app0:res/checked.png",
            "app0:res/unchecked.png"
        };

        for (int i = 0; i < max_textures; i++) {
            Tex texture;
            bool ret = Textures::LoadImage(paths[i], texture);
            IM_ASSERT(ret);

            icons.push_back(texture);
        }

        return 0;
    }

    void Exit(void) {
        for (int i = 0; i < max_textures; i++) {
            Textures::Free(icons[i].ptr);
        }
    }
}
