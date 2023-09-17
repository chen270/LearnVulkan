#include "buffer.hpp"
#include "context.h"

namespace toy2d {
Buffer::Buffer(size_t size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags property) : m_size(size)
{
    createBuffer(size, usage);
    auto & memInfo = queryMemoryInfo(property);
    allocateMemory(memInfo);
    bindingMem2Buf();
}

Buffer::~Buffer()
{
    Context::GetInstance().GetDevice().freeMemory(m_memory);
    Context::GetInstance().GetDevice().destroyBuffer(m_buffer);
}

void Buffer::createBuffer(size_t size, vk::BufferUsageFlags usage)
{
    vk::BufferCreateInfo createInfo;
    createInfo.setSize(size)
        .setUsage(usage)
        .setSharingMode(vk::SharingMode::eExclusive);

    m_buffer = Context::GetInstance().GetDevice().createBuffer(createInfo);
}

void Buffer::bindingMem2Buf()
{
    Context::GetInstance().GetDevice().bindBufferMemory(m_buffer, m_memory, 0);
}

void Buffer::allocateMemory(const MemoryInfo& memInfo)
{
    vk::MemoryAllocateInfo allocInfo;
    allocInfo.setMemoryTypeIndex(memInfo.index)
        .setAllocationSize(memInfo.size);
    m_memory = Context::GetInstance().GetDevice().allocateMemory(allocInfo);
}

Buffer::MemoryInfo Buffer::queryMemoryInfo(vk::MemoryPropertyFlags property)
{
    MemoryInfo memInfol;
    auto requirements = Context::GetInstance().GetDevice().getBufferMemoryRequirements(m_buffer);
    memInfol.size = requirements.size;

    // 拿到内存类型，需要用到物理设别
    auto properties = Context::GetInstance().GetPhyDevice().getMemoryProperties();
    for (int i = 0; i < properties.memoryTypeCount; i++) {
        if (((1 << i) & requirements.memoryTypeBits) &&
            (properties.memoryTypes[i].propertyFlags & property)) {
            memInfol.index = i;
            break;
        }
    }

    return memInfol;
}


}
