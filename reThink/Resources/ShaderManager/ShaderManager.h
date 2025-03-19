// Resources/ShaderManager.h
#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <filesystem>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "EventBus/EventTypes.h"
#include "EventBus/EventBus.h"
#include "ThreadPool/ThreadPool.h"


namespace fs = std::filesystem;

/**
 * @brief 着色器程序元数据，包含编译状态和文件信息
 */
struct ShaderMetadata {
    GLuint programId = 0;                // OpenGL着色器程序ID
    fs::file_time_type lastVertexWrite;  // 顶点着色器最后修改时间
    fs::file_time_type lastFragmentWrite;// 片段着色器最后修改时间
    bool isCompiling = false;            // 是否正在编译中
    bool success = false;                // 编译是否成功
    std::string lastError;               // 最后一次编译错误信息
};

/**
 * @brief 着色器资源管理器，负责加载、编译、热重载和生命周期管理
 * @note 通过构造函数注入线程池和事件总线，严格遵循依赖注入原则
 */
class ShaderManager {
public:
    /**
     * @brief 构造函数注入依赖项
     * @param threadPool 线程池实例指针（必须非空）
     * @param eventBus 事件总线实例指针（必须非空）
     * @throws std::invalid_argument 参数为空时抛出
     */
    ShaderManager(ThreadPool* threadPool, EventBus* eventBus);

    // 禁止拷贝和移动
    ShaderManager(const ShaderManager&) = delete;
    ShaderManager& operator=(const ShaderManager&) = delete;
    ~ShaderManager() = default;

    /**
     * @brief 从文件加载并编译着色器程序（异步）
     * @param vertexShaderPath 顶点着色器文件路径
     * @param fragmentShaderPath 片段着色器文件路径
     * @return 是否成功提交编译任务
     */
    bool LoadShaderFromFile(const std::string& vertexShaderPath, 
                           const std::string& fragmentShaderPath);

    /**
     * @brief 获取已编译的着色器程序ID
     * @param vertexShaderPath 顶点着色器路径
     * @param fragmentShaderPath 片段着色器路径
     * @return OpenGL程序ID，0表示未找到或编译失败
     */
    GLuint GetShaderProgram(const std::string& vertexShaderPath,
                            const std::string& fragmentShaderPath);

    /**
     * @brief 手动触发重新编译着色器
     * @param vertexShaderPath 顶点着色器路径
     * @param fragmentShaderPath 片段着色器路径
     */
    void ReloadShader(const std::string& vertexShaderPath,
                     const std::string& fragmentShaderPath);

    /**
     * @brief 检查文件修改并触发热重载（应在主线程定期调用）
     */
    void CheckForHotReload();


private:
    // 内部着色器程序存储结构
    std::unordered_map<std::string, ShaderMetadata> shaders_;
    mutable std::mutex shadersMutex_;
    ThreadPool* threadPool_;
    EventBus* eventBus_;

    /**
     * @brief 异步编译任务实现
     * @param vertexPath 顶点着色器路径
     * @param fragmentPath 片段着色器路径
     * @param isReload 是否为重载操作
     */
    void AsyncCompileTask(const std::string& vertexPath,
                         const std::string& fragmentPath,
                         bool isReload);

    /**
     * @brief 实际编译着色器的实现
     * @param vertexSource 顶点着色器源码
     * @param fragmentSource 片段着色器源码
     * @param vertexPath 顶点着色器路径（用于错误报告）
     * @param fragmentPath 片段着色器路径（用于错误报告）
     * @return 编译结果元数据
     */
    ShaderMetadata CompileShader(const std::string& vertexSource,
                                const std::string& fragmentSource,
                                const std::string& vertexPath,
                                const std::string& fragmentPath);

    /**
     * @brief 读取着色器文件内容
     * @param path 文件路径
     * @return 文件内容字符串
     * @throws std::runtime_error 文件读取失败时抛出
     */
    std::string ReadShaderFile(const std::string& path);
    /**
     * @brief 生成着色器存储的键值
     */
    static std::string MakeKey(const std::string& vertexPath,
                              const std::string& fragmentPath) {
        return vertexPath + "|" + fragmentPath;
    }
};