#pragma once
#include <functional>
#include <memory>
#include <cstdint>
#include <future>

namespace bentoclient
{
    /// @brief Fire and retry tasks in one go
    class Retry
    {
    public:
        typedef std::function<void(std::uint64_t, const std::exception&)> ErrorLogger;
    public:
        /// @brief Set up number of retries for a task
        /// @param nRetries Number of retries after failure. Set to 1 if task is to run twice.
        Retry(std::uint64_t nRetries) :
            m_nRetries(nRetries)
        {}
        Retry(const Retry&) = delete;
        Retry& operator = (const Retry&) = delete;

        Retry(Retry&&) = default;
        Retry& operator = (Retry&&) = default;

        /// @brief Launch and retry a task, which might run in a threadpool.
        /// @tparam T Return type
        /// @param funcToRetry The task to run and retry
        /// @param errorLogger A logger function for failures leading to retries
        /// @return Result type of function
        template <typename T>
        T run(std::function<T()> funcToRetry, ErrorLogger errorLogger)
        {
            std::uint64_t nTry = 0;
            while (true) {
                try {
                    return funcToRetry();
                } catch (const std::exception& e) {
                    if (++nTry > m_nRetries || noRetryError(e)) {
                        // retries exceeded, just rethrow to have error handling elsewhere
                        throw;
                    } else {
                        errorLogger(nTry, e);
                    }
                }
            }
        }
        /// @brief Flag error conditions that are guaranteed to fail again
        /// @param e Std exception that this method examines
        /// @return True if error should not be retried in this loop
        static bool noRetryError(const std::exception& e);
        /// @brief Overflow condition that occurs when response message size exceeded
        static bool isZstdBufferOverflow(const std::exception& e);

    private:
        std::uint64_t m_nRetries;
    };

    /// @brief Delayed retry for tasks on a variadic thread pool
    /// @tparam T Task return type, such as the value_type of a future
    template <typename T>
    class RetryDelayed
    {
    public:
        /// @brief Error logger functor for meaningful reports of a task potentially failing
        typedef std::function<void(std::uint64_t, const std::exception&)> ErrorLogger;
        /// @brief The future returning the return type
        typedef std::future<T> Future;
        /// @brief The function type that will be placed on some threadpool
        typedef std::function<Future()> FuncToRetry;
    public:
        /// @brief Set up retries for an asynchronous task and submit task to pool
        /// @param nRetries Number of retries. Set to 1 if a task shall run maximally twice.
        /// @param funcToRetry The asynchronous job to retry
        /// @param errorLogger Error logging
        RetryDelayed(std::uint64_t nRetries, FuncToRetry funcToRetry, ErrorLogger errorLogger) :
            m_nRetries(nRetries),
            m_nTry(0),
            m_futurePtr{},
            m_funcToRetry(funcToRetry),
            m_errorLogger(errorLogger)
        {
            // queues task on pool and returns future
            m_futurePtr = std::make_unique<Future>(std::move(m_funcToRetry()));
        }
        RetryDelayed(const RetryDelayed&) = delete;
        RetryDelayed& operator = (const RetryDelayed&) = delete;

        RetryDelayed(RetryDelayed&&) = default;
        RetryDelayed& operator = (RetryDelayed&&) = default;

        /// @brief Retrieve the return value from the future
        /// @details The variadic thread pool has packaged tasks keep potential exceptions until
        /// retrieval calling future.get(), when they will be rethrown. Therefore, the retry
        /// must run upon retrievel of asynchronous results pushed on a list for concurrent execution.
        /// @return 
        T retrieve() {
            while (true) {
                try {
                    return m_futurePtr->get();
                } catch(const std::exception& e) {
                    if (++m_nTry > m_nRetries || Retry::noRetryError(e)) {
                        // retries exceeded, just rethrow to have failure handling elsewhere
                        throw;
                    } else {
                        m_errorLogger(m_nTry, e);
                        // queue the task again and get a new future
                        m_futurePtr = std::make_unique<Future>(std::move(m_funcToRetry()));
                    }
                }
            }
        }

    private:
        std::uint64_t m_nRetries;
        std::uint64_t m_nTry;
        std::unique_ptr<Future> m_futurePtr;
        FuncToRetry m_funcToRetry;
        ErrorLogger m_errorLogger;
    };
}
