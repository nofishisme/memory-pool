#ifndef __CentralCahce_HPP_
#define __CentralCahce_HPP_
#include "Common.h"
#include <mutex>

namespace memoryPool{

class CentralCache
{
public:
    static CentralCache& getInstance(){
        static CentralCache instance;
        return instance;
    }
    void* fetch_range(size_t index, size_t batch_num);
    void return_range(void* start, size_t size, size_t bytes);
private:
    CentralCache(){
        for(auto& ptr : centralFreeList_){
            ptr.store(nullptr, std::memory_order_relaxed);
        }
        // 初始化所有锁
        for(auto& lock : locks_){
            lock.clear();
        }
    }
    // 从页缓存获取内存
    void *fetch_from_page_cache(size_t size);
private:
    //中心缓存的自由链表
    std::array<std::atomic<void*>, FREE_LIST_SIZE> centralFreeList_;
    //用于同步的自旋锁
    std::array<std::atomic_flag, FREE_LIST_SIZE> locks_;
};

}// namespace memoryPool

#endif