#include <memory>
#include <png.h>
#include <psp2/kernel/clib.h>
#include <string>

#include "imgui.h"
#include "log.h"
#include "textures.h"

std::vector<Tex> icons;

namespace Textures {
    constexpr int max_textures = 5;

    static bool Create(unsigned char *data, GLint format, Tex &texture) {    
        // Create a OpenGL texture identifier
        glGenTextures(1, &texture.id);
        glBindTexture(GL_TEXTURE_2D, texture.id);
        
        // Setup filtering parameters for display
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        // Upload pixels into texture
        glTexImage2D(GL_TEXTURE_2D, 0, format, texture.width, texture.height, 0, format, GL_UNSIGNED_BYTE, data);
        return true;
    }

    static bool LoadImagePNG(const std::string &path, Tex &texture) {
        bool ret = false;
        png_image image;
        sceClibMemset(&image, 0, (sizeof image));
        image.version = PNG_IMAGE_VERSION;
        
        if (png_image_begin_read_from_file(&image, path.c_str()) != 0) {
            image.format = PNG_FORMAT_RGBA;
            std::unique_ptr<png_byte[]> buffer(new png_byte[PNG_IMAGE_SIZE(image)]);
            
            if (png_image_finish_read(&image, nullptr, buffer.get(), 0, nullptr) != 0) {
                texture.width = image.width;
                texture.height = image.height;
                ret = Textures::Create(buffer.get(), GL_RGBA, texture);
                png_image_free(&image);
            }
            else {
                Log::Error("png_image_finish_read failed: %s\n", image.message);
                png_image_free(&image);
            }
        }
        else
            Log::Error("png_image_begin_read_from_memory failed: %s\n", image.message);
        
        return ret;
    }

    bool Init(void) {
        icons.resize(max_textures);

        const std::string paths[max_textures] {
            "app0:res/app.png",
            "app0:res/db.png",
            "app0:res/folder.png",
            "app0:res/page.png",
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
