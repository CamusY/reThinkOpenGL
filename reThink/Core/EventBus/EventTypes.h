// EventTypes.h
#ifndef EVENT_TYPES_H
#define EVENT_TYPES_H

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <nlohmann/json.hpp>
#include "EventBus.h"  // 引入 EventBus 以使用 Priority

// 前置声明依赖的结构体
struct ModelData {
    std::string uuid;              // 模型的唯一标识符
    std::string filepath;          // 模型文件路径 (如 glTF, OBJ 等)
    glm::mat4 transform;           // 模型的变换矩阵 (位置、旋转、缩放)
    std::vector<std::string> materialUUIDs; // 关联的材质 UUID 列表
    std::string vertexShaderPath;  // 顶点着色器路径
    std::string fragmentShaderPath;// 片段着色器路径
    std::vector<glm::vec3> vertices; // 顶点数据 (用于渲染和编辑)
    std::vector<unsigned int> indices; // 索引数据 (用于网格渲染)
    std::string parentUUID;        // 父模型的 UUID (支持层级结构，若无则为空)
    std::vector<glm::vec3> normals; // 顶点法线数据 (用于法线贴图和阴影计算)
};

struct KeyframeData {
    std::string modelUUID;         // 关联的模型 UUID
    glm::vec3 position;            // 关键帧位置
    glm::quat rotation;            // 关键帧旋转 (使用四元数表示，避免万向节锁)
    glm::vec3 scale;               // 关键帧缩放
    std::string materialUUID;      // 关联的材质 UUID (支持材质动画)
};

struct MaterialData {
    std::string uuid;              // 材质的唯一标识符
    glm::vec3 diffuseColor;        // 漫反射颜色
    glm::vec3 specularColor;       // 镜面反射颜色
    float shininess;               // 镜面高光系数
    std::string textureUUID;       // 关联的纹理 UUID
    std::string vertexShaderPath;  // 顶点着色器路径
    std::string fragmentShaderPath;// 片段着色器路径
};

struct Operation {
    std::function<void()> execute; // 执行操作的函数
    std::function<void()> undo;    // 撤销操作的函数
};

struct TextureData {
    std::string uuid;              // 纹理的唯一标识符
    std::string filepath;          // 纹理文件路径
    int width;                     // 纹理宽度
    int height;                    // 纹理高度
    int channels;                  // 纹理通道数
    unsigned int textureID;        // OpenGL 纹理 ID
};

namespace MyRenderer {
namespace Events {

    // 定义场景光照更新事件
    struct SceneLightUpdatedEvent {
        glm::vec3 lightDir;   // 光源方向
        glm::vec3 lightColor; // 光源颜色
    };

    // 模型选择更改事件
    struct ModelSelectionChangedEvent {
        std::string modelUUID; // 选中模型的唯一标识
        static constexpr EventBus::Priority priority = EventBus::Priority::High; // 高优先级，影响实时交互
    };

    // 操作模式更改事件
    struct OperationModeChangedEvent {
        enum class Mode { Vertex, Edge, Face, Object };
        Mode mode; // 当前编辑模式
        static constexpr EventBus::Priority priority = EventBus::Priority::Normal; // 普通优先级，模式切换不需实时
    };

    // 模型变换事件
    struct ModelTransformedEvent {
        std::string modelUUID;
        glm::mat4 transform; // 变换矩阵
        static constexpr EventBus::Priority priority = EventBus::Priority::High; // 高优先级，影响渲染
    };

/*    // 着色器编译完成事件
    struct ShaderCompiledEvent {
        std::string vertexPath;
        std::string fragmentPath;
        bool success;
        std::string errorMessage; // 若失败，携带错误信息
        static constexpr EventBus::Priority priority = EventBus::Priority::Normal; // 普通优先级，编译结果不需实时
    };
*/
    // 动画帧更改事件
    struct AnimationFrameChangedEvent {
        float currentTime; // 当前动画时间
        static constexpr EventBus::Priority priority = EventBus::Priority::High; // 高优先级，影响动画实时性
    };

    // 项目打开事件
    struct ProjectOpenedEvent {
        std::string projectPath; // 项目文件路径
        static constexpr EventBus::Priority priority = EventBus::Priority::Normal; // 普通优先级，项目加载不需实时
    };

    // 关键帧添加事件
    struct KeyframeAddedEvent {
        float time;
        KeyframeData keyframe; // 关键帧数据
        static constexpr EventBus::Priority priority = EventBus::Priority::Normal; // 普通优先级，动画编辑不需实时
    };

    // 布局更改事件
    struct LayoutChangeEvent {
        std::string layoutName; // 新布局配置名称
        static constexpr EventBus::Priority priority = EventBus::Priority::Normal; // 普通优先级，布局调整不紧急
    };

    // 打开文件事件
    struct OpenFileEvent {
        std::string filepath; // 文件路径
        static constexpr EventBus::Priority priority = EventBus::Priority::Normal; // 普通优先级，文件操作不需实时
    };

