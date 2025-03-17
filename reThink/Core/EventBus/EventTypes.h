// EventTypes.h
#pragma once
#include <string>
#include <glm/glm.hpp>

/**
 * @brief 定义所有事件类型的数据结构
 * @note 本模块不涉及依赖注入，仅作数据容器
 */
namespace EventTypes {

    // 模型变换事件
    struct ModelTransformedEvent {
        glm::mat4 transformMatrix;  // 世界变换矩阵
        std::string modelUUID;      // 模型唯一标识
    };

    // 着色器编译事件
    struct ShaderCompiledEvent {
        std::string shaderPath;     // 着色器文件路径
        bool success;               // 编译是否成功
        std::string errorLog;       // 错误信息（成功时为空）
    };

    // 场景加载事件
    struct SceneLoadedEvent {
        std::string scenePath;      // 场景文件路径
        int totalModels;            // 加载的模型数量
    };

    // 通用错误事件
    struct ErrorEvent {
        std::string module;         // 错误来源模块
        std::string message;         // 错误详细信息
        int severity;               // 错误等级（0-紧急,1-严重,2-警告）
    };
}