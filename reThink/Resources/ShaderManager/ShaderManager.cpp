// Resources/ShaderManager.cpp
#include "ShaderManager.h"
#include <fstream>
#include <sstream>
#include <glm/glm.hpp>

ShaderManager::ShaderManager(ThreadPool* threadPool, EventBus* eventBus)
    : threadPool_(threadPool), eventBus_(eventBus) {
    if (!threadPool_ || !eventBus_) {
        throw std::invalid_argument("ThreadPool和EventBus指针不能为空");
    }
}

bool ShaderManager::LoadShaderFromFile(const std::string& vertexPath, 
                                     const std::string& fragmentPath) {
    const auto key = MakeKey(vertexPath, fragmentPath);
    
    std::lock_guard<std::mutex> lock(shadersMutex_);
    if (shaders_.find(key) != shaders_.end()) {
        return false; // 已存在
    }

    // 初始化元数据
    ShaderMetadata meta;
    try {
        meta.vertexPath = vertexPath;   // 新增
        meta.fragmentPath = fragmentPath; // 新增
        meta.lastVertexWrite = fs::last_write_time(vertexPath);
        meta.lastFragmentWrite = fs::last_write_time(fragmentPath);
    } catch (const fs::filesystem_error&) {
        return false;
    }

    shaders_[key] = meta;
    
    // 提交异步任务
    threadPool_->Enqueue([this, vertexPath, fragmentPath] {
        AsyncCompileTask(vertexPath, fragmentPath, false);
    });
    
    return true;
}

GLuint ShaderManager::GetShaderProgram(const std::string& vertexPath,
                                      const std::string& fragmentPath) {
    const auto key = MakeKey(vertexPath, fragmentPath);
    std::lock_guard<std::mutex> lock(shadersMutex_);
    auto it = shaders_.find(key);
    return (it != shaders_.end() && !it->second.isCompiling) ? 
           it->second.programId : 0;
}

void ShaderManager::ReloadShader(const std::string& vertexPath,
                                const std::string& fragmentPath) {
    const auto key = MakeKey(vertexPath, fragmentPath);
    
    std::lock_guard<std::mutex> lock(shadersMutex_);
    if (shaders_.find(key) == shaders_.end()) return;

    // 标记为正在编译
    shaders_[key].isCompiling = true;
    
    threadPool_->Enqueue([this, vertexPath, fragmentPath] {
        AsyncCompileTask(vertexPath, fragmentPath, true);
    });
}

void ShaderManager::CheckForHotReload() {
    std::lock_guard<std::mutex> lock(shadersMutex_);
    
    for (auto& [key, meta] : shaders_) {
        if (meta.isCompiling) continue;

        try {
            const size_t separator = key.find('|');
            const std::string vertexPath = key.substr(0, separator);
            const std::string fragmentPath = key.substr(separator+1);

            const auto currentVertexWrite = fs::last_write_time(vertexPath);
            const auto currentFragmentWrite = fs::last_write_time(fragmentPath);

            if (currentVertexWrite > meta.lastVertexWrite ||
                currentFragmentWrite > meta.lastFragmentWrite) {
                
                meta.isCompiling = true;
                meta.lastVertexWrite = currentVertexWrite;
                meta.lastFragmentWrite = currentFragmentWrite;
                
                threadPool_->Enqueue([this, vertexPath, fragmentPath] {
                    AsyncCompileTask(vertexPath, fragmentPath, true);
                });
            }
        } catch (...) {
            // 文件可能被删除，忽略错误
        }
    }
}



