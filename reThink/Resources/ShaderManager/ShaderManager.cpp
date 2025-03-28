#include "ShaderManager.h"
#include <fstream>
#include <sstream>
#include <iostream>

ShaderManager::ShaderManager(std::shared_ptr<ThreadPool> threadPool)
    : threadPool_(std::move(threadPool)) {
    LoadDefaultShaders();
}

ShaderManager::~ShaderManager() {
    std::lock_guard<std::mutex> lock(shaderMutex_);
    for (auto& [key, program] : shaderPrograms_) {
        if (program != 0) {
            glDeleteProgram(program);
        }
    }
    shaderPrograms_.clear();
}

std::future<ShaderManager::CompileResult> ShaderManager::CompileShaderAsync(const std::string& vertexPath,
                                                                            const std::string& fragmentPath) {
    auto task = [this, vertexPath, fragmentPath]() {
        CompileShaderTask(vertexPath, fragmentPath);
        return CompileResult{true, ""}; // 编译结果通过日志反馈
    };
    return threadPool_->EnqueueTask(std::move(task));
}

GLuint ShaderManager::GetShaderProgram(const std::string& vertexPath, const std::string& fragmentPath) const {
    std::lock_guard<std::mutex> lock(shaderMutex_);
    std::string key = GetShaderKey(vertexPath, fragmentPath);
    auto it = shaderPrograms_.find(key);
    return (it != shaderPrograms_.end()) ? it->second : 0;
}

void ShaderManager::ReloadShader(const std::string& vertexPath, const std::string& fragmentPath) {
    std::lock_guard<std::mutex> lock(shaderMutex_);
    std::string key = GetShaderKey(vertexPath, fragmentPath);
    auto it = shaderPrograms_.find(key);
    if (it != shaderPrograms_.end() && it->second != 0) {
        glDeleteProgram(it->second);
        shaderPrograms_.erase(it);
    }
    CompileShaderAsync(vertexPath, fragmentPath);
}

void ShaderManager::CheckForHotReload() {
    std::lock_guard<std::mutex> lock(shaderMutex_);
    for (auto& [key, program] : shaderPrograms_) {
        size_t delimiterPos = key.find('|');
        if (delimiterPos == std::string::npos) continue;
        std::string vertexPath = key.substr(0, delimiterPos);
        std::string fragmentPath = key.substr(delimiterPos + 1);

        auto vertexIt = fileTimestamps_.find(vertexPath);
        auto fragmentIt = fileTimestamps_.find(fragmentPath);
        if (vertexIt == fileTimestamps_.end() || fragmentIt == fileTimestamps_.end()) continue;

        if (HasFileChanged(vertexPath, vertexIt->second) || HasFileChanged(fragmentPath, fragmentIt->second)) {
            std::cout << "Hot reloading shader: " << vertexPath << " and " << fragmentPath << std::endl;
            ReloadShader(vertexPath, fragmentPath);
        }
    }
}

void ShaderManager::LoadDefaultShaders() {
    // 假设默认着色器文件路径
    std::string defaultVertexPath = "Shaders/default.vs";
    std::string defaultFragmentPath = "Shaders/default.fs";

    // 检查文件是否存在，若不存在则创建
    if (!fs::exists(defaultVertexPath)) {
        std::ofstream file(defaultVertexPath);
        if (file.is_open()) {
            file << R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
out vec3 FragPos;
out vec3 Normal;
void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";
            file.close();
        }
    }

    if (!fs::exists(defaultFragmentPath)) {
        std::ofstream file(defaultFragmentPath);
        if (file.is_open()) {
            file << R"(
#version 330 core
in vec3 FragPos;
in vec3 Normal;
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 viewPos;
uniform vec3 diffuseColor;
uniform vec3 specularColor;
uniform float shininess;
out vec4 FragColor;
void main() {
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;
    vec3 norm = normalize(Normal);
    vec3 lightDirNorm = normalize(-lightDir);
    float diff = max(dot(norm, lightDirNorm), 0.0);
    vec3 diffuse = diff * lightColor * diffuseColor;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfwayDir = normalize(lightDirNorm + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), shininess);
    vec3 specular = spec * lightColor * specularColor;
    vec3 result = (ambient + diffuse + specular);
    FragColor = vec4(result, 1.0);
}
)";
            file.close();
        }
    }

    // 编译默认着色器
    //CompileShaderAsync(defaultVertexPath, defaultFragmentPath);
    CompileShaderTask(defaultVertexPath, defaultFragmentPath);
    GLuint program = GetShaderProgram(defaultVertexPath, defaultFragmentPath);
    if (program == 0) {
        std::cerr << "错误: 默认着色器编译失败!" << std::endl;
    } else {
        std::cout << "默认着色器编译成功, 程序ID: " << program << std::endl;
    }
}

