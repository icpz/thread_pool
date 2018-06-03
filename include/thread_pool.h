/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __THREAD_POOL_H__
#define __THREAD_POOL_H__

#include <vector>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>
#include <queue>
#include <stdexcept>
#include <future>

class ThreadPool {
public:
    ThreadPool(size_t pool_size);
    ~ThreadPool();

    template<class F, class... Args>
    auto PushTask(F&& f, Args&& ...args)
        -> std::future<std::result_of_t<F(Args...)>>;

private:
    void WorkerLoop();

    std::vector<std::thread> pool_;
    std::queue<std::function<void(void)>> tasks_;

    std::mutex queue_mutex_;
    std::condition_variable cv_;
    bool stop_;
};

inline ThreadPool::ThreadPool(size_t pool_size)
    :stop_(false) {
        for (size_t i = 0; i < pool_size; ++i) {
            pool_.emplace_back(std::bind(&ThreadPool::WorkerLoop, this));
        }
    }

inline ThreadPool::~ThreadPool() {
    {
        std::lock_guard<std::mutex> _(queue_mutex_);
        stop_ = true;
    }
    cv_.notify_all();
    for (auto &worker : pool_) {
        worker.join();
    }
}

template<class F, class... Args>
auto ThreadPool::PushTask(F&& f, Args&& ...args)
    -> std::future<std::result_of_t<F(Args...)>> {
        using result_type = std::result_of_t<F(Args...)>;
        auto task = \
            std::make_shared<std::packaged_task<result_type(void)>>(
                std::bind(std::forward<F>(f), std::forward<Args>(args)...)
            );

        auto result = task->get_future();
        {
            std::lock_guard<std::mutex> _(queue_mutex_);
            if (stop_) {
                throw std::runtime_error("Push task to a stopped pool");
            }
            tasks_.emplace([task{std::move(task)}]() { (*task)(); });
        }
        cv_.notify_one();
        return result;
    }

void ThreadPool::WorkerLoop() {
    std::function<void(void)> task;
    while (true) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            cv_.wait(lock, [this]() { return (stop_ || !tasks_.empty()); });
            if (stop_ && tasks_.empty()) {
                return;
            }
            task = std::move(tasks_.front());
            tasks_.pop();
        }
        task();
    }
}

#endif // __THREAD_POOL_H__
