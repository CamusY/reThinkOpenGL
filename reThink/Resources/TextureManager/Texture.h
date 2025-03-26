#pragma once
#include <glad/glad.h>
#include <string>
#include <memory>
#include <filesystem>

namespace fs = std::filesystem;

/**
 * @brief 纹理资源封装类，管理OpenGL纹理生命周期
 */
struct Texture
{
    GLuint id = 0;              // OpenGL纹理ID
    int width = 0;              // 纹理宽度
    int height = 0;             // 纹理高度
    int channels = 0;           // 颜色通道数
    std::string path;           // 纹理文件路径
    fs::file_time_type lastWriteTime; // 文件最后修改时间

    // 纹理参数
    GLenum wrapS = GL_REPEAT;   // S轴环绕方式
    GLenum wrapT = GL_REPEAT;   // T轴环绕方式
    GLenum minFilter = GL_LINEAR_MIPMAP_LINEAR; // 缩小过滤
    GLenum magFilter = GL_LINEAR; // 放大过滤

    ~Texture() {
        if (id != 0) {
            glDeleteTextures(1, &id);
        }
    }
};