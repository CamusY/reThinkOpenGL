// EventTypes.h
#pragma once
#include <string>
#include <glm/glm.hpp>

#include "imgui-docking/imgui_impl_opengl3_loader.h"

struct ModelData;
class ModelLoader;
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
        std::string vertexShaderPath;   // 顶点着色器路径
        std::string fragmentShaderPath;  // 片段着色器路径
        bool success;                    // 编译是否成功
        std::string errorLog;            // 错误信息（成功时为空）
        GLuint programId;                // 着色器程序ID（即使编译失败也保留旧ID）
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

    // 模型加载成功事件
    struct ModelLoadedEvent {
        std::string modelUUID;          // 模型唯一标识
        std::shared_ptr<ModelData> data;// 模型数据
        std::string filePath;           // 模型文件路径
    };

    // 模型加载失败事件
    struct ModelLoadFailedEvent {
        std::string modelUUID;          // 模型唯一标识
        std::string filePath;           // 模型文件路径
        std::string errorMessage;       // 错误信息
    };
    
    struct LayoutChangedEvent {}; // 布局变更事件
    struct WindowVisibilityEvent { // 窗口可见性事件
        std::string windowName;
        bool visible;
    };
    struct ProjectCreatedEvent {}; // 项目创建事件
    struct ProjectOpenedEvent {};  // 项目打开事件
}