#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H

#include <string>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <condition_variable>
#include "EventBus/EventBus.h"
#include "EventBus/EventTypes.h"
#include "Texture/Texture.h"
#include "ThreadPool/ThreadPool.h"

class TextureManager {
public:
    // 构造函数，注入 EventBus 和 ThreadPool
    TextureManager(std::shared_ptr<EventBus> eventBus, std::shared_ptr<ThreadPool> threadPool);
    ~TextureManager();

    // 异步加载纹理，返回 UUID，实际加载和上传通过线程池和队列完成
    std::string LoadTexture(const std::string& filepath);
    std::shared_ptr<Texture> GetTexture(const std::string& textureUUID) const;
    std::shared_ptr<Texture> GetTextureByFilepath(const std::string& filepath) const;
    void DeleteTexture(const std::string& textureUUID);
    void AddRef(const std::string& textureUUID);
    void Release(const std::string& textureUUID);

    // 主线程调用，处理纹理上传队列，将数据提交到 GPU
    void ProcessTextureUploadQueue();

private:
    struct TextureEntry {
        std::shared_ptr<Texture> texture;
        int refCount = 0;
    };

    // 纹理上传任务结构体
    struct TextureUploadTask {
        std::string uuid;
        std::string filepath;
        unsigned char* data = nullptr;
        int width = 0;
        int height = 0;
        int channels = 0;
    };

    std::string GenerateUUID() const;
    void SubscribeToEvents();
    void OnRequestTextureLoad(const MyRenderer::Events::RequestTextureLoadEvent& event);

    std::shared_ptr<EventBus> eventBus_;         // 事件总线，用于通知加载状态
    std::shared_ptr<ThreadPool> threadPool_;     // 线程池，用于异步加载文件
    std::map<std::string, TextureEntry> textures_;  // 已加载的纹理
    std::map<std::string, std::string> filepathToUUID_;  // 文件路径到 UUID 的映射
    mutable std::mutex mutex_;  // 保护 textures_ 和 filepathToUUID_

    std::queue<TextureUploadTask> uploadQueue_;  // 纹理上传队列
    std::mutex queueMutex_;  // 保护上传队列
    std::condition_variable queueCV_;  // 队列条件变量
};

#endif // TEXTURE_MANAGER_H
