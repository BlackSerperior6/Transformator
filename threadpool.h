#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <functional>
#include <thread>
#include <queue>
#include <mutex>


class ThreadPool
{
public:
    ThreadPool(size_t numThreads = std::thread::hardware_concurrency());

    ~ThreadPool();

    template<typename F>
    void AddTask(F&& task)
    {
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            tasks.emplace(std::forward<F>(task));
        }

        cv.notify_one();
    }

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queueMutex;
    std::condition_variable cv;
    bool stop;
};

#endif // THREADPOOL_H
