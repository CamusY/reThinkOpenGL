// EventBus.h
#ifndef EVENTBUS_H
#define EVENTBUS_H

#include <map>
#include <vector>
#include <functional>
#include <typeindex>
#include <mutex>
#include <atomic>
#include <iostream>
#include <memory>
#include<algorithm>

/**
 * @brief EventBus 是一个线程安全的事件总线，用于模块间解耦通信。
 * 
 * 它支持泛型事件类型，通过 Subscribe 订阅事件，通过 Publish 发布事件，
 * 并提供 Unsubscribe 方法以取消订阅。使用 std::mutex 确保线程安全，
 * 使用 std::type_index 区分不同事件类型。
 */
class EventBus {
public:
    // 定义订阅者 ID 类型，用于标识和取消订阅
    using SubscriberId = size_t;

    // 优先级枚举定义
    enum class Priority : uint8_t {
        High = 0,    // 最高优先级（如渲染、交互事件）
        Normal = 1,  // 普通优先级（默认）
        Low = 2      // 低优先级（如日志、后台任务）
    };
    
    /**
     * @brief 构造函数，初始化 EventBus。
     */
    EventBus() = default;

    /**
     * @brief 析构函数，确保资源正确释放。
     */
    ~EventBus() = default;

    // 禁止拷贝和赋值，避免意外的资源管理问题
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;

    /**
     * @brief 订阅指定类型的事件。
     * 
     * @tparam EventType 事件类型，支持泛型。
     * @param callback 回调函数，接收事件引用。
     * @return SubscriberId 订阅者 ID，用于后续取消订阅。
     */
    template<typename EventType>
    SubscriberId Subscribe(std::function<void(const EventType&)> callback, 
                          Priority priority = Priority::Normal) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto typeIndex = std::type_index(typeid(EventType));
        auto& subscribers = subscribers_[typeIndex];

        // 生成唯一的订阅者 ID
        SubscriberId id = nextId_++;
        
        // 将回调函数包装为 void(const void*) 类型，并存储
        subscribers.push_back(Subscriber{
            id,
            [callback, typeIndex](const void* event) {
                try {
                    callback(*static_cast<const EventType*>(event));
                } catch (const std::exception& e) {
                    std::cerr << "Error in event callback for type " 
                              << typeIndex.name() << ": " << e.what() << std::endl;
                }
            },
            priority  // 新增优先级字段
        });
        std::sort(subscribers.begin(), subscribers.end(), 
                          [](const Subscriber& a, const Subscriber& b) {
                              return a.priority < b.priority;
                          });
        return id;
    }

    /**
     * @brief 取消订阅指定类型事件的某个订阅者。
     * 
     * @param typeIndex 事件类型的 std::type_index，通常由 typeid(EventType) 生成。
     * @param id 订阅者 ID，由 Subscribe 返回。
     */
    void Unsubscribe(std::type_index typeIndex, SubscriberId id) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = subscribers_.find(typeIndex);
        if (it != subscribers_.end()) {
            auto& subscribers = it->second;
            subscribers.erase(
                std::remove_if(subscribers.begin(), subscribers.end(),
                    [id](const Subscriber& sub) { return sub.id == id; }),
                subscribers.end()
            );
            if (subscribers.empty()) {
                subscribers_.erase(typeIndex);
            }
        }
    }


    /**
     * @brief 发布指定类型的事件，通知所有订阅者。
     * 
     * @tparam EventType 事件类型，支持泛型。
     * @param event 事件实例的引用。
     */
    template<typename EventType>
    void Publish(const EventType& event) {
        std::vector<Subscriber> subscribersCopy;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto typeIndex = std::type_index(typeid(EventType));
            auto it = subscribers_.find(typeIndex);
            if (it != subscribers_.end()) {
                subscribersCopy = it->second; // 复制当前订阅者列表
            }
        }
        // 在锁外执行回调
        for (const auto& subscriber : subscribersCopy) {
            subscriber.callback(&event);
        }
    }

private:
    /**
     * @brief 订阅者结构体，包含 ID 和回调函数。
     */
    struct Subscriber {
        SubscriberId id;                    // 订阅者唯一标识
        std::function<void(const void*)> callback;  // 回调函数，接收事件指针
        Priority priority;
    };

    std::mutex mutex_;  // 互斥锁，确保线程安全
    std::map<std::type_index, std::vector<Subscriber>> subscribers_;  // 事件类型到订阅者列表的映射
    std::atomic<size_t> nextId_{0};  // 原子变量，用于生成唯一的订阅者 ID
};

#endif // EVENTBUS_H