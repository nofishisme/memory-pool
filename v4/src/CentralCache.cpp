#include "CentralCache.h"
#include "PageCache.h"
#include <cassert>
#include <thread>

namespace memoryPool{

//每次从PageCache获取span的大小
static const size_t SPAN_PAGES = 8;

void* CentralCache::fetch_range(size_t index, size_t batch_num){
    // 索引检查，当索引大于等于FREE_LIST_SIZE时，说明申请内存过大应直接向系统申请
    if (index >= FREE_LIST_SIZE || batch_num == 0){
        return nullptr;
    }
    //自旋锁保护
    while (locks_[index].test_and_set(std::memory_order_acquire)){
        std::this_thread::yield();// 添加线程让步，避免忙等待，避免过度消耗CPU
    }

    void* result = nullptr;
    try{
        //尝试从中心缓存获取内存块
        result = centralFreeList_[index].load(std::memory_order_relaxed);
        if(!result){
            //如果中心缓存为空，从页缓存获取新的内存块
            size_t size = (index + 1) * ALIGNMENT;
            result = fetch_from_page_cache(size);

            if (!result){
                locks_[index].clear(std::memory_order_release);
                return nullptr;
            }

            //将从PageCache获取的内存块切分成小块
            char* start = static_cast<char*>(result);
            //只有小于SPAN_PAGES * PageCache::PAGE_SIZE的size才会被分块
            size_t total_blocks = (SPAN_PAGES * PageCache::PAGE_SIZE) / size;
            size_t alloc_blocks = std::min(batch_num, total_blocks);

            //构建返回给ThreadCache的内存块链表
            if (alloc_blocks > 1){
                // 确保至少有两个块才构建链表
                // 构建链表
                for (size_t i = 1; i < alloc_blocks; ++i){
                    void* current = start + (i - 1) * size;
                    void* next = start + i * size;
                    *reinterpret_cast<void**>(current) = next; 
                }
                *reinterpret_cast<void**>(start + (alloc_blocks - 1) * size) = nullptr;
            }

            // 构建保留在CentralCache的链表
            if (total_blocks > alloc_blocks){
                void* remainStart = start + alloc_blocks * size;
                for (size_t i = alloc_blocks + 1; i < total_blocks; ++i){
                    void* current = start + (i - 1) * size;
                    void* next = start + i * size;
                    *reinterpret_cast<void**>(current) = next;
                }
                *reinterpret_cast<void**>(start + (total_blocks - 1) * size) = nullptr;
                centralFreeList_[index].store(remainStart, std::memory_order_release);
            }
        } else { //如果中心缓存有index对应大小的缓存块
            //从现有链表中获取指定数量的块
            void* current = result;
            void* prev = nullptr;
            size_t count = 0;

            while (current && count < batch_num){
                prev = current;
                current = *reinterpret_cast<void**>(current);
                count++;
            }
            if (prev) { // 当前centralFreeList_[index]链表上的内存块大于batchNum时需要用到 
                *reinterpret_cast<void**>(prev) = nullptr;
            }

            centralFreeList_[index].store(current, std::memory_order_release);
        }
    } catch (...) {
        locks_[index].clear(std::memory_order_release);
        throw;
    }

    //释放锁
    locks_[index].clear(std::memory_order_release);
    return result;
} 

void CentralCache::return_range(void* start, size_t size, size_t index){
    //当索引大于等于FREE_LIST_SIZE时，说明内存过大应直接向系统归还
    if (!start || index >= FREE_LIST_SIZE)
        return;
    while (locks_[index].test_and_set(std::memory_order_acquire)){
        std::this_thread::yield();
    }

    try {
        // 找到要归还的链表的最后一个节点
        void* end = start;
        size_t count = 1;
        while (*reinterpret_cast<void**>(end) != nullptr && count < size) {
            end = *reinterpret_cast<void**>(end);
            count++;
        }
        // 将归还的链表链接到中心缓存的链表头部
        void* current = centralFreeList_[index].load(std::memory_order_relaxed);
        *reinterpret_cast<void**>(end) = current;// 将原链表头接到归还链表的尾部
        centralFreeList_[index].store(start, std::memory_order_release);// 将归还的链表头设为新的链表头
    } catch (...){
        locks_[index].clear(std::memory_order_release);
        throw;
    }

    locks_[index].clear(std::memory_order_release);
}

void* CentralCache::fetch_from_page_cache(size_t size){
    //1 计算实际需要的页数
    size_t num_pages = (size + PageCache::PAGE_SIZE - 1) / PageCache::PAGE_SIZE;
    //2 根据大小决定分配策略
    if (size <= SPAN_PAGES * PageCache::PAGE_SIZE){
        //小于等于32KB的请求，使用固定8页
        return PageCache::getInstance().allocate_span(SPAN_PAGES);
    } else {
        //大于32KB的请求，按实际需求分配
        return PageCache::getInstance().allocate_span(num_pages);
    }
}

}//namespace memoryPool