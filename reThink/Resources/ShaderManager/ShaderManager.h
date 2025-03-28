#ifndef SHADER_MANAGER_H
#define SHADER_MANAGER_H

#include <string>
#include <unordered_map>
#include <memory>
#include <filesystem>
#include <future>
#include <mutex>
#include <glad/glad.h>
#include "ThreadPool/ThreadPool.h"

namespace fs = std::filesystem;

class ShaderManager {
public:
    struct CompileResult {
        bool success;
        std::string errorMessage;
    };

    ShaderManager(std::shared_ptr<ThreadPool> threadPool);
    ~ShaderManager();

    std::future<CompileResult> CompileShaderAsync(const std::string& vertexPath, const std::string& fragmentPath);

    GLuint GetShaderProgram(const std::string& vertexPath, const std::string& fragmentPath) const;

    void ReloadShader(const std::string& vertexPath, const std::string& fragmentPath);

    void CheckForHotReload();

private:
    void LoadDefaultShaders();
    GLuint CompileShader(GLenum shaderType, const std::string& source, std::string& errorMessage);
    GLuint LinkProgram(GLuint vertexShader, GLuint fragmentShader, std::string& errorMessage);
    std::string ReadShaderFile(const std::string& filepath);
    std::string GetShaderKey(const std::string& vertexPath, const std::string& fragmentPath) const;
    void CompileShaderTask(const std::string& vertexPath, const std::string& fragmentPath);

    bool HasFileChanged(const std::string& filepath, fs::file_time_type& lastWriteTime);

    std::shared_ptr<ThreadPool> threadPool_;
    std::unordered_map<std::string, GLuint> shaderPrograms_;
    std::unordered_map<std::string, fs::file_time_type> fileTimestamps_;
    mutable std::mutex shaderMutex_;
};

#endif // SHADER_MANAGER_H