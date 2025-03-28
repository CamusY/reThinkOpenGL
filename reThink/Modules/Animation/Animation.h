#ifndef ANIMATION_H
#define ANIMATION_H

#include <map>
#include <memory>
#include <string>
#include <functional>
#include <imgui.h>
#include "EventBus/EventBus.h"
#include "EventBus/EventTypes.h"
#include "Utils/JSONSerializer.h"

namespace MyRenderer {

    class Animation {
    public:
        Animation(std::shared_ptr<EventBus> eventBus);
        ~Animation();

        // 禁止拷贝和赋值
        Animation(const Animation&) = delete;
        Animation& operator=(const Animation&) = delete;

        // 更新动画窗口 UI
        void Update();

        // 加载动画数据到 UI
        void LoadAnimationData(const std::map<float, KeyframeData>& keyframes);

        // 添加关键帧
        void AddKeyframe(float time, const KeyframeData& keyframe);

        // 删除关键帧
        void RemoveKeyframe(float time);

        // 修改关键帧
        void ModifyKeyframe(float time, const KeyframeData& keyframe);

    private:
        // 初始化事件订阅
        void SetupEventSubscriptions();

        // 绘制时间轴
        void RenderTimeline();

        // 绘制关键帧编辑器
        void RenderKeyframeEditor();

        // 绘制播放控制按钮
        void RenderPlaybackControls();

        // 处理时间滑块拖动
        void HandleTimeSlider();
        
        // 处理层级更新
        void PropagateHierarchyUpdate(const std::string& modelUUID, const glm::mat4& transform);

        // 成员变量
        std::shared_ptr<EventBus> eventBus_;            // 事件总线
        std::map<float, KeyframeData> keyframes_;       // 关键帧数据
        float currentTime_ = 0.0f;                      // 当前动画时间（与滑块绑定）
        float maxTime_ = 10.0f;                         // 动画总时长，默认 10 秒
        bool isPlaying_ = false;                        // 是否正在播放（用于 UI 状态）
        EventBus::SubscriberId frameChangedSubId_;      // 帧更改事件的订阅 ID
        EventBus::SubscriberId dataLoadedSubId_;        // 数据加载事件的订阅 ID
        EventBus::SubscriberId updatedSubId_;           // 动画更新事件的订阅 ID
        EventBus::SubscriberId playbackStartedSubId_;   // 播放开始事件的订阅 ID
        EventBus::SubscriberId playbackStoppedSubId_;   // 播放停止事件的订阅 ID
    };

} // namespace MyRenderer

#endif // ANIMATION_H