// 实现其他私有方法...
void ShaderManager::AsyncCompileTask(const std::string& vertexPath,
                                   const std::string& fragmentPath,
                                   bool isReload) {
    // 读取着色器源码
    std::string vertexSource, fragmentSource;
    try {
        vertexSource = ReadShaderFile(vertexPath);
        fragmentSource = ReadShaderFile(fragmentPath);
    } catch (const std::exception& e) {
        // 文件读取失败处理
        std::lock_guard<std::mutex> lock(shadersMutex_);
        const auto key = MakeKey(vertexPath, fragmentPath);
        auto& meta = shaders_[key];
        meta.isCompiling = false;
        meta.lastError = e.what();
        
        // 发布错误事件
        EventTypes::ShaderCompiledEvent event;
        event.vertexShaderPath = vertexPath;
        event.fragmentShaderPath = fragmentPath;
        event.success = false;
        event.errorLog = e.what();
        event.programId = meta.programId; // 保留旧程序ID
        eventBus_->Publish(event);
        return;
    }

    // 执行实际编译
    auto newMeta = CompileShader(vertexSource, fragmentSource, vertexPath, fragmentPath);
    
    // 更新元数据
    {
        std::lock_guard<std::mutex> lock(shadersMutex_);
        const auto key = MakeKey(vertexPath, fragmentPath);
        auto& existingMeta = shaders_[key];
        
        // 保留旧程序ID用于事件报告
        newMeta.programId = existingMeta.programId;
        
        // 若编译成功则替换程序ID
        if (newMeta.success && newMeta.programId != 0) {
            if (existingMeta.programId != 0) {
                glDeleteProgram(existingMeta.programId); // 删除旧程序
            }
            existingMeta.programId = newMeta.programId;
        }
        
        // 更新其他元数据
        existingMeta.isCompiling = false;
        existingMeta.lastError = newMeta.lastError;
    }

    // 发布编译结果事件
    EventTypes::ShaderCompiledEvent event;
    event.vertexShaderPath = vertexPath;
    event.fragmentShaderPath = fragmentPath;
    event.success = newMeta.success;
    event.errorLog = newMeta.lastError;
    event.programId = newMeta.programId;
    eventBus_->Publish(event);
}

ShaderMetadata ShaderManager::CompileShader(const std::string& vertexSource,
                                          const std::string& fragmentSource,
                                          const std::string& vertexPath,
                                          const std::string& fragmentPath) {
    ShaderMetadata result;
    result.success = false;

    // 创建着色器对象
    auto CompileStage = [](GLenum type, const std::string& source, const std::string& path) -> GLuint {
        GLuint shader = glCreateShader(type);
        const char* src = source.c_str();
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);

        // 检查编译状态
        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            GLchar infoLog[512];
            glGetShaderInfoLog(shader, 512, nullptr, infoLog);
            glDeleteShader(shader);
            throw std::runtime_error("着色器编译错误(" + path + "):\n" + infoLog);
        }
        return shader;
    };

    GLuint vertexShader = 0;
    GLuint fragmentShader = 0;
    try {
        // 编译各阶段
        vertexShader = CompileStage(GL_VERTEX_SHADER, vertexSource, vertexPath);
        fragmentShader = CompileStage(GL_FRAGMENT_SHADER, fragmentSource, fragmentPath);

        // 创建程序对象
        result.programId = glCreateProgram();
        glAttachShader(result.programId, vertexShader);
        glAttachShader(result.programId, fragmentShader);
        glLinkProgram(result.programId);

        // 检查链接状态
        GLint linkSuccess;
        glGetProgramiv(result.programId, GL_LINK_STATUS, &linkSuccess);
        if (!linkSuccess) {
            GLchar infoLog[512];
            glGetProgramInfoLog(result.programId, 512, nullptr, infoLog);
            throw std::runtime_error("着色器链接错误:\n" + std::string(infoLog));
        }

        result.success = true;
    } catch (const std::exception& e) {
        result.lastError = e.what();
        if (result.programId != 0) {
            glDeleteProgram(result.programId);
            result.programId = 0;
        }
    }

    // 清理临时着色器对象
    if (vertexShader != 0) glDeleteShader(vertexShader);
    if (fragmentShader != 0) glDeleteShader(fragmentShader);
    return result;
}

std::string ShaderManager::ReadShaderFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("无法打开文件: " + path);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    if (buffer.str().empty()) {
        throw std::runtime_error("空文件: " + path);
    }
    return buffer.str();
}