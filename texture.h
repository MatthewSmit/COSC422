#pragma once

#include <GL/glew.h>
#include <IL/il.h>

class Texture {
public:
    explicit Texture(const std::string& filePath) {
        glCreateSamplers(1, &sampler);
        glCreateTextures(GL_TEXTURE_2D, 1, &texture);

        ILuint id = ilGenImage();
        ilBindImage(id);
        ilEnable(IL_ORIGIN_SET);
        ilOriginFunc(IL_ORIGIN_LOWER_LEFT);

        if (ilLoadImage(filePath.c_str())) {
            ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);

            glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            auto width = ilGetInteger(IL_IMAGE_WIDTH);
            auto height = ilGetInteger(IL_IMAGE_HEIGHT);

            glTextureStorage2D(texture, 1, GL_RGB8, width, height);
            glTextureSubImage2D(texture, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, ilGetData());
        } else {
            std::cerr << "Unable to load texture: " + filePath << std::endl;
            throw std::exception{};
        }

        ilDeleteImage(id);
    }

    ~Texture() {
        glDeleteTextures(1, &texture);
        glDeleteSamplers(1, &sampler);
    }

    void bind(int slot) {
        if (slot >= 0) {
            glBindSampler(slot, sampler);
            glBindTextureUnit(slot, texture);
        }
    }

private:
    GLuint sampler{};
    GLuint texture{};
};