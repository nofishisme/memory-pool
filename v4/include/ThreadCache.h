#ifndef __ThreadCache_HPP__
#define __ThreadCache_HPP__
#include "Common.h"
namespace memoryPool{

//线程本地缓存
class ThreadCache{
public:
    static ThreadCache* getInstance(){
        static thread_local ThreadCache instance;
        return &instance;
    }

    void* allocate(size_t size);
    void deallocate(void* ptr, size_t size);
private:
    ThreadCache() = default;
    //从中心缓存获取数据
    void* fetchFromCentralCache(size_t index);
    //归还内存到中心缓存
    void returnToCentralCache(void* start, size_t size);
    //计算批量获取的内存块数量
    size_t getBatchNum(size_t size);
    // 判断是否需要归还内存给中心缓存
    bool shouldReturnToCentralCache(size_t index);
private:
    //每个线程的自由链表数组
    std::array<void*, FREE_LIST_SIZE> freeList_;
    std::array<size_t, FREE_LIST_SIZE> freeListSize_;//自由链表大小统计
};

} //namespace memoryPool


#endif