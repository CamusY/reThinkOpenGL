#include "UndoRedoManager.h"
#include <stdexcept>
#include <iostream>

namespace MyRenderer {

UndoRedoManager::UndoRedoManager(std::shared_ptr<EventBus> eventBus)
    : eventBus_(std::move(eventBus)) {
    // 检查事件总线是否有效
    if (!eventBus_) {
        throw std::invalid_argument("UndoRedoManager: EventBus cannot be null");
    }

    // 订阅 PushUndoOperationEvent，接收其他模块推送的操作
    pushSubId_ = eventBus_->Subscribe<Events::PushUndoOperationEvent>(
        [this](const Events::PushUndoOperationEvent& event) {
            OnPushUndoOperation(event);
        }
    );

    // 订阅 UndoRedoEvent，响应用户触发的撤销或重做请求（例如 Ctrl+Z / Ctrl+Y）
    undoRedoSubId_ = eventBus_->Subscribe<Events::UndoRedoEvent>(
        [this](const Events::UndoRedoEvent& event) {
            OnUndoRedo(event);
        }
    );
}

UndoRedoManager::~UndoRedoManager() {
    // 清理资源，取消事件订阅以避免悬垂引用
    eventBus_->Unsubscribe(std::type_index(typeid(Events::PushUndoOperationEvent)), pushSubId_);
    eventBus_->Unsubscribe(std::type_index(typeid(Events::UndoRedoEvent)), undoRedoSubId_);
}

void UndoRedoManager::PushOperation(const Operation& op) {
    std::lock_guard<std::mutex> lock(stackMutex_); // 锁定互斥锁，确保线程安全

    // 检查操作的有效性
    if (!op.execute || !op.undo) {
        std::cerr << "[UndoRedoManager] Warning: Invalid operation with null function pointers" << std::endl;
        return; // 无效操作直接返回，不入栈
    }

    // 将操作推入撤销栈
    undoStack_.push(op);

    // 清空重做栈，因为新操作使之前的重做记录无效
    while (!redoStack_.empty()) {
        redoStack_.pop();
    }
}

void UndoRedoManager::Undo() {
    std::lock_guard<std::mutex> lock(stackMutex_); // 锁定互斥锁，确保线程安全

    // 检查撤销栈是否为空
    if (undoStack_.empty()) {
        std::cout << "[UndoRedoManager] Nothing to undo" << std::endl;
        return; // 无操作可撤销，直接返回
    }

    // 从撤销栈顶部取出一个操作
    Operation op = std::move(undoStack_.top());
    undoStack_.pop();

    try {
        // 执行撤销函数，恢复到操作之前的状态
        op.undo();
        // 将操作移入重做栈，以便后续重做
        redoStack_.push(std::move(op));
    } catch (const std::exception& e) {
        // 捕获异常，输出错误信息，避免程序崩溃
        std::cerr << "[UndoRedoManager] Error during undo: " << e.what() << std::endl;
    }
}

void UndoRedoManager::Redo() {
    std::lock_guard<std::mutex> lock(stackMutex_); // 锁定互斥锁，确保线程安全

    // 检查重做栈是否为空
    if (redoStack_.empty()) {
        std::cout << "[UndoRedoManager] Nothing to redo" << std::endl;
        return; // 无操作可重做，直接返回
    }

    // 从重做栈顶部取出一个操作
    Operation op = std::move(redoStack_.top());
    redoStack_.pop();

    try {
        // 执行操作的 execute 函数，恢复到操作之后的状态
        op.execute();
        // 将操作移回撤销栈，以便后续撤销
        undoStack_.push(std::move(op));
    } catch (const std::exception& e) {
        // 捕获异常，输出错误信息，避免程序崩溃
        std::cerr << "[UndoRedoManager] Error during redo: " << e.what() << std::endl;
    }
}

size_t UndoRedoManager::GetUndoStackSize() const {
    std::lock_guard<std::mutex> lock(stackMutex_); // 锁定互斥锁，确保线程安全
    return undoStack_.size(); // 返回撤销栈大小
}

size_t UndoRedoManager::GetRedoStackSize() const {
    std::lock_guard<std::mutex> lock(stackMutex_); // 锁定互斥锁，确保线程安全
    return redoStack_.size(); // 返回重做栈大小
}

void UndoRedoManager::OnPushUndoOperation(const Events::PushUndoOperationEvent& event) {
    // 处理 PushUndoOperationEvent，将操作推入撤销栈
    PushOperation(event.op);
}

void UndoRedoManager::OnUndoRedo(const Events::UndoRedoEvent& event) {
    // 根据事件的操作类型执行撤销或重做
    switch (event.action) {
        case Events::UndoRedoEvent::Action::Undo:
            Undo();
            break;
        case Events::UndoRedoEvent::Action::Redo:
            Redo();
            break;
        default:
            // 处理未知操作类型，输出警告
            std::cerr << "[UndoRedoManager] Unknown UndoRedoEvent action" << std::endl;
            break;
    }
}

} // namespace MyRenderer