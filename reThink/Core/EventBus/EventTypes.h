// EventTypes.h
#ifndef EVENT_TYPES_H
#define EVENT_TYPES_H

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

// 前置声明依赖的结构体（假设已在其他头文件中定义）
struct ModelData
{
};

struct KeyframeData
{
};

struct MaterialData
{
};

struct Operation
{
};

struct TextureData
{
};

namespace MyRenderer
{
    namespace Events
    {
        // 模型选择更改事件
        struct ModelSelectionChangedEvent
        {
            std::string modelUUID; // 选中模型的唯一标识
        };

        // 操作模式更改事件
        struct OperationModeChangedEvent
        {
            enum class Mode { Vertex, Edge, Face, Object };

            Mode mode; // 当前编辑模式
        };

        // 模型变换事件
        struct ModelTransformedEvent
        {
            std::string modelUUID;
            glm::mat4 transform; // 变换矩阵
        };

        // 着色器编译完成事件
        struct ShaderCompiledEvent
        {
            std::string vertexPath;
            std::string fragmentPath;
            bool success;
            std::string errorMessage; // 若失败，携带错误信息
        };

        // 动画帧更改事件
        struct AnimationFrameChangedEvent
        {
            float currentTime; // 当前动画时间
        };

        // 项目打开事件
        struct ProjectOpenedEvent
        {
            std::string projectPath; // 项目文件路径
        };

        // 关键帧添加事件
        struct KeyframeAddedEvent
        {
            float time;
            KeyframeData keyframe; // 关键帧数据
        };

        // 布局更改事件
        struct LayoutChangeEvent
        {
            std::string layoutName; // 新布局配置名称
        };

        // 打开文件事件
        struct OpenFileEvent
        {
            std::string filepath; // 文件路径
        };

        // 视口聚焦事件
        struct ViewportFocusEvent
        {
            bool focusState; // 聚焦状态
        };

        // 模型加载完成事件
        struct ModelLoadedEvent
        {
            ModelData modelData; // 模型数据
        };

        // 模型删除事件
        struct ModelDeletedEvent
        {
            std::string modelUUID; // 被删除模型的唯一标识
        };

        // 材质更新事件
        struct MaterialUpdatedEvent
        {
            std::string materialUUID;
            MaterialData data; // 材质数据
        };

        // 请求加载纹理事件
        struct RequestTextureLoadEvent
        {
            std::string filepath; // 纹理文件路径
        };

        // 纹理加载完成事件
        struct TextureLoadedEvent
        {
            std::string filepath; // 加载完成的纹理文件路径
        };

        // 动画数据加载完成事件
        struct AnimationDataLoadedEvent
        {
            std::string animationDataPath; // 动画数据文件路径
        };

        // 请求加载动画数据事件
        struct RequestAnimationDataLoadEvent
        {
            std::string animationDataPath; // 动画数据文件路径
        };

        // 请求保存动画数据事件
        struct RequestAnimationDataSaveEvent
        {
            std::string animationDataPath; // 动画数据保存路径
        };

        // 动画播放控制事件
        struct AnimationPlaybackControlEvent
        {
            enum class Action { Play, Pause, Stop };

            Action action; // 播放动作
        };

        // 动画更新事件
        struct AnimationUpdatedEvent
        {
            std::map<float, KeyframeData> keyframes; // 更新后的关键帧数据
        };

        // 动画播放开始事件
        struct AnimationPlaybackStartedEvent
        {
            // 无额外数据，标记播放开始
        };

        // 动画播放停止事件
        struct AnimationPlaybackStoppedEvent
        {
            // 无额外数据，标记播放停止
        };

        // 关键帧修改事件
        struct KeyframeModifiedEvent
        {
            float time;
            KeyframeData keyframe; // 修改后的关键帧数据
        };

        // 关键帧删除事件
        struct KeyframeDeletedEvent
        {
            float time; // 被删除的关键帧时间
        };

        // 请求更改动画帧事件
        struct RequestAnimationFrameChangeEvent
        {
            float newTime; // 请求的新时间
        };

        // 推送撤销操作事件
        struct PushUndoOperationEvent
        {
            Operation op; // 操作对象
        };

        // 撤销/重做事件
        struct UndoRedoEvent
        {
            enum class Action { Undo, Redo };

            Action action; // 操作类型
        };

        // 请求创建模型事件（程序化生成）
        struct RequestModelCreatedEvent
        {
            std::string algorithmName; // 算法名称
            nlohmann::json params; // 参数（JSON格式）
        };

        // 程序化生成开始事件
        struct ProceduralGenerationStartedEvent
        {
            // 无额外数据，标记生成开始
        };

        // 进度更新事件
        struct ProgressUpdateEvent
        {
            float progress; // 进度值（0.0 到 1.0）
        };

        // 程序化生成完成事件
        struct ProceduralGenerationCompletedEvent
        {
            bool success; // 是否成功
            std::string errorMessage; // 若失败，携带错误信息
        };

        // 请求取消生成事件
        struct RequestGenerationCancelEvent
        {
            // 无额外数据，标记取消请求
        };

        // 程序化生成停止事件
        struct ProceduralGenerationStoppedEvent
        {
            // 无额外数据，标记生成停止
        };

        // 项目保存事件
        struct ProjectSavedEvent
        {
            std::string projectPath; // 保存的项目路径
        };

        // 项目加载失败事件
        struct ProjectLoadFailedEvent
        {
            std::string errorMsg; // 错误信息
        };

        // 请求编译着色器事件
        struct ShaderCompileRequestEvent
        {
            std::string filepath; // 着色器文件路径
        };

        // 变换工具事件
        struct TransformToolEvent
        {
            enum class Tool { Translate, Rotate, Scale };

            Tool tool; // 变换工具类型
        };

        // 请求新建项目事件
        struct RequestNewProjectEvent
        {
            std::string projectName; // 项目名称
            std::string projectDir; // 项目目录
        };

        // 请求打开项目事件
        struct RequestOpenProjectEvent
        {
            std::string projectPath; // 项目文件路径
        };

        // 请求保存项目事件
        struct RequestSaveProjectEvent
        {
            std::string projectPath; // 项目保存路径
        };

        // 模型创建事件（程序化生成结果）
        struct ModelCreatedEvent
        {
            ModelData modelData; // 生成的模型数据
        };

        // 纹理更新事件
        struct TextureUpdatedEvent
        {
            std::string filepath; // 更新后的纹理路径
        };
    } // namespace Events
} // namespace MyRenderer

#endif // EVENT_TYPES_H