    // 视口聚焦事件
    struct ViewportFocusEvent {
        bool focusState; // 聚焦状态
        static constexpr EventBus::Priority priority = EventBus::Priority::High; // 高优先级，影响用户交互
    };

    // 模型加载完成事件
    struct ModelLoadedEvent {
        ModelData modelData; // 模型数据
        static constexpr EventBus::Priority priority = EventBus::Priority::Normal; // 普通优先级，加载完成不需实时
    };

    // 模型删除事件
    struct ModelDeletedEvent {
        std::string modelUUID; // 被删除模型的唯一标识
        static constexpr EventBus::Priority priority = EventBus::Priority::High; // 高优先级，影响渲染和场景
    };

    // 材质创建事件
    struct MaterialCreatedEvent {
        std::string materialUUID; // 新材质的唯一标识符
        static constexpr EventBus::Priority priority = EventBus::Priority::Normal; // 普通优先级，材质创建不紧急
    };

    // 材质更新事件
    struct MaterialUpdatedEvent {
        std::string materialUUID;    // 材质 UUID
        glm::vec3 diffuseColor;      // 漫反射颜色
        glm::vec3 specularColor;     // 镜面反射颜色
        float shininess;             // 镜面高光系数
        std::string textureUUID;     // 纹理 UUID
        static constexpr EventBus::Priority priority = EventBus::Priority::High; // 高优先级，影响渲染
    };

    // 材质删除事件
    struct MaterialDeletedEvent {
        std::string materialUUID; // 删除的材质 UUID
        static constexpr EventBus::Priority priority = EventBus::Priority::High; // 高优先级，影响渲染
    };

    // 请求材质创建事件
    struct RequestMaterialCreationEvent {
        static constexpr EventBus::Priority priority = EventBus::Priority::Normal; // 普通优先级，请求处理不紧急
    };

    // 请求加载纹理事件
    struct RequestTextureLoadEvent {
        std::string filepath; // 纹理文件路径
        static constexpr EventBus::Priority priority = EventBus::Priority::Normal; // 普通优先级，加载请求不需实时
    };

    // 纹理加载完成事件
    struct TextureLoadedEvent {
        std::string uuid;            // 纹理的 UUID
        std::string filepath;        // 纹理文件路径
        bool success;                // 是否成功
        std::string errorMessage;    // 若失败，携带错误信息
        static constexpr EventBus::Priority priority = EventBus::Priority::Normal; // 普通优先级，加载完成不需实时
    };

    // 纹理删除事件
    struct TextureDeletedEvent {
        std::string textureUUID; // 删除的纹理 UUID
        static constexpr EventBus::Priority priority = EventBus::Priority::High; // 高优先级，影响渲染
    };

    // 动画数据加载完成事件
    struct AnimationDataLoadedEvent {
        std::string animationDataPath;              // 动画数据文件路径
        std::map<float, KeyframeData> keyframes;    // 加载的关键帧数据
        static constexpr EventBus::Priority priority = EventBus::Priority::Normal; // 普通优先级，数据加载不需实时
    };

    // 请求加载动画数据事件
    struct RequestAnimationDataLoadEvent {
        std::string animationDataPath; // 动画数据文件路径
        static constexpr EventBus::Priority priority = EventBus::Priority::Normal; // 普通优先级，请求不紧急
    };

    // 请求保存动画数据事件
    struct RequestAnimationDataSaveEvent {
        std::string animationDataPath; // 动画数据保存路径
        static constexpr EventBus::Priority priority = EventBus::Priority::Normal; // 普通优先级，保存不需实时
    };

    // 动画播放控制事件
    struct AnimationPlaybackControlEvent {
        enum class Action { Play, Pause, Stop };
        Action action; // 播放动作
        static constexpr EventBus::Priority priority = EventBus::Priority::High; // 高优先级，影响动画实时性
    };

    // 动画更新事件
    struct AnimationUpdatedEvent {
        std::map<float, KeyframeData> keyframes; // 更新后的关键帧数据
        static constexpr EventBus::Priority priority = EventBus::Priority::Normal; // 普通优先级，更新不需实时
    };

    // 动画播放开始事件
    struct AnimationPlaybackStartedEvent {
        static constexpr EventBus::Priority priority = EventBus::Priority::High; // 高优先级，影响动画实时性
    };

    // 动画播放停止事件
    struct AnimationPlaybackStoppedEvent {
        static constexpr EventBus::Priority priority = EventBus::Priority::High; // 高优先级，影响动画实时性
    };

    // 关键帧修改事件
    struct KeyframeModifiedEvent {
        float time;
        KeyframeData keyframe; // 修改后的关键帧数据
        static constexpr EventBus::Priority priority = EventBus::Priority::Normal; // 普通优先级，修改不需实时
    };

    // 关键帧删除事件
    struct KeyframeDeletedEvent {
        float time; // 被删除的关键帧时间
        static constexpr EventBus::Priority priority = EventBus::Priority::Normal; // 普通优先级，删除不需实时
    };

