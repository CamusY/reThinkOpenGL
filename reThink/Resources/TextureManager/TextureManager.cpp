#include "TextureManager.h"
#include <stdexcept>
#include <random>
#include <chrono>
#include <sstream>
#include <filesystem>
#include <stb_image.h>
namespace fs = std::filesystem;

TextureManager::TextureManager(std::shared_ptr<EventBus> eventBus, std::shared_ptr<ThreadPool> threadPool)
    : eventBus_(std::move(eventBus)), threadPool_(std::move(threadPool)) {
    if (!eventBus_) throw std::invalid_argument("TextureManager: EventBus cannot be null");
    if (!threadPool_) throw std::invalid_argument("TextureManager: ThreadPool cannot be null");
    SubscribeToEvents();
}

TextureManager::~TextureManager() {
    std::lock_guard<std::mutex> lock(mutex_);
    textures_.clear();
    filepathToUUID_.clear();
}

std::string TextureManager::LoadTexture(const std::string& filepath) {
    if (filepath.empty()) throw std::invalid_argument("TextureManager: Filepath cannot be empty");

    // 检查是否已加载
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = filepathToUUID_.find(filepath);
        if (it != filepathToUUID_.end()) {
            textures_[it->second].refCount++;
            return it->second;
        }
    }

    if (!fs::exists(filepath)) {
        eventBus_->Publish(MyRenderer::Events::TextureLoadedEvent{
            "", filepath, false, "File not found"
        });
        return "";
    }

    std::string uuid = GenerateUUID();
    {
        std::lock_guard<std::mutex> lock(mutex_);
        filepathToUUID_[filepath] = uuid;
        auto texture = std::make_shared<Texture>(eventBus_, uuid, filepath);
        textures_[uuid] = {texture, 1};  // 初始引用计数为 1
    }

    // 异步加载纹理文件数据
    threadPool_->EnqueueTask([this, uuid, filepath]() {
        stbi_set_flip_vertically_on_load(true);
        int width, height, channels;
        unsigned char* data = stbi_load(filepath.c_str(), &width, &height, &channels, 0);

        if (!data) {
            eventBus_->Publish(MyRenderer::Events::TextureLoadedEvent{
                uuid, filepath, false, "Failed to load texture"
            });
            return;
        }

        // 将加载完成的数据加入上传队列
        {
            std::lock_guard<std::mutex> queueLock(queueMutex_);
            uploadQueue_.push({uuid, filepath, data, width, height, channels});
        }
        queueCV_.notify_one();
    });

    return uuid;
}

void TextureManager::ProcessTextureUploadQueue() {
    std::unique_lock<std::mutex> lock(queueMutex_);
    while (!uploadQueue_.empty()) {
        TextureUploadTask task = uploadQueue_.front();
        uploadQueue_.pop();
        lock.unlock();

        // 在主线程中上传纹理到 GPU
        auto texture = GetTexture(task.uuid);
        if (texture) {
            texture->UploadToGPU(task.data, task.width, task.height, task.channels);
            eventBus_->Publish(MyRenderer::Events::TextureLoadedEvent{
                task.uuid, task.filepath, true, ""
            });
        }
        stbi_image_free(task.data);

        lock.lock();
    }
}

std::shared_ptr<Texture> TextureManager::GetTexture(const std::string& textureUUID) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = textures_.find(textureUUID);
    return (it != textures_.end()) ? it->second.texture : nullptr;
}

std::shared_ptr<Texture> TextureManager::GetTextureByFilepath(const std::string& filepath) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = filepathToUUID_.find(filepath);
    if (it != filepathToUUID_.end()) {
        auto texIt = textures_.find(it->second);
        return (texIt != textures_.end()) ? texIt->second.texture : nullptr;
    }
    return nullptr;
}

void TextureManager::DeleteTexture(const std::string& textureUUID) {
    if (textureUUID.empty()) throw std::invalid_argument("TextureManager: Texture UUID cannot be empty");

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = textures_.find(textureUUID);
    if (it != textures_.end()) {
        if (--it->second.refCount <= 0) {
            for (auto filepathIt = filepathToUUID_.begin(); filepathIt != filepathToUUID_.end(); ) {
                if (filepathIt->second == textureUUID) {
                    filepathIt = filepathToUUID_.erase(filepathIt);
                    break;
                } else {
                    ++filepathIt;
                }
            }
            eventBus_->Publish(MyRenderer::Events::TextureDeletedEvent{textureUUID});
            textures_.erase(it);
        }
    }
}

void TextureManager::AddRef(const std::string& textureUUID) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (auto it = textures_.find(textureUUID); it != textures_.end()) {
        it->second.refCount++;
    }
}

void TextureManager::Release(const std::string& textureUUID) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (auto it = textures_.find(textureUUID); it != textures_.end()) {
        if (--it->second.refCount <= 0) {
            for (auto filepathIt = filepathToUUID_.begin(); filepathIt != filepathToUUID_.end(); ) {
                if (filepathIt->second == textureUUID) {
                    filepathIt = filepathToUUID_.erase(filepathIt);
                    break;
                } else {
                    ++filepathIt;
                }
            }
            eventBus_->Publish(MyRenderer::Events::TextureDeletedEvent{textureUUID});
            textures_.erase(it);
        }
    }
}

std::string TextureManager::GenerateUUID() const {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    static std::uniform_int_distribution<> dis2(8, 11);

    auto now = std::chrono::system_clock::now().time_since_epoch().count();
    std::stringstream ss;
    ss << std::hex << now << dis(gen) << dis2(gen);
    return ss.str();
}

void TextureManager::SubscribeToEvents() {
    eventBus_->Subscribe<MyRenderer::Events::RequestTextureLoadEvent>(
        [this](const MyRenderer::Events::RequestTextureLoadEvent& event) {
            OnRequestTextureLoad(event);
        });
}

void TextureManager::OnRequestTextureLoad(const MyRenderer::Events::RequestTextureLoadEvent& event) {
    LoadTexture(event.filepath);
}
