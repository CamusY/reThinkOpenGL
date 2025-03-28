#ifndef TEXTURE_H
#define TEXTURE_H

#include <string>
#include <memory>
#include <filesystem>
#include "EventBus/EventBus.h"
#include "EventBus/EventTypes.h" 

using namespace MyRenderer::Events;
namespace fs = std::filesystem;

class Texture {
public:
    // 构造函数，仅初始化基本信息，不加载数据
    Texture(std::shared_ptr<EventBus> eventBus, const std::string& uuid, const std::string& filepath);
    ~Texture() { Unload(); }

    std::string GetUUID() const { return uuid_; }
    std::string GetFilepath() const { return filepath_; }
    unsigned int GetTextureID() const { return textureID_; }
    int GetWidth() const { return width_; }
    int GetHeight() const { return height_; }

    // 绑定纹理到指定单元
    void Bind(unsigned int unit) const;
    // 释放纹理资源
    void Unload();
    // 在主线程中上传纹理数据到 GPU
    void UploadToGPU(unsigned char* data, int width, int height, int channels);

private:
    std::shared_ptr<EventBus> eventBus_;  // 事件总线
    std::string uuid_;          // 纹理 UUID
    std::string filepath_;      // 纹理文件路径
    unsigned int textureID_ = 0; // OpenGL 纹理 ID
    int width_ = 0;             // 纹理宽度
    int height_ = 0;            // 纹理高度
    int channels_ = 0;          // 通道数
};

#endif // TEXTURE_H
