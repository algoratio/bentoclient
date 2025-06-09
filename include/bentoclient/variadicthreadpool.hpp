#pragma once
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/post.hpp>
#include <future>
#include <functional>

namespace bentoclient
{
    /// @brief A thread pool for variable number of parametes submission of jobs
    class VariadicThreadPool
    {
    public:
        /// @brief Creates a variadic pool
        /// @param nThreads Number of internal threads 
        VariadicThreadPool(std::uint64_t nThreads) : m_pool(nThreads) {}
        VariadicThreadPool(const VariadicThreadPool&) = delete;
        VariadicThreadPool& operator = (const VariadicThreadPool&) = delete;
        VariadicThreadPool(VariadicThreadPool&&) = default;
        VariadicThreadPool& operator = (VariadicThreadPool&&) = default;

        ~VariadicThreadPool() 
        {
            m_pool.join();
        }

        void join() {
            m_pool.join();
        }

        /// @brief Variable number of parameters submission of jobs
        /// @tparam Func Functor type to run asynchronously
        /// @tparam ...Args Argument types to submit to the functor
        /// @param func Functor to run asynchronously
        /// @param ...args Variable arguments list
        /// @return Future for the result returned from functor
        template <typename Func, typename... Args>
        auto post(Func&& func, Args&&... args) -> std::future<std::invoke_result_t<Func, Args...>> {
            using ReturnType = std::invoke_result_t<Func, Args...>;

            auto task = std::make_shared<std::packaged_task<ReturnType()>>(
                std::bind(std::forward<Func>(func), std::forward<Args>(args)...)
            );

            std::future<ReturnType> result = task->get_future();

            boost::asio::post(m_pool, [task]() { (*task)(); });

            return result;
        }

        /// @brief Variable number of parameters submission of jobs where no future for results is wanted / needed
        /// @tparam Func Functor type to run
        /// @tparam ...Args Argument types
        /// @param func Functor to run
        /// @param ...args Variable arguments list
        template <typename Func, typename... Args>
        void postNoFuture(Func&& func, Args&&... args) {
            // Wrap the task with std::bind and post it to the thread pool
            boost::asio::post(m_pool, std::bind(std::forward<Func>(func), std::forward<Args>(args)...));
        }
    
    private:
        boost::asio::thread_pool m_pool;
    };
}