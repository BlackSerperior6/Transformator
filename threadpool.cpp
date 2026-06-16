#include "threadpool.h"

ThreadPool::ThreadPool(size_t numThreads) : stop(false)
{
    for (size_t i = 0; i < numThreads; ++i)
    {
        workers.emplace_back([this]
        {
            while (true)
            {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(queueMutex);

                    cv.wait(lock, [this]
                    {
                        return stop || !tasks.empty();
                    });

                    if (stop && tasks.empty())
                        return;

                    task = std::move(tasks.front());
                    tasks.pop();
                }

                task();
            }
        });
    }
}

ThreadPool::~ThreadPool()
{
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        stop = true;
    }

    cv.notify_all();

    for (auto& worker : workers)
    {
        if (worker.joinable())
            worker.join();
    }
}

template<typename F, typename... Args>
void ThreadPool::AddTask(F&& f, Args&&... args)
{
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        tasks.emplace([=]()
        {
            std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
        });
    }

    cv.notify_one();
}
