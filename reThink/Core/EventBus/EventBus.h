// EventBus.h
#pragma once
#include <functional>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <vector>
#include "EventTypes.h"

/**
 * @brief 线程安全的事件总线系统
 * @note 使用依赖注入设计，通过构造函数初始化必要依赖
 */
class EventBus {
public:
    using Callback = std::function<void(const void*)>;
    
    /**
     * @brief 构造函数注入日志记录器（示例）
     * @param logger 日志记录器接口（实际项目中可替换具体实现）
     */
    explicit EventBus(std::function<void(const std::string&)> logger = nullptr)
        : logger_(logger ? logger : defaultLogger) {}

    // 禁止拷贝和移动
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;

    /**
     * @brief 订阅指定类型事件
     * @tparam EventType 事件类型（来自EventTypes命名空间）
     * @param callback 事件回调函数
     */
    template<typename EventType>
    void Subscribe(Callback callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto typeHash = typeid(EventType).hash_code();
        subscribers_[typeHash].push_back(std::make_shared<Callback>(callback));
        
        if(logger_) {
            logger_("事件类型 " + std::to_string(typeHash) + " 新增订阅者");
        }
    }

    /**
     * @brief 发布事件
     * @tparam EventType 事件类型
     * @param event 事件数据引用
     */
    template<typename EventType>
    void Publish(const EventType& event) {
        const auto typeHash = typeid(EventType).hash_code();
        std::vector<std::shared_ptr<Callback>> callbacks;
        
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = subscribers_.find(typeHash);
            if(it != subscribers_.end()) {
                callbacks = it->second;
            }
        }

        for(auto& cb : callbacks) {
            if(*cb) {
                try {
                    (*cb)(&event);
                } catch(const std::exception& e) {
                    if(logger_) {
                        logger_(std::string("事件回调异常: ") + e.what());
                    }
                }
            }
        }
    }

private:
    std::unordered_map<size_t, std::vector<std::shared_ptr<Callback>>> subscribers_;
    std::mutex mutex_;
    std::function<void(const std::string&)> logger_;
    
    // 默认日志实现
    static void defaultLogger(const std::string& msg) {
        // 生产环境可替换为无操作
        printf("[EventBus] %s\n", msg.c_str());
    }
};