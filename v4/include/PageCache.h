#ifndef __PageCache_HPP_
#define __PageCache_HPP_
#include "Common.h"
#include <map>
#include <mutex>

namespace memoryPool{

typedef std::lock_guard<std::mutex> LockGuard;
class PageCache
{
public:
    static const size_t PAGE_SIZE = 4096; //4K页大小
    static PageCache& getInstance(){
        static PageCache instance;
        return instance;
    }

    //分配指定页数的span
    void* allocate_span(size_t num_pages);
    // 释放span
    void deallocate_span(void* ptr, size_t num_pages);
private:
    PageCache() = default;
    ~PageCache(){
        for(auto ii : span_map_){
            if(ii.second){
                delete ii.second;
            }
        }
    }
    //向系统申请内存
    void* system_alloc(size_t num_pages);
private:
    struct Span
    {
        void* page_addr; //页起始地址
        size_t num_pages; //页数
        Span* next;  //链表指针
    };
    //按页数管理空闲span，不同页数对应不同Span链表
    std::map<size_t, Span*> free_spans_;
    //页起始地址到span的映射，用于回收
    std::map<void*, Span*> span_map_;
    std::mutex mutex_;
};

}// namespace memoryPool

#endif