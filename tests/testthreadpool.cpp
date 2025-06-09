#include <catch2/catch_test_macros.hpp>
#include "bentoclient/threadpool.hpp"
#include <list>
#include <thread>
#include <chrono>

TEST_CASE( "Threadpool join and query", "[threadjoin]" ) {
    {
    std::list<bentoclient::ThreadPool::JobId> jobList;
    int nThreads = 10;
    bentoclient::ThreadPool::ResultMap results;
    {
        bentoclient::ThreadPool pool(nThreads);
        jobList.push_back(pool.post([](){
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }));
        pool.join();
        results = pool.query();
    }
    for (auto id : jobList)
    {
        auto it = results.find(id);
        REQUIRE(it != results.end());
        REQUIRE(it->second.m_failed == false);
        REQUIRE(it->second.m_running == false);
    }
    }

    {
    std::list<bentoclient::ThreadPool::JobId> jobList;
    std::uint64_t nThreads = 10;
    bentoclient::ThreadPool::ResultMap results;
    {
        bentoclient::ThreadPool pool(nThreads);
        for (std::uint64_t i = 0; i < nThreads; ++i) {
            jobList.push_back(pool.post([i](){
                std::this_thread::sleep_for(std::chrono::milliseconds(i * 50));
            }));
        }
        pool.join();
        results = pool.query();
     }
    for (auto id : jobList)
    {
        auto it = results.find(id);
        REQUIRE(it != results.end());
        REQUIRE(it->second.m_failed == false);
        REQUIRE(it->second.m_running == false);
    }
    }

    {
    std::list<bentoclient::ThreadPool::JobId> jobList;
    std::uint64_t nThreads = 10;
    bentoclient::ThreadPool::ResultMap results;
    {
        bentoclient::ThreadPool pool(nThreads);
        for (std::uint64_t i = 0; i < nThreads; ++i) {
            jobList.push_back(pool.post([i](){
                std::this_thread::sleep_for(std::chrono::milliseconds(i * 100));
            }));
        }
        while (results.size() < nThreads) {
            bentoclient::ThreadPool::ResultMap r = pool.query();
            for (auto it = r.begin(); it != r.end(); ++it) {
                results.emplace(it->first, std::move(it->second));
            }
        }
        // Verify a query without pending jobs does not hang the client
        bentoclient::ThreadPool::ResultMap r = pool.query();
        REQUIRE(r.size() == 0);
    }
    for (auto id : jobList)
    {
        auto it = results.find(id);
        REQUIRE(it != results.end());
        REQUIRE(it->second.m_failed == false);
        REQUIRE(it->second.m_running == false);
    }
    }

    {
    std::list<bentoclient::ThreadPool::JobId> jobList;
    std::uint64_t nThreads = 10;
    bentoclient::ThreadPool pool(nThreads);
    int nSleep = 10;
    for (std::uint64_t i = 0; i < nThreads; ++i) {
        jobList.push_back(pool.post([i,nSleep](){
            std::this_thread::sleep_for(std::chrono::milliseconds(i * nSleep));
        }));
    }
    while (jobList.size() > 0) {
        for (auto it = jobList.begin(); it != jobList.end(); ) {
            std::this_thread::sleep_for(std::chrono::milliseconds(nSleep/2));
            bentoclient::ThreadPool::Result r = pool.query(*it);
            if (!r.m_running) {
                jobList.erase(it++);
            } else {
                ++it;
            }
        }
    }
    REQUIRE(jobList.size() == 0);
    try {
        pool.query(1);
        REQUIRE(false);
    } catch (const std::invalid_argument ex) {
        REQUIRE(std::string("Invalid JobId 1") == std::string(ex.what()));
    }
    }

}

#include <mutex>
#include <condition_variable>
TEST_CASE( "Threadpool queries threadpool", "[threadthread]" ) {
    std::uint64_t nWorkerThreads = 10;
    bentoclient::ThreadPool worker(nWorkerThreads);
    std::uint64_t nQueryThreads = 5;
    bentoclient::ThreadPool querier(nQueryThreads);
    std::list<bentoclient::ThreadPool::JobId> queryJobs, workerJobs;
    bentoclient::ThreadPool::ResultMap results;
    std::mutex mutex, condition_mutex;
    std::condition_variable condition;
    int nSleep = 10;
    for (std::uint64_t i = 0; i < 2*nQueryThreads; ++i)
    {
        bentoclient::ThreadPool::JobId id = querier.post([&mutex, &condition,
            &condition_mutex, &workerJobs, &results, nWorkerThreads, &worker, nSleep]{
            for (std::uint64_t j = 0; j < 2*nWorkerThreads; ++j)
            {
                bentoclient::ThreadPool::JobId idw = worker.post([&mutex, nSleep, j, 
                        &condition, &condition_mutex, nWorkerThreads]{
                    std::this_thread::sleep_for(std::chrono::milliseconds(nSleep * j));
                    if (j > nWorkerThreads)
                    {
                        std::unique_lock<std::mutex> lock(mutex);
                        condition.wait(lock);
                    }

                });
                {
                    std::lock_guard<std::mutex> lock(mutex);
                    workerJobs.push_back(idw);
                }
            }
            auto subResults = worker.query();
            while (!subResults.empty())
            {
                for (auto it = subResults.begin(); it != subResults.end(); ++it)
                {
                    std::lock_guard<std::mutex> lock(mutex);
                    REQUIRE(it->second.m_failed == false);
                    results.emplace(it->first, std::move(it->second));
                }
                subResults = worker.query();
            }
        
        });
        queryJobs.push_back(id);
    }
    auto workerSize = [&workerJobs,&mutex](){
        std::lock_guard<std::mutex> lock(mutex);
        return workerJobs.size();
    };
    auto resultsSize = [&results,&mutex](){
        std::lock_guard<std::mutex> lock(mutex);
        return results.size();
    };
    std::uint64_t expectedWorkers = nQueryThreads * nWorkerThreads * 4;
    while (workerSize() < expectedWorkers) {
        std::this_thread::sleep_for(std::chrono::milliseconds(nSleep));
        condition.notify_all();
    }
    REQUIRE(resultsSize() < workerJobs.size());
    while (resultsSize() < workerJobs.size())
    {
        condition.notify_all();
        std::this_thread::sleep_for(std::chrono::milliseconds(nSleep));
    }
    bentoclient::ThreadPool::ResultMap endResults;
    while (endResults.size() < nQueryThreads * 2)
    {
        auto subResults = querier.query();
        for (auto it = subResults.begin(); it != subResults.end();++it)
        {
            REQUIRE(it->second.m_failed == false);
            endResults.emplace(it->first, std::move(it->second));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(nSleep));
    }
    REQUIRE(endResults.size() == nQueryThreads*2);
}
