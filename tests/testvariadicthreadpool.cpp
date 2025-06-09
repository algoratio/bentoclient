#include <catch2/catch_test_macros.hpp>
#include "bentoclient/threadpool.hpp"
#include "bentoclient/variadicthreadpool.hpp"
#include "bentoclient/retry.hpp"
#include <list>
#include <thread>
#include <chrono>
#include <sstream>
#include <mutex>
#include <fmt/core.h>

TEST_CASE( "Variadic Threadpool immediately accessing future", "[varthreadimmediate]" ) {
    std::uint64_t nWorkerThreads = 10;
    bentoclient::ThreadPool worker(nWorkerThreads);
    std::uint64_t nVarThreads = 5;
    bentoclient::VariadicThreadPool varPool(nVarThreads);
    int nSleep = 10;
    typedef std::function<std::string(int)> VarFunc;
    VarFunc varFun = [](int nSleep) -> std::string {
        std::this_thread::sleep_for(std::chrono::milliseconds(nSleep));
        std::ostringstream oss;
        oss << "Thread ID(" << std::this_thread::get_id() << ") slept "
            << nSleep << " milliseconds";
        return oss.str();
    };
    std::mutex mutex;
    std::list<std::string> results;
    for (std::uint64_t i = 0; i < 2 * nWorkerThreads; ++i)
    {
        bentoclient::ThreadPool::JobId jobId = worker.post([
            &varPool, &varFun, &mutex, &results, nSleep, nVarThreads](){
            for (std::uint64_t j = 0; j < 2 * nVarThreads; ++j)
            {
                auto future = varPool.post(varFun, nSleep * j);
                std::string result = future.get();
                std::lock_guard<std::mutex> lock(mutex);
                results.emplace_back(std::move(result));
            }
        });
    }
    worker.join();
    REQUIRE(results.size() == nWorkerThreads * nVarThreads * 4);
}

TEST_CASE( "Variadic Threadpool immediately retrying future", "[varthreadimmediateretry]" ) {
    std::uint64_t nWorkerThreads = 10;
    bentoclient::ThreadPool worker(nWorkerThreads);
    std::uint64_t nVarThreads = 5;
    bentoclient::VariadicThreadPool varPool(nVarThreads);
    int nSleep = 10;
    std::atomic<std::uint64_t> counter(0);
    typedef std::function<std::string(int)> VarFunc;
    VarFunc varFun = [&counter](int nSleep) -> std::string {
        if (++counter %5 == 0) {
            throw std::runtime_error(fmt::format("Bad count {}", counter.load()));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(nSleep));
        std::ostringstream oss;
        oss << "Thread ID(" << std::this_thread::get_id() << ") slept "
            << nSleep << " milliseconds";
        return oss.str();
    };
    std::mutex mutex;
    std::list<std::string> results;
    std::list<std::string> errors;
    using FuncToRetry = std::function<std::string()>;
    for (std::uint64_t i = 0; i < 2 * nWorkerThreads; ++i)
    {
        bentoclient::ThreadPool::JobId jobId = worker.post([
            &varPool, &varFun, &mutex, &results, &errors, nSleep, nVarThreads](){
            for (std::uint64_t j = 0; j < 2 * nVarThreads; ++j)
            {
                FuncToRetry funcToRetry = [&varPool, &varFun, nSleep, j](){
                    auto future = varPool.post(varFun, nSleep * j);
                    return future.get();
                };
                bentoclient::Retry::ErrorLogger errorLogger = [&mutex,&errors](std::uint64_t nTry, const std::exception& e){
                    std::ostringstream oss;
                    oss << "FuncToRetry fails nTry[" << nTry << "] with exception " << e.what() << std::endl;
                    std::lock_guard<std::mutex> lock(mutex);
                    errors.emplace_back(std::move(oss.str())); 
                };
                std::uint64_t nRetries = 20;
                bentoclient::Retry retry(nRetries);
                std::string result = retry.run(funcToRetry, errorLogger);
                std::lock_guard<std::mutex> lock(mutex);
                results.emplace_back(std::move(result));
            }
        });
    }
    worker.join();
    REQUIRE(results.size() == nWorkerThreads * nVarThreads * 4);
    REQUIRE(errors.size() > 0);
    bool foundNoLike = false;
    for (auto it = results.begin(); it != results.end(); ++it)
    {
        if ((*it).find("Bad count") != std::string::npos)
        {
            foundNoLike = true;
            break;
        }
    }
    REQUIRE(foundNoLike == false);
}

