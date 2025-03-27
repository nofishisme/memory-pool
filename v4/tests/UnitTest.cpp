#include "MemoryPool.h"
#include <iostream>
#include <vector>
#include <thread>
#include <cassert>
#include <cstring>
#include <random>
#include <algorithm>
#include <atomic>
#include <memory>

using namespace memoryPool;

// 基础分配测试
void testBasicAllocation() 
{
    std::cout << "Running basic allocation test..." << std::endl;
    
    // 测试小内存分配
    {
        auto ptr1 = std::shared_ptr<void>(MemoryPool::allocate(8), MemoryPoolDeleter(8));
    }
    // void* ptr1 = MemoryPool::allocate(8);
    // assert(ptr1 != nullptr);
    // MemoryPool::deallocate(ptr1, 8);

    // 测试中等大小内存分配
    {
        auto ptr1 = std::shared_ptr<void>(MemoryPool::allocate(1024), MemoryPoolDeleter(1024));
    }
    // void* ptr2 = MemoryPool::allocate(1024);
    // assert(ptr2 != nullptr);
    // MemoryPool::deallocate(ptr2, 1024);

    // 测试大内存分配（超过MAX_BYTES）
    {
        auto ptr1 = std::shared_ptr<void>(MemoryPool::allocate(8 * 1024), MemoryPoolDeleter(8 * 1024));
    }
    // void* ptr3 = MemoryPool::allocate(1024 * 1024);
    // assert(ptr3 != nullptr);
    // MemoryPool::deallocate(ptr3, 1024 * 1024);

    std::cout << "Basic allocation test passed!" << std::endl;
}

int main(){
    try{
        std::cout << "Starting memory pool tests..." << std::endl;
        testBasicAllocation();

        std::cout << "All tests passed successfully!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}