GLuint ShaderManager::CompileShader(GLenum shaderType, const std::string& source, std::string& errorMessage) {
    GLuint shader = glCreateShader(shaderType);
    const char* sourceCStr = source.c_str();
    glShaderSource(shader, 1, &sourceCStr, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        errorMessage = std::string("Shader compilation failed: ") + infoLog;
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint ShaderManager::LinkProgram(GLuint vertexShader, GLuint fragmentShader, std::string& errorMessage) {
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        errorMessage = std::string("Program linking failed: ") + infoLog;
        glDeleteProgram(program);
        return 0;
    }
    return program;
}

std::string ShaderManager::ReadShaderFile(const std::string& filepath) {
    std::cout << "尝试读取着色器文件: " << filepath << std::endl;
    std::cout << "绝对路径: " << std::filesystem::absolute(filepath) << std::endl;
    
    // 检查文件是否存在
    if (!fs::exists(filepath)) {
        std::cerr << "错误: 着色器文件不存在: " << filepath << std::endl;
        return "";
    }

    std::ifstream file(filepath);
    if (!file.is_open()) {
        char buffer[256];
        strerror_s(buffer, sizeof(buffer), errno);
        std::cerr << "无法打开着色器文件: " << filepath 
                  << " 错误: " << buffer << std::endl;
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    
    // 检查读取的内容是否为空
    std::string content = buffer.str();
    if (content.empty()) {
        std::cerr << "警告: 着色器文件内容为空: " << filepath << std::endl;
    }
    
    return content;
}

std::string ShaderManager::GetShaderKey(const std::string& vertexPath, const std::string& fragmentPath) const {
    return vertexPath + "|" + fragmentPath;
}

void ShaderManager::CompileShaderTask(const std::string& vertexPath, const std::string& fragmentPath) {
    std::cout << "开始编译着色器: " << vertexPath << " 和 " << fragmentPath << std::endl;
    
    std::string vertexSource = ReadShaderFile(vertexPath);
    std::string fragmentSource = ReadShaderFile(fragmentPath);
    std::string errorMessage;

    if (vertexSource.empty()) {
        std::cerr << "错误: 顶点着色器源为空: " << vertexPath << std::endl;
        return;
    }
    if (fragmentSource.empty()) {
        std::cerr << "错误: 片段着色器源为空: " << fragmentPath << std::endl;
        return;
    }

    if (vertexSource.empty() || fragmentSource.empty()) {
        std::cerr << "Failed to read shader source: " << vertexPath << " or " << fragmentPath << std::endl;
        return;
    }

    GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, vertexSource, errorMessage);
    if (vertexShader == 0) {
        std::cerr << errorMessage << std::endl;
        return;
    }

    GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentSource, errorMessage);
    if (fragmentShader == 0) {
        glDeleteShader(vertexShader);
        std::cerr << errorMessage << std::endl;
        return;
    }

    GLuint program = LinkProgram(vertexShader, fragmentShader, errorMessage);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    if (program == 0) {
        std::cerr << errorMessage << std::endl;
        return;
    }

    {
        std::lock_guard<std::mutex> lock(shaderMutex_);
        std::string key = GetShaderKey(vertexPath, fragmentPath);
        shaderPrograms_[key] = program;
        if (!vertexPath.empty()) fileTimestamps_[vertexPath] = fs::last_write_time(vertexPath);
        if (!fragmentPath.empty()) fileTimestamps_[fragmentPath] = fs::last_write_time(fragmentPath);
    }
}

bool ShaderManager::HasFileChanged(const std::string& filepath, fs::file_time_type& lastWriteTime) {
    try {
        fs::file_time_type currentTime = fs::last_write_time(filepath);
        if (currentTime != lastWriteTime) {
            lastWriteTime = currentTime;
            return true;
        }
        return false;
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
        return false;
    }
}