TEST_CASE( "Variadic Threadpool delayed accessing future", "[varthreaddelayed]" ) {
    std::uint64_t nWorkerThreads = 10;
    bentoclient::ThreadPool worker(nWorkerThreads);
    std::uint64_t nVarThreads = 5;
    bentoclient::VariadicThreadPool varPool(nVarThreads);
    int nSleep = 10;
    typedef std::function<std::string(int)> VarFunc;
    VarFunc varFun = [](int nSleep) -> std::string {
        std::this_thread::sleep_for(std::chrono::milliseconds(nSleep));
        std::ostringstream oss;
        oss << "Thread ID(" << std::this_thread::get_id() << ") slept "
            << nSleep << " milliseconds";
        return oss.str();
    };
    std::mutex mutex;
    std::list<std::string> results;
    for (std::uint64_t i = 0; i < 2 * nWorkerThreads; ++i)
    {
        bentoclient::ThreadPool::JobId jobId = worker.post([
            &varPool, &varFun, &mutex, &results, nSleep, nVarThreads](){
            std::list<std::future<std::string>> futures;
            for (std::uint64_t j = 0; j < 2 * nVarThreads; ++j)
            {
                auto future = varPool.post(varFun, nSleep * j);
                futures.emplace_back(std::move(future));
            }
            for (auto& future : futures) {
                std::string result = future.get();
                std::lock_guard<std::mutex> lock(mutex);
                results.emplace_back(std::move(result));
            }
        });
    }
    worker.join();
    REQUIRE(results.size() == nWorkerThreads * nVarThreads * 4);
}

TEST_CASE( "Variadic Threadpool passing exceptions to ThreadPool", "[varthreadexcept]" ) {
    std::uint64_t nWorkerThreads = 10;
    bentoclient::ThreadPool worker(nWorkerThreads);
    std::uint64_t nVarThreads = 5;
    bentoclient::VariadicThreadPool varPool(nVarThreads);
    int nSleep = 20;
    typedef std::function<std::string(int)> VarFunc;
    VarFunc varFun = [](int nSleep) -> std::string {
        if (nSleep == 40) {
            throw std::runtime_error("I don't like 40");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(nSleep));
        std::ostringstream oss;
        oss << "Thread ID(" << std::this_thread::get_id() << ") slept "
            << nSleep << " milliseconds";
        return oss.str();
    };
    std::mutex mutex;
    std::list<std::string> results;
    for (std::uint64_t i = 0; i < 2 * nWorkerThreads; ++i)
    {
        bentoclient::ThreadPool::JobId jobId = worker.post([
            &varPool, &varFun, &mutex, &results, nSleep, nVarThreads](){
            std::list<std::future<std::string>> futures;
            for (std::uint64_t j = 0; j < 2 * nVarThreads; ++j)
            {
                auto future = varPool.post(varFun, nSleep * j);
                futures.emplace_back(std::move(future));
            }
            for (auto& future : futures) {
                std::string result = future.get();
                std::lock_guard<std::mutex> lock(mutex);
                results.emplace_back(std::move(result));
            }
        });
    }
    worker.join();
    bentoclient::ThreadPool::ResultMap resultMap = worker.query();
    REQUIRE(resultMap.size() == nWorkerThreads * 2);
    for (auto it = resultMap.begin(); it != resultMap.end(); ++it)
    {
        REQUIRE(it->second.m_failed == true);
        REQUIRE(it->second.m_message == "I don't like 40");
    }
    // all worker threads die at the third sleep time and then don't deliver results
    REQUIRE(results.size() == nWorkerThreads * 2 * 2);

}

