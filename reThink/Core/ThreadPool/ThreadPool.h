// ThreadPool.h
#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <functional>
#include <queue>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <stdexcept>

// ThreadPool 模块，提供异步任务执行服务
class ThreadPool {
public:
    // 构造函数，初始化线程池，参数为线程数，默认为硬件并发线程数
    explicit ThreadPool(size_t threadCount = std::thread::hardware_concurrency());
    
    // 析构函数，停止线程池并清理资源
    ~ThreadPool();

    // 将任务加入队列，支持优先级，默认优先级为 0（最低）
    void EnqueueTask(std::function<void()> task, int priority = 0);

    // 等待所有任务完成
    void WaitAll();

    // 设置错误回调函数，用于任务执行失败时通知调用方
    void SetErrorCallback(std::function<void(const std::string&)> callback);

    // 获取当前任务队列中的任务数（仅用于调试或状态检查）
    size_t GetPendingTaskCount() const;

private:
    // 任务结构体，包含任务函数和优先级
    struct Task {
        std::function<void()> taskFunction;
        int priority;

        Task(std::function<void()> func, int prio) : taskFunction(std::move(func)), priority(prio) {}

        // 用于优先级队列比较，高优先级任务先执行
        bool operator<(const Task& other) const {
            return priority < other.priority;
        }
    };

    // 工作线程函数
    void WorkerThread();

    // 线程池中的线程集合
    std::vector<std::thread> workers;

    // 任务队列，使用优先级队列管理
    std::priority_queue<Task> tasks;

    // 互斥锁，保护任务队列
    mutable std::mutex queueMutex;

    // 条件变量，用于线程同步
    std::condition_variable condition;

    // 停止标志，控制线程池退出
    std::atomic<bool> stop;

    // 当前正在执行的任务数
    std::atomic<size_t> activeTasks;

    // 错误回调函数
    std::function<void(const std::string&)> errorCallback;
};

#endif // THREADPOOL_H