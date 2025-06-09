#include "bentoclient/requester.hpp"
#include "bentoclient/getter.hpp"
#include "bentoclient/retriever.hpp"
#include "bentoclient/persister.hpp"
#include "bentoclient/threadpool.hpp"

using namespace bentoclient;

static_assert(std::is_same_v<ThreadPool::JobId, Requester::JobId>, "JobId in threadpool and requester must have same type");

Requester::Requester(std::unique_ptr<Getter>&& getter, 
    std::unique_ptr<Retriever>&& retriever,
    std::unique_ptr<Persister>&& persister) :
    m_getter(std::move(getter)),
    m_retriever(std::move(retriever)),
    m_persister(std::move(persister))
{}
