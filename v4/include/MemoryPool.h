#ifndef __MemoryPool_HPP__
#define __MemoryPool_HPP__
#include "ThreadCache.h"
#include <memory>
#include <iostream>
namespace memoryPool{

class MemoryPool{
public:
    static void* allocate(size_t size){
        return ThreadCache::getInstance()->allocate(size);
    }
    static void deallocate(void* ptr, size_t size){
        //std::cout << "MemoryPool::deallocate, size:" << size <<  std::endl;
        ThreadCache::getInstance()->deallocate(ptr, size);
    }
};

class MemoryPoolDeleter
{
public:
    MemoryPoolDeleter(size_t size):size_(size){

    }
    void operator()(void* p){
        MemoryPool::deallocate(p, size_);
    }
    size_t size_;
};

}//namespace memoryPool

#endif