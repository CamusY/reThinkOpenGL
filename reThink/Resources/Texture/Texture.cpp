#include "Texture.h"
#include <glad/glad.h>

Texture::Texture(std::shared_ptr<EventBus> eventBus, const std::string& uuid, const std::string& filepath)
    : eventBus_(eventBus), uuid_(uuid), filepath_(filepath) {
    if (!eventBus_) throw std::invalid_argument("Texture: EventBus cannot be null");
    if (uuid_.empty()) throw std::invalid_argument("Texture: UUID cannot be empty");
}

void Texture::UploadToGPU(unsigned char* data, int width, int height, int channels) {
    if (textureID_ != 0) return;  // 已上传则跳过

    width_ = width;
    height_ = height;
    channels_ = channels;

    // 生成并配置 OpenGL 纹理
    glGenTextures(1, &textureID_);
    glBindTexture(GL_TEXTURE_2D, textureID_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
}

void Texture::Bind(unsigned int unit) const {
    if (textureID_ == 0) return;  // 未上传则不绑定
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, textureID_);
}

void Texture::Unload() {
    if (textureID_ != 0) {
        glDeleteTextures(1, &textureID_);
        textureID_ = 0;
    }
    filepath_.clear();
    width_ = height_ = channels_ = 0;
}
