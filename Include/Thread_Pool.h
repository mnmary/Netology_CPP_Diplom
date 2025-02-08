#pragma once
#include <iostream>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include "Safe_Queue.h"


class Thread_Pool
{
private:
    std::vector<std::thread> arrThread;
    Safe_Queue<std::function <void()> > queue;
    std::mutex mutex;

    bool stop{ false };
   
 
public:
    Thread_Pool()
    {
        int cntPool = std::thread::hardware_concurrency();
        for (int i = 0; i < cntPool; i++)
        {
            arrThread.push_back(std::thread(&Thread_Pool::work, this));
        }
    }
    ~Thread_Pool()
    {
        std::unique_lock<std::mutex> lock(mutex);
        stop = true;
        lock.unlock();
        for (auto& t : arrThread) 
        {
            t.join();
        }
    }
    void work()
    {
        while (!stop) {
            std::function<void()> work;

            std::unique_lock<std::mutex> lock(mutex);
            if (!queue.empty())
            {
                work = queue.pop();
                std::cout << "POP\n";
                work();
            }
            lock.unlock();
        }
    }

    void submit(std::vector<std::function<void()> > arrWork) 
    {
        if (!stop) 
        {
            for (auto& w : arrWork)
            {
                queue.push(w);
                std::cout << "PUSH\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
};