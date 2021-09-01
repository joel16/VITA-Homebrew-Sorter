#include <png.h>
#include <psp2/kernel/clib.h>
#include <string>

#include "imgui.h"
#include "log.h"
#include "textures.h"

std::vector<Tex> icons;

namespace Textures {
    constexpr int max_textures = 4;

    static bool LoadImage(unsigned char *data, GLint format, Tex &texture, void (*free_func)(void *)) {    
        // Create a OpenGL texture identifier
        glGenTextures(1, &texture.id);
        glBindTexture(GL_TEXTURE_2D, texture.id);
        
        // Setup filtering parameters for display
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        // Upload pixels into texture
        glTexImage2D(GL_TEXTURE_2D, 0, format, texture.width, texture.height, 0, format, GL_UNSIGNED_BYTE, data);

        if (*free_func)
            free_func(data);
        
        return true;
    }

    static bool LoadImagePNG(const std::string &path, Tex &texture) {
        bool ret = false;
        png_image image;
        sceClibMemset(&image, 0, (sizeof image));
        image.version = PNG_IMAGE_VERSION;

        if (png_image_begin_read_from_file(&image, path.c_str()) != 0) {
            png_bytep buffer;
            image.format = PNG_FORMAT_RGBA;
            buffer = new png_byte[PNG_IMAGE_SIZE(image)];
            
            if (buffer != nullptr && png_image_finish_read(&image, nullptr, buffer, 0, nullptr) != 0) {
                texture.width = image.width;
                texture.height = image.height;
                ret = Textures::LoadImage(buffer, GL_RGBA, texture, nullptr);
                
                if (buffer == nullptr)
                    png_image_free(&image);
                else
                    delete[] buffer;
            }
            else {
                if (buffer == nullptr) {
                    Log::Error("png_byte buffer: returned nullptr\n");
                    png_image_free(&image);
                }
                else {
                    Log::Error("png_image_finish_read failed: %s\n", image.message);
                    delete[] buffer;
                }
            }
        }
        else
            Log::Error("png_image_begin_read_from_file(%s) failed: %s\n", path.c_str(), image.message);
        
        return ret;
    }

    bool Init(void) {
        icons.resize(max_textures);

        const std::string paths[max_textures] {
            "app0:res/app.png",
            "app0:res/db.png",
            "app0:res/folder.png",
            "app0:res/trash.png"
        };

        for (int i = 0; i < max_textures; i++) {
            bool ret = Textures::LoadImagePNG(paths[i], icons[i]);
            IM_ASSERT(ret);
        }

        return 0;
    }

    void Exit(void) {
        for (int i = 0; i < max_textures; i++) {
            glDeleteTextures(1, &icons[i].id);
        }
    }
}
