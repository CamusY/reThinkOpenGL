#include "Animation.h"
#include <imgui.h>
#include <algorithm>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

namespace MyRenderer {

Animation::Animation(std::shared_ptr<EventBus> eventBus)
    : eventBus_(eventBus) {
    SetupEventSubscriptions();
}

Animation::~Animation() {
    // 取消事件订阅
    eventBus_->Unsubscribe(std::type_index(typeid(Events::AnimationFrameChangedEvent)), frameChangedSubId_);
    eventBus_->Unsubscribe(std::type_index(typeid(Events::AnimationDataLoadedEvent)), dataLoadedSubId_);
    eventBus_->Unsubscribe(std::type_index(typeid(Events::AnimationUpdatedEvent)), updatedSubId_);
    eventBus_->Unsubscribe(std::type_index(typeid(Events::AnimationPlaybackStartedEvent)), playbackStartedSubId_);
    eventBus_->Unsubscribe(std::type_index(typeid(Events::AnimationPlaybackStoppedEvent)), playbackStoppedSubId_);
}

void Animation::SetupEventSubscriptions() {
    // 订阅 AnimationFrameChangedEvent，同步时间轴位置
    frameChangedSubId_ = eventBus_->Subscribe<Events::AnimationFrameChangedEvent>(
        [this](const Events::AnimationFrameChangedEvent& event) {
            currentTime_ = event.currentTime;
            isPlaying_ = true; // 假设帧更改意味着播放中
        });

    // 订阅 AnimationDataLoadedEvent，初始化 UI 数据
    dataLoadedSubId_ = eventBus_->Subscribe<Events::AnimationDataLoadedEvent>(
        [this](const Events::AnimationDataLoadedEvent& event) {
            LoadAnimationData(event.keyframes);
        });

    // 订阅 AnimationUpdatedEvent，同步关键帧数据
    updatedSubId_ = eventBus_->Subscribe<Events::AnimationUpdatedEvent>(
        [this](const Events::AnimationUpdatedEvent& event) {
            keyframes_ = event.keyframes;
            // 重新计算最大时间
            if (!keyframes_.empty()) {
                maxTime_ = std::max(maxTime_, keyframes_.rbegin()->first);
            }
        });

    // 订阅 AnimationPlaybackStartedEvent，更新播放状态
    playbackStartedSubId_ = eventBus_->Subscribe<Events::AnimationPlaybackStartedEvent>(
        [this](const Events::AnimationPlaybackStartedEvent& event) {
            isPlaying_ = true;
        });

    // 订阅 AnimationPlaybackStoppedEvent，更新播放状态
    playbackStoppedSubId_ = eventBus_->Subscribe<Events::AnimationPlaybackStoppedEvent>(
        [this](const Events::AnimationPlaybackStoppedEvent& event) {
            isPlaying_ = false;
        });
}

void Animation::Update() {
    ImGui::Begin("Animation");

    // 绘制时间轴
    RenderTimeline();

    // 绘制关键帧编辑器
    RenderKeyframeEditor();

    // 绘制播放控制按钮
    RenderPlaybackControls();

    // 处理时间滑块拖动
    HandleTimeSlider();

    ImGui::End();
}

void Animation::LoadAnimationData(const std::map<float, KeyframeData>& keyframes) {
    keyframes_ = keyframes;
    // 计算最大时间
    if (!keyframes_.empty()) {
        maxTime_ = std::max(maxTime_, keyframes_.rbegin()->first);
    } else {
        maxTime_ = 10.0f; // 默认值
    }
    currentTime_ = 0.0f; // 重置当前时间
}

void Animation::AddKeyframe(float time, const KeyframeData& keyframe) {
    // 创建撤销操作
    Operation op;
    op.execute = [this, time, keyframe]() {
        eventBus_->Publish(Events::KeyframeAddedEvent{time, keyframe});
        PropagateHierarchyUpdate(keyframe.modelUUID,
            glm::translate(glm::mat4(1.0f), keyframe.position) *
            glm::mat4_cast(keyframe.rotation) *
            glm::scale(glm::mat4(1.0f), keyframe.scale));
    };
    op.undo = [this, time]() {
        eventBus_->Publish(Events::KeyframeDeletedEvent{time});
    };
    eventBus_->Publish(Events::PushUndoOperationEvent{op});
    // 发布关键帧添加事件
    eventBus_->Publish(Events::KeyframeAddedEvent{time, keyframe});
    // 更新本地数据（可选，依赖 AnimationManager 更新）
    keyframes_[time] = keyframe;
    maxTime_ = std::max(maxTime_, time);
}

void Animation::RemoveKeyframe(float time) {
    auto it = keyframes_.find(time);
    if (it != keyframes_.end()) {
        KeyframeData oldKeyframe = it->second;

        // 创建撤销操作
        Operation op;
        op.execute = [this, time]() {
            eventBus_->Publish(Events::KeyframeDeletedEvent{time});
        };
        op.undo = [this, time, oldKeyframe]() {
            eventBus_->Publish(Events::KeyframeAddedEvent{time, oldKeyframe});
            PropagateHierarchyUpdate(oldKeyframe.modelUUID,
                glm::translate(glm::mat4(1.0f), oldKeyframe.position) *
                glm::mat4_cast(oldKeyframe.rotation) *
                glm::scale(glm::mat4(1.0f), oldKeyframe.scale));
        };
        // 记录操作到 UndoRedoManager
        eventBus_->Publish(Events::PushUndoOperationEvent{op});
        // 发布关键帧删除事件
        eventBus_->Publish(Events::KeyframeDeletedEvent{time});
        // 更新本地数据（可选，依赖 AnimationManager 更新）
        keyframes_.erase(time);
        if (!keyframes_.empty()) {
            maxTime_ = keyframes_.rbegin()->first;
        } else {
            maxTime_ = 10.0f;
        }
    }
}

void Animation::ModifyKeyframe(float time, const KeyframeData& keyframe) {
    auto it = keyframes_.find(time);
    if (it != keyframes_.end()) {
        KeyframeData oldKeyframe = it->second;

        // 创建撤销操作
        Operation op;
        op.execute = [this, time, keyframe]() {
            eventBus_->Publish(Events::KeyframeModifiedEvent{time, keyframe});
            PropagateHierarchyUpdate(keyframe.modelUUID,
                glm::translate(glm::mat4(1.0f), keyframe.position) *
                glm::mat4_cast(keyframe.rotation) *
                glm::scale(glm::mat4(1.0f), keyframe.scale));
        };
        op.undo = [this, time, oldKeyframe]() {
            eventBus_->Publish(Events::KeyframeModifiedEvent{time, oldKeyframe});
            PropagateHierarchyUpdate(oldKeyframe.modelUUID,
                glm::translate(glm::mat4(1.0f), oldKeyframe.position) *
                glm::mat4_cast(oldKeyframe.rotation) *
                glm::scale(glm::mat4(1.0f), oldKeyframe.scale));
        };
        // 记录操作到 UndoRedoManager
        eventBus_->Publish(Events::PushUndoOperationEvent{op});
        // 发布关键帧修改事件
        eventBus_->Publish(Events::KeyframeModifiedEvent{time, keyframe});
        // 更新本地数据（可选，依赖 AnimationManager 更新）
        keyframes_[time] = keyframe;
    }
}

void Animation::RenderTimeline() {
    ImGui::Text("Timeline");
    ImGui::SliderFloat("Time", &currentTime_, 0.0f, maxTime_, "%.2f s");

    // 绘制关键帧标记
    ImGui::BeginChild("TimelineMarkers", ImVec2(0, 50), true);
    for (const auto& [time, keyframe] : keyframes_) {
        ImGui::SetCursorPosX(time / maxTime_ * ImGui::GetWindowWidth());
        ImGui::Text("|"); // 简单标记，可替换为更复杂的 UI
        if (ImGui::IsItemClicked()) {
            currentTime_ = time; // 点击跳转到关键帧时间
        }
    }
    ImGui::EndChild();
}

void Animation::RenderKeyframeEditor() {
    ImGui::Separator();
    ImGui::Text("Keyframes");

    if (ImGui::Button("Add Keyframe")) {
        KeyframeData newKeyframe;
        newKeyframe.modelUUID = "default_model"; // 示例值，应从 UI 或上下文获取
        newKeyframe.position = glm::vec3(0.0f);
        newKeyframe.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // 单位四元数
        newKeyframe.scale = glm::vec3(1.0f);
        newKeyframe.materialUUID = "default_material"; // 示例值
        AddKeyframe(currentTime_, newKeyframe);
    }

    // 显示和编辑关键帧列表
    for (auto it = keyframes_.begin(); it != keyframes_.end();) {
        float time = it->first;
        KeyframeData& keyframe = it->second;

        ImGui::PushID(time);
        ImGui::Text("Time: %.2f", time);
        ImGui::SameLine();
        if (ImGui::Button("Delete")) {
            RemoveKeyframe(time);
            it = keyframes_.begin(); // 重置迭代器，因为 map 可能改变
            ImGui::PopID();
            continue;
        }

        // 编辑关键帧属性
        bool modified = false;
        if (ImGui::DragFloat3("Position", &keyframe.position[0], 0.1f)) modified = true;
        if (ImGui::DragFloat4("Rotation (quat)", &keyframe.rotation[0], 0.01f)) modified = true;
        if (ImGui::DragFloat3("Scale", &keyframe.scale[0], 0.01f, 0.1f, 10.0f)) modified = true;

        if (modified) {
            ModifyKeyframe(time, keyframe);
        }

        ImGui::PopID();
        ++it;
    }
}

void Animation::RenderPlaybackControls() {
    ImGui::Separator();
    ImGui::Text("Playback Controls");

    if (isPlaying_) {
        if (ImGui::Button("Pause")) {
            eventBus_->Publish(Events::AnimationPlaybackControlEvent{Events::AnimationPlaybackControlEvent::Action::Pause});
        }
    } else {
        if (ImGui::Button("Play")) {
            eventBus_->Publish(Events::AnimationPlaybackControlEvent{Events::AnimationPlaybackControlEvent::Action::Play});
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Stop")) {
        eventBus_->Publish(Events::AnimationPlaybackControlEvent{Events::AnimationPlaybackControlEvent::Action::Stop});
        currentTime_ = 0.0f; // 重置时间
    }
}

void Animation::HandleTimeSlider() {
    static float lastTime = currentTime_;
    if (currentTime_ != lastTime && !isPlaying_) {
        // 用户拖动滑块，发布时间更改请求
        eventBus_->Publish(Events::RequestAnimationFrameChangeEvent{currentTime_});
        lastTime = currentTime_;
    }
}

void Animation::PropagateHierarchyUpdate(const std::string& modelUUID, const glm::mat4& transform) {
    // 发布层级更新事件，通知子模型更新变换
    eventBus_->Publish(Events::HierarchyUpdateEvent{modelUUID, transform});
}


} // namespace MyRenderer
