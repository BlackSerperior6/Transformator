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
