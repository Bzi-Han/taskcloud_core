#ifndef THREAD_POOL_H //! THREAD_POOL_H
#define THREAD_POOL_H

#include <thread>
#include <memory>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <future>
#include <type_traits>

class ThreadPool
{
public:
    ThreadPool(size_t threadPoolSize = std::thread::hardware_concurrency() + 3)
    {
        for (uint32_t i = 0; i < threadPoolSize; i++)
        {
            m_workers.emplace_back(
                [this]
                {
                    while (m_started)
                    {
                        std::function<void()> task(nullptr);

                        if (!m_tasks.empty())
                            m_takeCondition.notify_one();
                        {
                            std::unique_lock<std::mutex> locker(m_takeMutex);
                            m_takeCondition.wait(locker, [this]
                                                 { return !m_tasks.empty(); });
                            task = std::move(m_tasks.front());
                            m_tasks.pop_front();
                        }

                        if (nullptr != task)
                            task();
                    }
                });
        }
    }
    ~ThreadPool()
    {
        m_started = false;
        for (size_t i = m_tasks.size(); i < m_workers.size(); i++)
            m_tasks.emplace_back([] {});
        m_takeCondition.notify_all();
        for (auto &v : m_workers)
            if (v.joinable())
                v.join();
    }

    /**
     * @name addRunable
     * @brief 添加执行体到线程池中
     *
     * @param runable 需要执行的函数或方法
     * @param params 需要传递给执行函数或方法的参数
     *
     * @return 获取执行体返回值的std::future<result_t>对象
     */
    template <typename runable_t, typename... param_t>
    constexpr auto addRunable(const runable_t &runable, param_t &&...params)
    {
#ifndef _MSC_VER //!_MSC_VER
#if __cplusplus >= 201703L
        using result_t = typename std::invoke_result<runable_t &, param_t...>::type;
#else
        using result_t = typename std::result_of<runable_t &(param_t...)>::type;
#endif
#else
#if _MSVC_LANG >= 201703L
        using result_t = typename std::invoke_result<runable_t &, param_t...>::type;
#else
        using result_t = typename std::result_of<runable_t &(param_t...)>::type;
#endif
#endif //!_MSC_VER
        auto task = std::make_shared<std::packaged_task<result_t()>>(
            std::bind(runable, std::forward<param_t>(params)...));

        auto result = task->get_future();
        {
            std::unique_lock<std::mutex> locker(m_takeMutex);

            m_tasks.emplace_back([task]
                                 { (*task)(); });
        }
        m_takeCondition.notify_one();

        return result;
    }

    /**
     * @name addRunableNoWrap
     * @brief 添加无参无返回值执行体到线程池中，不进行包装
     *
     * @param runable 需要执行的函数或方法
     *
     * @return void
     */
    template <typename runable_t>
    constexpr void addRunableNoWrap(const runable_t &runable)
    {
        {
            std::unique_lock<std::mutex> locker(m_takeMutex);

            m_tasks.emplace_back(std::move(runable));
        }
        m_takeCondition.notify_one();
    }

private:
    std::vector<std::thread> m_workers;
    std::deque<std::function<void()>> m_tasks;
    bool m_started = true;
    std::mutex m_takeMutex;
    std::condition_variable m_takeCondition;
};

#endif //! THREAD_POOL_H