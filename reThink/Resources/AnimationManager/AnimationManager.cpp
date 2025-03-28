#include "AnimationManager.h"
#include <GLFW/glfw3.h> // 用于 glfwGetTime()
#include <algorithm>
#include <stdexcept>
#include <glm/gtc/matrix_transform.hpp>

namespace MyRenderer {

AnimationManager::AnimationManager(std::shared_ptr<JSONSerializer> jsonSerializer,
                                 std::shared_ptr<EventBus> eventBus)
    : jsonSerializer_(std::move(jsonSerializer)),
      eventBus_(std::move(eventBus)) {
    if (!jsonSerializer_ || !eventBus_) {
        throw std::invalid_argument("AnimationManager: JSONSerializer and EventBus must not be null");
    }
    SetupEventSubscriptions();
}

AnimationManager::~AnimationManager() {}

void AnimationManager::SetupEventSubscriptions() {
    // 订阅 AnimationPlaybackControlEvent
    eventBus_->Subscribe<Events::AnimationPlaybackControlEvent>(
        [this](const Events::AnimationPlaybackControlEvent& event) {
            switch (event.action) {
                case Events::AnimationPlaybackControlEvent::Action::Play:
                    PlayAnimation();
                    break;
                case Events::AnimationPlaybackControlEvent::Action::Pause:
                    PauseAnimation();
                    break;
                case Events::AnimationPlaybackControlEvent::Action::Stop:
                    PauseAnimation();
                    currentTime_ = 0.0f; // 重置时间
                    eventBus_->Publish(Events::AnimationFrameChangedEvent{currentTime_});
                    break;
            }
        });

    // 订阅 KeyframeAddedEvent
    eventBus_->Subscribe<Events::KeyframeAddedEvent>(
        [this](const Events::KeyframeAddedEvent& event) {
            AddKeyframe(event.time, event.keyframe);
        });

    // 订阅 KeyframeModifiedEvent
    eventBus_->Subscribe<Events::KeyframeModifiedEvent>(
        [this](const Events::KeyframeModifiedEvent& event) {
            ModifyKeyframe(event.time, event.keyframe);
        });

    // 订阅 KeyframeDeletedEvent
    eventBus_->Subscribe<Events::KeyframeDeletedEvent>(
        [this](const Events::KeyframeDeletedEvent& event) {
            RemoveKeyframe(event.time);
        });

    // 订阅 RequestAnimationDataLoadEvent
    eventBus_->Subscribe<Events::RequestAnimationDataLoadEvent>(
        [this](const Events::RequestAnimationDataLoadEvent& event) {
            if (LoadAnimationData(event.animationDataPath)) {
                eventBus_->Publish(Events::AnimationDataLoadedEvent{event.animationDataPath});
            }
        });

    // 订阅 RequestAnimationDataSaveEvent
    eventBus_->Subscribe<Events::RequestAnimationDataSaveEvent>(
        [this](const Events::RequestAnimationDataSaveEvent& event) {
            SaveAnimationData(event.animationDataPath);
        });

    // 订阅 RequestAnimationFrameChangeEvent
    eventBus_->Subscribe<Events::RequestAnimationFrameChangeEvent>(
        [this](const Events::RequestAnimationFrameChangeEvent& event) {
            currentTime_ = event.newTime;
            eventBus_->Publish(Events::AnimationFrameChangedEvent{currentTime_});
        });

    // 新增：监听 ControlPanel 的变换请求，动画播放时忽略
    eventBus_->Subscribe<Events::ModelTransformedEvent>(
        [this](const Events::ModelTransformedEvent& event) {
            if (isPlaying_) {
                // 动画播放时，AnimationManager 是权威源，恢复关键帧变换
                glm::mat4 correctTransform = GetModelTransformAtTime(currentTime_);
                eventBus_->Publish(Events::ModelTransformedEvent{event.modelUUID, correctTransform});
            }
        });
}

bool AnimationManager::LoadAnimationData(const std::filesystem::path& projectFilePath) {
    try {
        nlohmann::json j = JSONSerializer::DeserializeFromFile<nlohmann::json>(projectFilePath);
        DeserializeKeyframes(j);
        eventBus_->Publish(Events::AnimationUpdatedEvent{keyframes_});
        return true;
    } catch (const JsonSerializationException&) {
        // 这里可以添加日志或错误处理，暂不实现
        return false;
    }
}

bool AnimationManager::SaveAnimationData(const std::filesystem::path& projectFilePath) {
    try {
        nlohmann::json j;
        SerializeKeyframes(j);
        JSONSerializer::SerializeToFile(j, projectFilePath);
        return true;
    } catch (const JsonSerializationException&) {
        // 这里可以添加日志或错误处理，暂不实现
        return false;
    }
}

void AnimationManager::AddKeyframe(float time, const KeyframeData& keyframeData) {
    Operation op;
    op.execute = [this, time, keyframeData]() {
        keyframes_[time] = keyframeData;
        eventBus_->Publish(Events::AnimationUpdatedEvent{keyframes_});
    };
    op.undo = [this, time]() {
        keyframes_.erase(time);
        eventBus_->Publish(Events::AnimationUpdatedEvent{keyframes_});
    };
    eventBus_->Publish(Events::PushUndoOperationEvent{op});

    keyframes_[time] = keyframeData;
    eventBus_->Publish(Events::AnimationUpdatedEvent{keyframes_});
}

void AnimationManager::RemoveKeyframe(float time) {
    auto it = keyframes_.find(time);
    if (it != keyframes_.end()) {
        KeyframeData oldKeyframe = it->second;
        Operation op;
        op.execute = [this, time]() {
            keyframes_.erase(time);
            eventBus_->Publish(Events::AnimationUpdatedEvent{keyframes_});
        };
        op.undo = [this, time, oldKeyframe]() {
            keyframes_[time] = oldKeyframe;
            eventBus_->Publish(Events::AnimationUpdatedEvent{keyframes_});
        };
        eventBus_->Publish(Events::PushUndoOperationEvent{op});

        keyframes_.erase(time);
        eventBus_->Publish(Events::AnimationUpdatedEvent{keyframes_});
    }
}

void AnimationManager::ModifyKeyframe(float time, const KeyframeData& keyframeData) {
    auto it = keyframes_.find(time);
    if (it != keyframes_.end()) {
        KeyframeData oldKeyframe = it->second;
        Operation op;
        op.execute = [this, time, keyframeData]() {
            keyframes_[time] = keyframeData;
            eventBus_->Publish(Events::AnimationUpdatedEvent{keyframes_});
        };
        op.undo = [this, time, oldKeyframe]() {
            keyframes_[time] = oldKeyframe;
            eventBus_->Publish(Events::AnimationUpdatedEvent{keyframes_});
        };
        eventBus_->Publish(Events::PushUndoOperationEvent{op});

        keyframes_[time] = keyframeData;
        eventBus_->Publish(Events::AnimationUpdatedEvent{keyframes_});
    }
}

const std::map<float, KeyframeData>& AnimationManager::GetKeyframes() const {
    return keyframes_;
}

glm::mat4 AnimationManager::GetModelTransformAtTime(float time) {
    if (keyframes_.empty()) {
        return glm::mat4(1.0f); // 返回单位矩阵
    }

    auto nextIt = keyframes_.lower_bound(time);
    if (nextIt == keyframes_.begin()) {
        return glm::translate(glm::mat4(1.0f), nextIt->second.position) *
               glm::mat4_cast(nextIt->second.rotation) *
               glm::scale(glm::mat4(1.0f), nextIt->second.scale);
    }
    if (nextIt == keyframes_.end()) {
        auto last = std::prev(keyframes_.end());
        return glm::translate(glm::mat4(1.0f), last->second.position) *
               glm::mat4_cast(last->second.rotation) *
               glm::scale(glm::mat4(1.0f), last->second.scale);
    }

    auto prevIt = std::prev(nextIt);
    return InterpolateKeyframes(time, prevIt->first, prevIt->second, nextIt->first, nextIt->second);
}

void AnimationManager::SetInterpolationMethod(InterpolationMethod method) {
    interpMethod_ = method;
}

void AnimationManager::PlayAnimation() {
    if (!isPlaying_) {
        isPlaying_ = true;
        startTime_ = glfwGetTime() - currentTime_; // 调整开始时间以继续播放
        eventBus_->Publish(Events::AnimationPlaybackStartedEvent{});
    }
}

void AnimationManager::PauseAnimation() {
    if (isPlaying_) {
        isPlaying_ = false;
        eventBus_->Publish(Events::AnimationPlaybackStoppedEvent{});
    }
}

void AnimationManager::Update(float deltaTime) {
    if (isPlaying_) {
        // 使用独立的时间计算逻辑，避免依赖外部deltaTime
        currentTime_ = static_cast<float>(glfwGetTime() - startTime_);
        // 发布时间更新事件
        eventBus_->Publish(Events::AnimationFrameChangedEvent{currentTime_});
    }
}

glm::mat4 AnimationManager::InterpolateKeyframes(float time, float prevTime, const KeyframeData& prev, float nextTime, const KeyframeData& next) const {
    float t = (time - prevTime) / (nextTime - prevTime); // 归一化时间
    if (interpMethod_ == InterpolationMethod::Linear) {
        glm::vec3 interpolatedPos = glm::mix(prev.position, next.position, t);
        glm::quat interpolatedRot = glm::slerp(prev.rotation, next.rotation, t);
        glm::vec3 interpolatedScale = glm::mix(prev.scale, next.scale, t);

        return glm::translate(glm::mat4(1.0f), interpolatedPos) *
               glm::mat4_cast(interpolatedRot) *
               glm::scale(glm::mat4(1.0f), interpolatedScale);
    } else { // Spline（暂未实现，留作扩展）
        // 未来可实现样条插值
        return glm::mat4(1.0f);
    }
}

void AnimationManager::SerializeKeyframes(nlohmann::json& j) const {
    j["keyframes"] = nlohmann::json::object();
    for (const auto& [time, keyframe] : keyframes_) {
        nlohmann::json kf;
        kf["modelUUID"] = keyframe.modelUUID;
        kf["position"] = {keyframe.position.x, keyframe.position.y, keyframe.position.z};
        kf["rotation"] = {keyframe.rotation.w, keyframe.rotation.x, keyframe.rotation.y, keyframe.rotation.z};
        kf["scale"] = {keyframe.scale.x, keyframe.scale.y, keyframe.scale.z};
        kf["materialUUID"] = keyframe.materialUUID;
        j["keyframes"][std::to_string(time)] = kf;
    }
}

void AnimationManager::DeserializeKeyframes(const nlohmann::json& j) {
    keyframes_.clear();
    if (j.contains("keyframes") && j["keyframes"].is_object()) {
        for (auto& [timeStr, kf] : j["keyframes"].items()) {
            float time = std::stof(timeStr);
            KeyframeData keyframe;
            keyframe.modelUUID = kf["modelUUID"].get<std::string>();
            keyframe.position = glm::vec3(kf["position"][0], kf["position"][1], kf["position"][2]);
            keyframe.rotation = glm::quat(kf["rotation"][0], kf["rotation"][1], kf["rotation"][2], kf["rotation"][3]);
            keyframe.scale = glm::vec3(kf["scale"][0], kf["scale"][1], kf["scale"][2]);
            keyframe.materialUUID = kf["materialUUID"].get<std::string>();
            keyframes_[time] = keyframe;
        }
    }
}

} // namespace MyRenderer
