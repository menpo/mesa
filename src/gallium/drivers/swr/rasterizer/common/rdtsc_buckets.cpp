/****************************************************************************
* Copyright (C) 2014-2015 Intel Corporation.   All Rights Reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice (including the next
* paragraph) shall be included in all copies or substantial portions of the
* Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
* IN THE SOFTWARE.
* 
* @file rdtsc_buckets.cpp
* 
* @brief implementation of rdtsc buckets.
* 
* Notes:
* 
******************************************************************************/
#include "rdtsc_buckets.h"
#include <inttypes.h>

THREAD UINT tlsThreadId = 0;

void BucketManager::RegisterThread(const std::string& name)
{
    // lazy evaluate threadviz knob
    if (!mThreadViz && KNOB_BUCKETS_ENABLE_THREADVIZ)
    {
        uint32_t pid = GetCurrentProcessId();
        std::stringstream str;
        str << "threadviz." << pid;
        mThreadVizDir = str.str();
        CreateDirectory(mThreadVizDir.c_str(), NULL);

        mThreadViz = true;
    }

    BUCKET_THREAD newThread;
    newThread.name = name;
    newThread.root.children.reserve(mBuckets.size());
    newThread.root.id = 0;
    newThread.root.pParent = nullptr;
    newThread.pCurrent = &newThread.root;

    mThreadMutex.lock();

    // assign unique thread id for this thread
    size_t id = mThreads.size();
    newThread.id = (UINT)id;
    tlsThreadId = (UINT)id;

    // open threadviz file if enabled
    if (mThreadViz)
    {
        std::stringstream ss;
        ss << mThreadVizDir << "\\threadviz_thread." << newThread.id << ".dat";
        newThread.vizFile = fopen(ss.str().c_str(), "wb");
    }

    // store new thread
    mThreads.push_back(newThread);

    mThreadMutex.unlock();
}

UINT BucketManager::RegisterBucket(const BUCKET_DESC& desc)
{
    mThreadMutex.lock();
    size_t id = mBuckets.size();
    mBuckets.push_back(desc);
    mThreadMutex.unlock();
    return (UINT)id;
}

void BucketManager::PrintBucket(FILE* f, UINT level, uint64_t threadCycles, uint64_t parentCycles, const BUCKET& bucket)
{
    const char *arrows[] = {
        "",
        "|-> ",
        "    |-> ",
        "        |-> ",
        "            |-> ",
        "                |-> ",
        "                    |-> ",
        "                        |-> ",
        "                            |-> ",
    };

    // compute percent of total cycles used by this bucket
    float percentTotal = (float)((double)bucket.elapsed / (double)threadCycles * 100.0);

    // compute percent of parent cycles used by this bucket
    float percentParent = (float)((double)bucket.elapsed / (double)parentCycles * 100.0);

    // compute average cycle count per invocation
    uint64_t CPE = bucket.elapsed / bucket.count;

    BUCKET_DESC &desc = mBuckets[bucket.id];

    // construct hierarchy visualization
    char hier[80];
    strcpy(hier, arrows[level]);
    strcat(hier, desc.name.c_str());

    // print out
    fprintf(f, "%6.2f %6.2f %-10" PRIu64 " %-10" PRIu64 " %-10u %-10lu %-10u %s\n", 
        percentTotal, 
        percentParent, 
        bucket.elapsed, 
        CPE, 
        bucket.count, 
        (unsigned long)0, 
        (uint32_t)0, 
        hier
    );

    // dump all children of this bucket
    for (const BUCKET& child : bucket.children)
    {
        if (child.count)
        {
            PrintBucket(f, level + 1, threadCycles, bucket.elapsed, child);
        }
    }
}

void BucketManager::PrintThread(FILE* f, const BUCKET_THREAD& thread)
{
    // print header
    fprintf(f, "\nThread %u (%s)\n", thread.id, thread.name.c_str());
    fprintf(f, " %%Tot   %%Par  Cycles     CPE        NumEvent   CPE2       NumEvent2  Bucket\n");

    // compute thread level total cycle counts across all buckets from root
    const BUCKET& root = thread.root;
    uint64_t totalCycles = 0;
    for (const BUCKET& child : root.children)
    {
        totalCycles += child.elapsed;
    }

    for (const BUCKET& child : root.children)
    {
        if (child.count)
        {
            PrintBucket(f, 0, totalCycles, totalCycles, child);
        }
    }
}

void BucketManager::DumpThreadViz()
{
    // ensure all thread data is flushed
    mThreadMutex.lock();
    for (auto& thread : mThreads)
    {
        fflush(thread.vizFile);
        fclose(thread.vizFile);
    }
    mThreadMutex.unlock();

    // dump bucket descriptions
    std::stringstream ss;
    ss << mThreadVizDir << "\\threadviz_buckets.dat";

    FILE* f = fopen(ss.str().c_str(), "wb");
    for (auto& bucket : mBuckets)
    {
        Serialize(f, bucket);
    }
    fclose(f);
}

void BucketManager::PrintReport(const std::string& filename)
{
    if (mThreadViz)
    {
        DumpThreadViz();
    }
    else
    {
        FILE* f = fopen(filename.c_str(), "w");

        mThreadMutex.lock();
        for (const BUCKET_THREAD& thread : mThreads)
        {
            PrintThread(f, thread);
            fprintf(f, "\n");
        }
        mThreadMutex.unlock();

        fclose(f);
    }
}

void BucketManager_StartBucket(BucketManager* pBucketMgr, uint32_t id)
{
    pBucketMgr->StartBucket(id);
}

void BucketManager_StopBucket(BucketManager* pBucketMgr, uint32_t id)
{
    pBucketMgr->StopBucket(id);
}
