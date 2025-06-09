#include "bentoclient/threadpool.hpp"
#include "bentoclient/variadicthreadpool.hpp"
#include <map>
#include <set>
#include <fmt/core.h>

using namespace bentoclient;

class ThreadPool::Impl
{
public:
    Impl(std::uint64_t nThreads) : 
    m_pool(nThreads), 
    m_jobId(0), 
    m_resultMap{},
    m_pendingJobs{} 
    {}

    void join()
    {
        m_pool.join();
    }

    JobId post(std::function<void()> job)
    {
        JobId jobId = getNextJobId();
        std::function<void()> wrapper = [this, job, jobId](){
            Result result(false);
            try {
                job();
            } catch (const std::exception& ex) {
                result.m_failed = true;
                result.m_message = ex.what();
            } catch (...) {
                result.m_failed = true;
                result.m_message = m_genericMessage;
            }
            this->storeResult(jobId, std::move(result));
            this->m_condition.notify_all();
        };
        pushPending(jobId);
        m_pool.postNoFuture(wrapper);
        return jobId;
    }

    Result query(JobId id)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_resultMap.find(id);
        if (it != m_resultMap.end()) {
        
            Result res(std::move(it->second));
            m_resultMap.erase(it);
            return res;
        }
        auto pjIt = m_pendingJobs.find(id);
        if (pjIt == m_pendingJobs.end()){
            throw std::invalid_argument(fmt::format("Invalid JobId {}", id));
        }
        return Result();
    }

    ResultMap query()
    {
        std::unique_lock<std::mutex> lock(m_mutex);

        m_condition.wait(lock, [this](){
            return !this->m_resultMap.empty() 
                // Upon notification, lock must be released if other threads
                // retrieved results and there are no pending jobs.
                || this->m_pendingJobs.empty();
        });
        ResultMap ret;
        for (auto it = m_resultMap.begin(); it != m_resultMap.end();)
        {
            ret.emplace(it->first, std::move(it->second));
            m_resultMap.erase(it++);
        }
        return ret;
    }

    JobId getNextJobId()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return ++m_jobId;
    }

    void storeResult(JobId id, Result&& result)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto pjIt = m_pendingJobs.find(id);
        if (pjIt != m_pendingJobs.end()) {
            m_pendingJobs.erase(pjIt);
        } else {
            result.m_message =fmt::format("Error corrupted ThreadPool missing pending JobId {}", id);
            result.m_failed = true;
        }
        m_resultMap.emplace(id, std::move(result));
    }
private:
    void pushPending(JobId id)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_pendingJobs.insert(id);
    }

private:
    VariadicThreadPool m_pool;
    JobId m_jobId;
    ResultMap m_resultMap;
    std::set<JobId> m_pendingJobs;
    std::mutex m_mutex;
    std::condition_variable m_condition;
};

const std::string ThreadPool::m_genericMessage("ThreadPool::Impl: Generic exception (...)");

ThreadPool::ThreadPool(std::uint64_t nThreads) :
    m_impl(std::make_unique<Impl>(nThreads))
{}

ThreadPool::~ThreadPool()
{
    m_impl->join();
}

ThreadPool::JobId ThreadPool::post(std::function<void()> job)
{
    return m_impl->post(job);
}

ThreadPool::Result ThreadPool::query(JobId id)
{
    return m_impl->query(id);
}

ThreadPool::ResultMap ThreadPool::query()
{
    return m_impl->query();
}

void ThreadPool::join()
{
    m_impl->join();
}
