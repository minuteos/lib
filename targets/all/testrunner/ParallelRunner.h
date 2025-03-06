/*
 * Copyright (c) 2025 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * testrunner/ParallelRunner.h
 *
 * Helpers for running workloads in parallel, useful in manually executed
 * brute-force verification tests.
 *
 * Parallel execution is used only in release builds and only on supported
 * targets (e.g. host)
 */

#include <thread>
#include <queue>

#ifndef TESTRUNNER_PARALLEL_SUPPORTED
#if Thost && !DEBUG
#define TESTRUNNER_PARALLEL_SUPPORTED   1
#else
#define TESTRUNNER_PARALLEL_SUPPORTED   0
#endif
#endif

#if TESTRUNNER_PARALLEL_SUPPORTED

template <typename T>
struct MessageQueue
{
    std::mutex mtx;
    std::condition_variable cv;
    std::deque<T> q;
    bool done = false;

    void Enqueue(const T &item)
    {
        std::lock_guard lck(mtx);
        q.push_back(item);
        cv.notify_all();
    }

    bool Dequeue(T &out)
    {
        std::lock_guard lck(mtx);
        if (q.empty())
        {
            return false;
        }
        out = q.front();
        q.pop_front();
        return true;
    }

    bool WaitAndDequeue(T &out)
    {
        std::unique_lock lck(mtx);
        while (q.empty())
        {
            if (done)
            {
                return false;
            }
            cv.wait(lck);
        }
        out = q.front();
        q.pop_front();
        return true;
    }

    void Close()
    {
        std::lock_guard lck(mtx);
        done = true;
        cv.notify_all();
    }
};

template<typename TReq, typename TRes>
struct ParallelRunner
{
    MessageQueue<TReq> req;
    MessageQueue<TRes> res;
    std::vector<std::thread *> workers;
    std::function<TRes(TReq)> work;

    ParallelRunner(std::function<TRes(TReq)> work)
        : work(work)
    {
#if DIAG || DEBUG || 0
        int numWorkers = 1;
#else
        int numWorkers = std::thread::hardware_concurrency();
#endif
        for (int i = 0; i < numWorkers; i++)
        {
            workers.push_back(new std::thread(&ParallelRunner::Worker, this));
        }
    }

    ~ParallelRunner()
    {
        req.Close();
        for (auto t : workers)
        {
            t->join();
            delete t;
        }
    }

    void Worker()
    {
        TReq r;
        while (req.WaitAndDequeue(r))
        {
            res.Enqueue(work(r));
        }
        res.Close();
    }

    void Enqueue(TReq&& r)
    {
        req.Enqueue(r);
    }

    void Close()
    {
        req.Close();
    }

    bool WaitAndDequeue(TRes& r)
    {
        return res.WaitAndDequeue(r);
    }
};

#else

template<typename TReq, typename TRes>
struct ParallelRunner
{
    std::function<TRes(TReq)> work;

    ParallelRunner(std::function<TRes(TReq)> work)
        : work(work)
    {
    }

    void Enqueue(TReq&& r)
    {
        work(r);
    }
};

#endif