TEST_CASE( "ThreadPool tasks handling Variadic Threadpool exceptions", "[threadhandlevar]" ) {
    std::uint64_t nWorkerThreads = 10;
    bentoclient::ThreadPool worker(nWorkerThreads);
    std::uint64_t nVarThreads = 5;
    bentoclient::VariadicThreadPool varPool(nVarThreads);
    int nSleep = 10;
    typedef std::function<std::string(int)> VarFunc;
    VarFunc varFun = [](int nSleep) -> std::string {
        if (nSleep == 40) {
            throw std::runtime_error("I don't like 40");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(nSleep));
        std::ostringstream oss;
        oss << "Thread ID(" << std::this_thread::get_id() << ") slept "
            << nSleep << " milliseconds";
        return oss.str();
    };
    std::mutex mutex;
    std::list<std::string> results;
    for (std::uint64_t i = 0; i < 2 * nWorkerThreads; ++i)
    {
        bentoclient::ThreadPool::JobId jobId = worker.post([
            &varPool, &varFun, &mutex, &results, nSleep, nVarThreads](){
            std::list<std::future<std::string>> futures;
            for (std::uint64_t j = 0; j < 2 * nVarThreads; ++j)
            {
                auto future = varPool.post(varFun, nSleep * j);
                futures.emplace_back(std::move(future));
            }
            for (auto& future : futures) {
                std::string result;
                try {
                    result = future.get();
                } catch (const std::exception& e)
                {
                    result = e.what();
                }
                std::lock_guard<std::mutex> lock(mutex);
                results.emplace_back(std::move(result));
            }
        });
    }
    worker.join();
    bentoclient::ThreadPool::ResultMap resultMap = worker.query();
    REQUIRE(resultMap.size() == nWorkerThreads * 2);
    for (auto it = resultMap.begin(); it != resultMap.end(); ++it)
    {
        REQUIRE(it->second.m_failed == false);
    }
    // only varPool threads that die at the third sleep time don't deliver results
    REQUIRE(results.size() == nWorkerThreads * nVarThreads * 2 * 2);
    bool foundNoLike = false;
    for (auto it = results.begin(); it != results.end(); ++it)
    {
        if (*it == "I don't like 40")
        {
            foundNoLike = true;
            break;
        }
    }
    REQUIRE(foundNoLike == true);
}

TEST_CASE( "ThreadPool with delayed retries of sporadic failures", "[threaddelayretry]" ) {
    std::uint64_t nWorkerThreads = 10;
    bentoclient::ThreadPool worker(nWorkerThreads);
    std::uint64_t nVarThreads = 5;
    bentoclient::VariadicThreadPool varPool(nVarThreads);
    int nSleep = 10;
    std::atomic<std::uint64_t> counter(0);
    typedef std::function<std::string(int)> VarFunc;
    VarFunc varFun = [&counter](int nSleep) -> std::string {
        if (++counter % 5 == 0) {
            throw std::runtime_error(fmt::format("Bad count {}", counter.load()));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(nSleep));
        std::ostringstream oss;
        oss << "Thread ID(" << std::this_thread::get_id() << ") slept "
            << nSleep << " milliseconds";
        return oss.str();
    };
    using RString = bentoclient::RetryDelayed<std::string>;
    std::mutex mutex;
    std::list<std::string> results;
    std::list<std::string> errors;
    for (std::uint64_t i = 0; i < 2 * nWorkerThreads; ++i)
    {
        bentoclient::ThreadPool::JobId jobId = worker.post([
            &varPool, &varFun, &mutex, &results, &errors, nSleep, nVarThreads](){
            std::list<RString> futures;
            // failures can add up randomly, but we want the test case to always succeed
            // so set retries to very high.
            std::uint64_t nRetries = 20;
            for (std::uint64_t j = 0; j < 2 * nVarThreads; ++j)
            {
                RString::FuncToRetry funcToRetry = [&varFun,nSleep,&varPool,j](){
                    return varPool.post(varFun, nSleep * j);
                };
                RString::ErrorLogger errorLogger = [&mutex,&errors](std::uint64_t nTry, const std::exception& e)
                {
                    std::ostringstream oss;
                    oss << "FuncToRetry fails [" << nTry << "] with exception " << e.what() << std::endl;
                    std::lock_guard<std::mutex> lock(mutex);
                    errors.emplace_back(std::move(oss.str()));
                };
                futures.emplace_back(std::move(RString(nRetries, funcToRetry, errorLogger)));
            }
            for (auto& future : futures) {
                std::string result;
                try {
                    result = future.retrieve();
                } catch (const std::exception& e)
                {
                    result = e.what();
                }
                std::lock_guard<std::mutex> lock(mutex);
                results.emplace_back(std::move(result));
            }
        });
    }
    worker.join();
    bentoclient::ThreadPool::ResultMap resultMap = worker.query();
    REQUIRE(resultMap.size() == nWorkerThreads * 2);
    for (auto it = resultMap.begin(); it != resultMap.end(); ++it)
    {
        REQUIRE(it->second.m_failed == false);
    }
    // var threads that throw are all retried
    REQUIRE(results.size() == nWorkerThreads * nVarThreads * 2 * 2);
    bool foundNoLike = false;
    for (auto it = results.begin(); it != results.end(); ++it)
    {
        if ((*it).find("Bad count") != std::string::npos)
        {
            foundNoLike = true;
            break;
        }
    }
    REQUIRE(foundNoLike == false);
    REQUIRE(errors.size() > 0);
}
