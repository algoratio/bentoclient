#pragma once
#include <memory>
#include <cstdint>
#include <functional>
#include <string>
#include <map>

namespace bentoclient
{
    /// @brief Thread pool utility class for simplified task submission and progress checks
    class ThreadPool
    {
        class Impl;
    public:
        /// @brief Unique identifier for a submitted job
        typedef std::uint64_t JobId;
        /// @brief The status / result of a submitted job
        struct Result
        {
            Result(bool running = true) :
            m_running(running),
            m_failed(false),
            m_message{}
            {}
            bool m_running;
            bool m_failed;
            std::string m_message;
        };
        /// @brief maps job IDs to status messages / results
        typedef std::map<JobId, Result> ResultMap;
    public:
        /// @brief Sets up a thread pool 
        /// @param nThreads Number of threads to create
        ThreadPool(std::uint64_t nThreads);
        ThreadPool(const ThreadPool&) = delete;
        ThreadPool& operator = (const ThreadPool&) = delete;
        ThreadPool(ThreadPool&&) = default;
        ThreadPool& operator = (ThreadPool&&) = default;
        ~ThreadPool();

        /// @brief Posts a job for asynchronous processing
        /// @return A unique ID for the submitted job
        JobId post(std::function<void()> job);
        /// @brief Queries status for a specific job, nonblocking
        Result query(JobId id);
        /// @brief Queries status for all running jobs, block until results come available
        /// @return A map of results for terminated jobs, or empty map if all jobs done
        ResultMap query();
        /// @brief Waits for all internal threads to finish
        void join();

    private:
        std::unique_ptr<Impl> m_impl;
    public:
        /// @brief An identifier string for generic (...) exceptions caught
        static const std::string m_genericMessage;
    };
}