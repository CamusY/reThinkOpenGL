// ThreadPool.h
#pragma once
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <future>

/**
 * @brief 线程安全的任务线程池
 * @note 通过依赖注入传递到需要使用的模块
 */
class ThreadPool {
public:
    /**
     * @brief 构造函数注入必要配置
     * @param threadCount 线程数量（默认硬件并发数）
     * @param onError 错误处理回调（可选）
     */
    explicit ThreadPool(
        unsigned int threadCount = std::thread::hardware_concurrency(),
        std::function<void(const std::exception&)> onError = nullptr
    ) : stop_(false), errorHandler_(onError) {
        // 防御性检查
        if(threadCount == 0) throw std::invalid_argument("线程数不能为零");
        
        for(unsigned int i = 0; i < threadCount; ++i) {
            workers_.emplace_back([this] { workerLoop(); });
        }
    }

    ~ThreadPool() {
        stop();
    }

    /**
     * @brief 提交任务到线程池
     * @tparam F 可调用对象类型
     * @param task 要执行的任务
     */
    template<typename F>
    auto Enqueue(F&& task) -> std::future<decltype(task())> {
        using return_type = decltype(task());
    
        auto task_ptr = std::make_shared<std::packaged_task<return_type()>>(
            std::forward<F>(task)
        );
    
        std::future<return_type> res = task_ptr->get_future();
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            if(stop_) throw std::runtime_error("线程池已停止");
            tasks_.emplace([task_ptr](){ (*task_ptr)(); });
        }
        condition_.notify_one();
        return res;
    }

    /**
     * @brief 停止线程池（等待所有任务完成）
     */
    void stop() noexcept {
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            if(stop_) return;
            stop_ = true;
        }
        condition_.notify_all();
        for(std::thread& worker : workers_) {
            if(worker.joinable()) worker.join();
        }
    }

private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queueMutex_;
    std::condition_variable condition_;
    std::atomic<bool> stop_;
    std::function<void(const std::exception&)> errorHandler_;

    void workerLoop() {
        while(true) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(queueMutex_);
                condition_.wait(lock, [this] {
                    return stop_ || !tasks_.empty();
                });

                if(stop_ && tasks_.empty()) return;

                task = std::move(tasks_.front());
                tasks_.pop();
            }

            try {
                task();
            } catch(const std::exception& e) {
                if(errorHandler_) {
                    errorHandler_(e);
                } else {
                    // 默认错误处理
                    fprintf(stderr, "线程池任务异常: %s\n", e.what());
                }
            }
        }
    }
};