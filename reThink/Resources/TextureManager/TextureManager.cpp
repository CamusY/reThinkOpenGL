#include "TextureManager.h"
#include <./stb_image.h>
#include <glad/glad.h>
#include <fstream>
#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <unordered_set>

using namespace std;

static bool IsImageFileSupported(const fs::path& path) {
    static const unordered_set<string> supportedExtensions = {
        ".png", ".jpg", ".jpeg", ".tga", ".bmp", ".psd", ".gif", ".hdr", ".pic"
    };
    return supportedExtensions.count(path.extension().string()) > 0;
}

static GLenum GetGLFormat(int channels) {
    switch (channels) {
        case 1: return GL_RED;
        case 2: return GL_RG;
        case 3: return GL_RGB;
        case 4: return GL_RGBA;
        default: throw runtime_error("不支持的通道数: " + to_string(channels));
    }
}

shared_ptr<Texture> TextureManager::LoadTexture(const fs::path& texturePath) {
    // 防御性检查
    if (texturePath.empty()) {
        throw invalid_argument("纹理路径不能为空");
    }
    if (!fs::exists(texturePath)) {
        throw runtime_error("纹理文件不存在: " + texturePath.string());
    }
    if (!IsImageFileSupported(texturePath)) {
        throw runtime_error("不支持的图像格式: " + texturePath.extension().string());
    }

    // 检查缓存
    const string key = texturePath.string();
    {
        lock_guard<mutex> lock(cacheMutex_);
        auto it = textureCache_.find(key);
        if (it != textureCache_.end()) {
            return it->second;
        }
    }

    // 执行实际加载
    auto texture = LoadTextureInternal(texturePath);
    
    // 更新缓存
    lock_guard<mutex> lock(cacheMutex_);
    return textureCache_[key] = texture;
}

void TextureManager::LoadTextureAsync(const fs::path& texturePath,
                                    function<void(shared_ptr<Texture>)> callback) 
{
    // 检查是否支持异步
    if (!threadPool_) {
        throw runtime_error("线程池未初始化，无法执行异步加载");
    }

    // 提交异步任务
    threadPool_->Enqueue([=]() {
        try {
            auto texture = LoadTexture(texturePath);
            if (callback) callback(texture);
        } catch (...) {
            if (callback) callback(nullptr);
        }
    });
}

shared_ptr<Texture> TextureManager::GetTexture(const fs::path& texturePath) const {
    lock_guard<mutex> lock(cacheMutex_);
    auto it = textureCache_.find(texturePath.string());
    return it != textureCache_.end() ? it->second : nullptr;
}

bool TextureManager::ReloadIfModified(const fs::path& texturePath) {
    // 检查文件是否修改
    auto currentTexture = GetTexture(texturePath);
    if (!currentTexture) return false;

    try {
        const auto newWriteTime = fs::last_write_time(texturePath);
        if (newWriteTime <= currentTexture->lastWriteTime) return false;

        // 重新加载纹理
        auto newTexture = LoadTextureInternal(texturePath);
        
        lock_guard<mutex> lock(cacheMutex_);
        textureCache_[texturePath.string()] = newTexture;
        return true;
    } catch (...) {
        return false;
    }
}

shared_ptr<Texture> TextureManager::LoadTextureInternal(const fs::path& texturePath) {
    // 加载图像数据
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unique_ptr<unsigned char, decltype(&stbi_image_free)> data(
        stbi_load(texturePath.string().c_str(), &width, &height, &channels, 0),
        stbi_image_free
    );

    if (!data) {
        throw runtime_error("无法加载纹理: " + string(stbi_failure_reason()));
    }

    // 创建OpenGL纹理
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // 设置纹理参数
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, defaultWrapS_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, defaultWrapT_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, defaultMinFilter_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, defaultMagFilter_);

    // 上传数据并生成mipmap
    GLenum format = GetGLFormat(channels);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data.get());
    glGenerateMipmap(GL_TEXTURE_2D);

    // 创建纹理对象
    auto texture = make_shared<Texture>();
    texture->id = textureID;
    texture->width = width;
    texture->height = height;
    texture->channels = channels;
    texture->path = texturePath.string();
    texture->lastWriteTime = fs::last_write_time(texturePath);
    texture->wrapS = defaultWrapS_;
    texture->wrapT = defaultWrapT_;
    texture->minFilter = defaultMinFilter_;
    texture->magFilter = defaultMagFilter_;

    return texture;
}

void TextureManager::SetDefaultTextureParams(GLenum wrapS, GLenum wrapT,
                                           GLenum minFilter, GLenum magFilter) 
{
    defaultWrapS_ = wrapS;
    defaultWrapT_ = wrapT;
    defaultMinFilter_ = minFilter;
    defaultMagFilter_ = magFilter;
}