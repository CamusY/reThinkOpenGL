#ifndef ANIMATION_MANAGER_H
#define ANIMATION_MANAGER_H

#include <map>
#include <memory>
#include <functional>
#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "EventBus/EventBus.h"
#include "EventBus/EventTypes.h"
#include "Utils/JSONSerializer.h"

namespace MyRenderer {

class AnimationManager {
public:
    // 插值方法枚举
    enum class InterpolationMethod { Linear, Spline };

    // 构造函数，接收 JSONSerializer 和 EventBus 的共享指针
    explicit AnimationManager(std::shared_ptr<JSONSerializer> jsonSerializer,
                             std::shared_ptr<EventBus> eventBus);
    ~AnimationManager();

    // 禁止拷贝和赋值
    AnimationManager(const AnimationManager&) = delete;
    AnimationManager& operator=(const AnimationManager&) = delete;

    // 从 .json 文件加载动画数据
    bool LoadAnimationData(const std::filesystem::path& projectFilePath);

    // 保存动画数据到 .json 文件
    bool SaveAnimationData(const std::filesystem::path& projectFilePath);

    // 添加关键帧并记录操作到 UndoRedoManager
    void AddKeyframe(float time, const KeyframeData& keyframeData);

    // 删除指定时间的关键帧并记录操作
    void RemoveKeyframe(float time);

    // 修改指定时间的关键帧
    void ModifyKeyframe(float time, const KeyframeData& keyframeData);

    // 获取所有关键帧数据
    const std::map<float, KeyframeData>& GetKeyframes() const;

    // 获取指定时间的模型变换矩阵（插值计算）
    glm::mat4 GetModelTransformAtTime(float time);

    // 设置插值方法
    void SetInterpolationMethod(InterpolationMethod method);

    // 启动动画播放
    void PlayAnimation();

    // 暂停动画播放
    void PauseAnimation();

    // 在主渲染循环中调用，更新动画时间并发布帧更新事件
    void Update(float deltaTime);

    // 获取动画播放状态
    bool IsPlaying() const { return isPlaying_; }

private:
   
    // 初始化事件订阅
    void SetupEventSubscriptions();

    // 计算两关键帧之间的插值变换矩阵
    glm::mat4 InterpolateKeyframes(float time, float prevTime, const KeyframeData& prev, float nextTime, const KeyframeData& next) const;

    // 将关键帧数据序列化为 JSON
    void SerializeKeyframes(nlohmann::json& j) const;

    // 从 JSON 反序列化关键帧数据
    void DeserializeKeyframes(const nlohmann::json& j);

    // 成员变量
    std::shared_ptr<JSONSerializer> jsonSerializer_; // JSON 序列化工具
    std::shared_ptr<EventBus> eventBus_;             // 事件总线
    std::map<float, KeyframeData> keyframes_;        // 关键帧数据
    InterpolationMethod interpMethod_ = InterpolationMethod::Linear; // 默认插值方法
    bool isPlaying_ = false;                         // 是否正在播放
    float currentTime_ = 0.0f;                       // 当前动画时间（秒）
    double startTime_ = 0.0;                         // 动画开始时间戳
};

} // namespace MyRenderer
#endif // ANIMATION_MANAGER_H