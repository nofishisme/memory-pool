#include "PageCache.h"
#include <sys/mman.h>
#include <cstring>

namespace memoryPool{

void* PageCache::allocate_span(size_t num_pages){
     LockGuard lock(mutex_);
    // 查找合适的空闲span
    // lower_bound函数返回第一个大于等于numPages的元素的迭代器
    auto it = free_spans_.lower_bound(num_pages);
    if(it != free_spans_.end()){
        Span* span = it->second;
        // 将取出的span从原有的空闲链表freeSpans_[it->first]中移除
        if(span->next){
            free_spans_[it->first] = span->next;
        } else {
            free_spans_.erase(it);
        }
        // 如果span大于需要的numPages则进行分割
        if(span->num_pages > num_pages){
            Span* newSpan = new Span;
            newSpan->page_addr = static_cast<char*>(span->page_addr) + 
                                 num_pages * PAGE_SIZE;
            newSpan->num_pages = span->num_pages - num_pages;
            newSpan->next = nullptr;

            // 将超出部分放回空闲Span*列表头部
            auto& list = free_spans_[newSpan->num_pages];
            newSpan->next = list;
            list = newSpan;

            span->num_pages = num_pages;
        }
        //记录span信息用于回收
        span_map_[span->page_addr] = span;
        return span->page_addr;
    }

    //没有合适的span，向系统申请
    void* memory = system_alloc(num_pages);
    if(!memory)
        return nullptr;

    //创建新的span
    Span* span = new Span;
    span->page_addr = memory;
    span->num_pages = num_pages;
    span->next = nullptr;
    span_map_[memory] = span;
    return memory;
}

void PageCache::deallocate_span(void* ptr, size_t num_pages){
    LockGuard lock(mutex_);
    //查找对应的span，没找到代表不是pageCache分配的内存，返回
    auto it = span_map_.find(ptr);
    if(it == span_map_.end())
        return;
    
    Span* span = it->second;

    //尝试合并相邻的span
    void* nextAddr = static_cast<char*>(ptr) + num_pages * PAGE_SIZE;
    auto nextIt = span_map_.find(nextAddr);
    if(nextIt != span_map_.end()){
        Span* nextSpan = nextIt->second;
        // 1. 首先检查nextSpan是否在空闲链表中
        bool found = false;
        auto& nextList = free_spans_[nextSpan->num_pages];
        //检查是不是头节点
        if(nextList == nextSpan){
            nextList = nextSpan->next;
            found = true;
        } else if (nextList){
            Span* prev = nextList;
            while (prev->next){
                if (prev->next == nextSpan){
                    //将nextSpan从空闲链表移除
                    prev->next = nextSpan->next;
                    found = true;
                    break;
                }
                prev = prev->next;
            }
        }

        //2，只有找到nextSpan的情况下才进行合并
        if (found) {
            //合并span
            span->num_pages += nextSpan->num_pages;
            span_map_.erase(nextAddr);
            delete nextSpan;
        }
    }

    //将合并后的span通过头插法插入空闲列表
    auto& list = free_spans_[span->num_pages];
    span->next = list;
    list = span;
}

void* PageCache::system_alloc(size_t num_pages){
    size_t size = num_pages * PAGE_SIZE;

    //使用mmp分配内存
    void* ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED)
        return nullptr;
    memset(ptr, 0, size);
    return ptr;
}

}// namespace memoryPool