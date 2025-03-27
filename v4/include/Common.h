#ifndef __Common_HPP_
#define __Common_HPP_
#include <cstddef>
#include <atomic>
#include <array>

namespace memoryPool
{
//8字节对齐
constexpr size_t ALIGNMENT = 8;
constexpr size_t MAX_BYTES = 256 * 1024;
constexpr size_t FREE_LIST_SIZE = MAX_BYTES / ALIGNMENT;

//内存块头部信息
struct BlockHeader
{
    size_t size;
    bool in_use;
    BlockHeader* next;
};

//大小类管理
class SizeClass{
public:
    static size_t round_up(size_t bytes){
        return (bytes + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
    }

    static size_t get_index(size_t bytes){
        bytes = std::max(bytes, ALIGNMENT);
        return (bytes + ALIGNMENT - 1) / ALIGNMENT - 1;
    }
};
    
} // namespace memoryPool


#endif