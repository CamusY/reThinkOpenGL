#pragma once
#include <unordered_map>
#include <memory>
#include <string>
#include <mutex>
#include <filesystem>
#include <functional>
#include "Texture.h"
#include "ThreadPool/ThreadPool.h"

namespace fs = std::filesystem;

/**
 * @brief 纹理资源管理器，负责纹理加载、缓存和生命周期管理
 * @dependency 可选依赖ThreadPool实现异步加载
 */
class TextureManager {
public:
    /**
     * @brief 构造函数（同步加载版本）
     */
    TextureManager() = default;

    /**
     * @brief 构造函数（异步加载版本）
     * @param threadPool 线程池实例（必须非空）
     */
    explicit TextureManager(std::shared_ptr<ThreadPool> threadPool)
        : threadPool_(threadPool) {}

    // 禁止拷贝和移动
    TextureManager(const TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;

    /**
     * @brief 同步加载纹理（立即返回纹理对象）
     * @param texturePath 纹理文件路径
     * @return 纹理对象的共享指针，失败时返回nullptr
     * @throws std::invalid_argument 路径无效时抛出
     */
    std::shared_ptr<Texture> LoadTexture(const fs::path& texturePath);

    /**
     * @brief 异步加载纹理（通过回调返回结果）
     * @param texturePath 纹理文件路径
     * @param callback 加载完成回调函数
     */
    void LoadTextureAsync(const fs::path& texturePath, 
                        std::function<void(std::shared_ptr<Texture>)> callback);

    /**
     * @brief 获取已加载的纹理
     * @param texturePath 纹理文件路径
     * @return 纹理对象的共享指针，未加载时返回nullptr
     */
    std::shared_ptr<Texture> GetTexture(const fs::path& texturePath) const;

    /**
     * @brief 检查并重新加载修改过的纹理
     * @param texturePath 要检查的纹理路径
     * @return 是否成功重新加载
     */
    bool ReloadIfModified(const fs::path& texturePath);

    /**
     * @brief 设置纹理参数模板
     * @param wrapS S轴环绕方式
     * @param wrapT T轴环绕方式
     * @param minFilter 缩小过滤
     * @param magFilter 放大过滤
     */
    void SetDefaultTextureParams(GLenum wrapS = GL_REPEAT,
                               GLenum wrapT = GL_REPEAT,
                               GLenum minFilter = GL_LINEAR_MIPMAP_LINEAR,
                               GLenum magFilter = GL_LINEAR);

private:
    mutable std::mutex cacheMutex_;
    std::unordered_map<std::string, std::shared_ptr<Texture>> textureCache_;
    std::shared_ptr<ThreadPool> threadPool_; // 可选依赖

    // 默认纹理参数
    GLenum defaultWrapS_ = GL_REPEAT;
    GLenum defaultWrapT_ = GL_REPEAT;
    GLenum defaultMinFilter_ = GL_LINEAR_MIPMAP_LINEAR;
    GLenum defaultMagFilter_ = GL_LINEAR;

    /**
     * @brief 实际执行纹理加载的内部方法
     * @param texturePath 纹理文件路径
     * @return 加载成功的纹理对象
     */
    std::shared_ptr<Texture> LoadTextureInternal(const fs::path& texturePath);
};