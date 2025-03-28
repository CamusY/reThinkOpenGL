#ifndef UNDO_REDO_MANAGER_H
#define UNDO_REDO_MANAGER_H

#include <stack>
#include <functional>
#include <mutex>
#include "EventBus/EventBus.h"  // 事件总线头文件
#include "EventBus/EventTypes.h" // 事件类型定义头文件

namespace MyRenderer {

/**
 * @brief UndoRedoManager 类，负责管理撤销和重做操作。
 * 
 * 该类通过命令模式（Operation 结构体）记录用户操作，维护撤销栈和重做栈。
 * 通过事件总线订阅 `PushUndoOperationEvent` 来接收操作，订阅 `UndoRedoEvent` 来执行撤销或重做。
 * 支持线程安全操作，使用互斥锁保护栈数据。
 */
class UndoRedoManager {
public:
    /**
     * @brief 构造函数，初始化 UndoRedoManager 并订阅相关事件。
     * 
     * @param eventBus 事件总线实例的智能指针，用于事件通信，必须非空。
     * @throws std::invalid_argument 如果 eventBus 为空。
     */
    explicit UndoRedoManager(std::shared_ptr<EventBus> eventBus);

    /**
     * @brief 析构函数，清理资源并取消事件订阅。
     */
    ~UndoRedoManager();

    // 禁止拷贝和赋值，避免资源管理和事件订阅的重复问题
    UndoRedoManager(const UndoRedoManager&) = delete;
    UndoRedoManager& operator=(const UndoRedoManager&) = delete;

    /**
     * @brief 将操作推入撤销栈。
     * 
     * 检查操作的有效性（execute 和 undo 函数非空），若有效则推入撤销栈并清空重做栈。
     * 该方法应该是线程安全的吧，我用互斥锁保护了栈操作，理论上应该就是安全的。
     * 
     * @param op 操作对象，包含 execute 和 undo 函数。
     */
    void PushOperation(const Operation& op);

    /**
     * @brief 执行撤销操作。
     * 
     * 从撤销栈顶部弹出一个操作，执行其 undo 函数，并将操作移入重做栈。
     * 若撤销栈为空，则输出提示信息，不执行任何操作。
     * 该方法肯定是线程安全的，使用互斥锁保护栈操作。
     */
    void Undo();

    /**
     * @brief 执行重做操作。
     * 
     * 从重做栈顶部弹出一个操作，执行其 execute 函数，并将操作移回撤销栈。
     * 若重做栈为空，则输出提示信息，不执行任何操作。
     * 包线程安全的
     */
    void Redo();

    /**
     * @brief 获取当前撤销栈中的操作数量。
     * 
     * 该方法是线程安全的，使用互斥锁保护栈访问。
     * 可在 const 上下文中调用，用于调试或 UI 显示。
     * 
     * @return size_t 撤销栈中的操作数量。
     */
    size_t GetUndoStackSize() const;

    /**
     * @brief 获取当前重做栈中的操作数量。
     * 
     * 线程安全。
     * 可在 const 上下文中调用，用于调试或 UI 显示。
     * 
     * @return size_t 重做栈中的操作数量。
     */
    size_t GetRedoStackSize() const;

private:
    /**
     * @brief 处理 PushUndoOperationEvent 事件。
     * 
     * 将事件中的操作对象推入撤销栈，调用 PushOperation 方法。
     * 该方法作为事件回调，由事件总线在订阅时调用。
     * 
     * @param event PushUndoOperationEvent 事件实例，包含操作对象。
     */
    void OnPushUndoOperation(const Events::PushUndoOperationEvent& event);

    /**
     * @brief 处理 UndoRedoEvent 事件。
     * 
     * 根据事件的操作类型（Undo 或 Redo），调用 Undo 或 Redo 方法。
     * 该方法作为事件回调，由事件总线在订阅时调用。
     * 
     * @param event UndoRedoEvent 事件实例，包含操作类型。
     */
    void OnUndoRedo(const Events::UndoRedoEvent& event);

    std::shared_ptr<EventBus> eventBus_;  // 事件总线实例，用于订阅和发布事件
    std::stack<Operation> undoStack_;     // 撤销栈，存储可撤销的操作
    std::stack<Operation> redoStack_;     // 重做栈，存储可重做的操作
    mutable std::mutex stackMutex_;       // 互斥锁，确保栈操作的线程安全，mutable 允许在 const 方法中使用
    EventBus::SubscriberId pushSubId_;    // PushUndoOperationEvent 的订阅者 ID，用于取消订阅
    EventBus::SubscriberId undoRedoSubId_; // UndoRedoEvent 的订阅者 ID，用于取消订阅
};

} // namespace MyRenderer

#endif // UNDO_REDO_MANAGER_H