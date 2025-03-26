// ThreadPool.cpp
#include "ThreadPool.h"
#include <stdexcept>
#include <sstream>

ThreadPool::ThreadPool(size_t threadCount) 
    : stop(false), activeTasks(0) {
    if (threadCount == 0) {
        throw std::invalid_argument("ThreadPool: threadCount must be greater than 0");
    }

    // 初始化工作线程
    for (size_t i = 0; i < threadCount; ++i) {
        workers.emplace_back(&ThreadPool::WorkerThread, this);
    }
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        stop = true;
    }
    
    // 通知所有线程退出
    condition.notify_all();

    // 等待所有线程结束
    for (auto& worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void ThreadPool::EnqueueTask(std::function<void()> task, int priority) {
    if (!task) {
        throw std::invalid_argument("ThreadPool: Cannot enqueue null task");
    }

    {
        std::unique_lock<std::mutex> lock(queueMutex);
        if (stop) {
            throw std::runtime_error("ThreadPool: Cannot enqueue task after shutdown");
        }

        // 将任务加入优先级队列
        tasks.emplace(std::move(task), priority);
    }

    // 通知一个等待的工作线程
    condition.notify_one();
}

void ThreadPool::WaitAll() {
    // 等待所有任务完成
    while (true) {
        std::unique_lock<std::mutex> lock(queueMutex);
        if (tasks.empty() && activeTasks == 0) {
            break;
        }
        lock.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 短暂休眠以减少忙等待
    }
}

void ThreadPool::SetErrorCallback(std::function<void(const std::string&)> callback) {
    std::lock_guard<std::mutex> lock(queueMutex);
    errorCallback = std::move(callback);
}

size_t ThreadPool::GetPendingTaskCount() const {
    std::lock_guard<std::mutex> lock(queueMutex);
    return tasks.size() + activeTasks;
}

void ThreadPool::WorkerThread() {
    while (true) {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(queueMutex);

            // 等待任务或停止信号
            condition.wait(lock, [this] { 
                return stop || !tasks.empty(); 
            });

            // 如果停止且任务队列为空，退出线程
            if (stop && tasks.empty()) {
                return;
            }

            // 从队列中取出一个任务
            task = std::move(tasks.top().taskFunction);
            tasks.pop();
            activeTasks++;
        }

        // 执行任务
        try {
            task();
        } catch (const std::exception& e) {
            // 捕获任务执行中的异常，调用错误回调
            std::string errorMsg = std::string("ThreadPool task failed: ") + e.what();
            std::lock_guard<std::mutex> lock(queueMutex);
            if (errorCallback) {
                errorCallback(errorMsg);
            }
        } catch (...) {
            // 捕获未知异常
            std::string errorMsg = "ThreadPool task failed with unknown exception";
            std::lock_guard<std::mutex> lock(queueMutex);
            if (errorCallback) {
                errorCallback(errorMsg);
            }
        }

        // 任务执行完成，减少活跃任务计数
        activeTasks--;
        
        // 通知可能在 WaitAll 中等待的线程
        condition.notify_all();
    }
}