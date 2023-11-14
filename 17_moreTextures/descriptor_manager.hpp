#ifndef __DESCRIPTOR_MANAGER_H__
#define __DESCRIPTOR_MANAGER_H__

#include "vulkan/vulkan.hpp"
#include <memory>
#include <vector>

namespace toy2d {

class DescriptorSetManager final
{
public:
    struct SetInfo {
        vk::DescriptorSet set;
        vk::DescriptorPool pool;
    };

    static void Init(uint32_t maxFlight);
    static void Quit();
    static DescriptorSetManager& GetInstance();
    DescriptorSetManager(uint32_t maxFlight);
    ~DescriptorSetManager();
    std::vector<DescriptorSetManager::SetInfo> allocBufferDescriptorSet(uint32_t num);
    DescriptorSetManager::SetInfo AllocImageSet();
    void FreeImageSet(const SetInfo& info);

private:
    static std::unique_ptr<DescriptorSetManager>m_instance;

    int m_maxFlightCount;
    struct PoolInfo {
        vk::DescriptorPool pool_;
        uint32_t remainNum_;
    };
    PoolInfo bufferSetPool_;

    std::vector<PoolInfo> fulledImageSetPool_;
    std::vector<PoolInfo> avalibleImageSetPool_;

    void createBufferDescriptorPool();
    void createImageSetPool();
};


}

#endif // __DESCRIPTOR_MANAGER_H__