    // 请求更改动画帧事件
    struct RequestAnimationFrameChangeEvent {
        float newTime; // 请求的新时间
        static constexpr EventBus::Priority priority = EventBus::Priority::High; // 高优先级，影响动画实时性
    };

    // 推送撤销操作事件
    struct PushUndoOperationEvent {
        Operation op; // 操作对象
        static constexpr EventBus::Priority priority = EventBus::Priority::Normal; // 普通优先级，撤销不需实时
    };

    // 撤销/重做事件
    struct UndoRedoEvent {
        enum class Action { Undo, Redo };
        Action action; // 操作类型
        static constexpr EventBus::Priority priority = EventBus::Priority::Normal; // 普通优先级，撤销/重做不紧急
    };

    // 请求创建模型事件（程序化生成）
    struct RequestModelCreatedEvent {
        std::string algorithmName; // 算法名称
        nlohmann::json params; // 参数（JSON格式）
        static constexpr EventBus::Priority priority = EventBus::Priority::Normal; // 普通优先级，请求不紧急
    };

    // 层级更新事件
    struct HierarchyUpdateEvent {
        std::string parentUUID; // 父模型 UUID
        glm::mat4 transform;    // 父模型的变换
        static constexpr EventBus::Priority priority = EventBus::Priority::High; // 高优先级，影响渲染
    };
    
    // 程序化生成开始事件
    struct ProceduralGenerationStartedEvent {
        static constexpr EventBus::Priority priority = EventBus::Priority::Normal; // 普通优先级，生成开始不需实时
    };

    // 进度更新事件
    struct ProgressUpdateEvent {
        float progress; // 进度值（0.0 到 1.0）
        static constexpr EventBus::Priority priority = EventBus::Priority::Low; // 低优先级，进度更新不紧急
    };

    // 程序化生成完成事件
    struct ProceduralGenerationCompletedEvent {
        bool success;              // 是否成功
        std::string errorMessage;  // 若失败，携带错误信息
        ModelData modelData;       // 生成的模型数据（成功时有效）
        static constexpr EventBus::Priority priority = EventBus::Priority::Normal; // 普通优先级
    };

    // 请求取消生成事件
    struct RequestGenerationCancelEvent {
        static constexpr EventBus::Priority priority = EventBus::Priority::Normal; // 普通优先级，取消请求不紧急
    };

    // 程序化生成停止事件
    struct ProceduralGenerationStoppedEvent {
        static constexpr EventBus::Priority priority = EventBus::Priority::Normal; // 普通优先级，停止不需实时
    };

    // 项目保存事件
    struct ProjectSavedEvent {
        std::string projectPath; // 保存的项目路径
        static constexpr EventBus::Priority priority = EventBus::Priority::Normal; // 普通优先级，保存不需实时
    };

    // 项目加载失败事件
    struct ProjectLoadFailedEvent {
        std::string errorMsg; // 错误信息
        static constexpr EventBus::Priority priority = EventBus::Priority::Normal; // 普通优先级，失败通知不紧急
    };

/*    // 请求编译着色器事件
    struct ShaderCompileRequestEvent {
        std::string vertexPath;      // 顶点着色器文件路径
        std::string fragmentPath;    // 片段着色器文件路径
        std::string vertexCode;      // 顶点着色器代码（可选）
        std::string fragmentCode;    // 片段着色器代码（可选）
        static constexpr EventBus::Priority priority = EventBus::Priority::Normal;
    };
*/
    // 变换工具事件
    struct TransformToolEvent {
        enum class Tool { Translate, Rotate, Scale };
        Tool tool; // 变换工具类型
        static constexpr EventBus::Priority priority = EventBus::Priority::High; // 高优先级，影响交互和渲染
    };

    // 请求新建项目事件
    struct RequestNewProjectEvent {
        std::string projectName; // 项目名称
        std::string projectDir; // 项目目录
        static constexpr EventBus::Priority priority = EventBus::Priority::Normal; // 普通优先级，新建项目不紧急
    };

    // 请求打开项目事件
    struct RequestOpenProjectEvent {
        std::string projectPath; // 项目文件路径
        static constexpr EventBus::Priority priority = EventBus::Priority::Normal; // 普通优先级，打开项目不紧急
    };

    // 请求保存项目事件
    struct RequestSaveProjectEvent {
        std::string projectPath; // 项目保存路径
        static constexpr EventBus::Priority priority = EventBus::Priority::Normal; // 普通优先级，保存不需实时
    };

    // 模型创建事件（程序化生成结果）
    struct ModelCreatedEvent {
        ModelData modelData; // 生成的模型数据
        static constexpr EventBus::Priority priority = EventBus::Priority::Normal; // 普通优先级，创建完成不需实时
    };

    // 纹理更新事件
    struct TextureUpdatedEvent {
        std::string filepath; // 更新后的纹理路径
        static constexpr EventBus::Priority priority = EventBus::Priority::High; // 高优先级，影响渲染
    };

} // namespace Events
} // namespace MyRenderer

#endif // EVENT_TYPES_H
