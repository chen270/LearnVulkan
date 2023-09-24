﻿#ifndef __BUFFER_H__
#define __BUFFER_H__

#include "vulkan/vulkan.hpp"

namespace toy2d {
class Buffer final
{
public:
    Buffer(size_t size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags property);
    ~Buffer();

    vk::Buffer m_buffer;
    vk::DeviceMemory m_memory;
    size_t m_size;
    void* m_map;

private:
    struct MemoryInfo final {
        size_t size;
        uint32_t index;
    };

    void createBuffer(size_t size, vk::BufferUsageFlags usage);
    void allocateMemory(const MemoryInfo& memInfo);
    void bindingMem2Buf();
    MemoryInfo queryMemoryInfo(vk::MemoryPropertyFlags property);

public:
    static std::uint32_t QueryBufferMemTypeIndex(std::uint32_t type, vk::MemoryPropertyFlags flag);
};

}

#endif // __BUFFER_H__
