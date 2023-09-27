#include "descriptor_manager.hpp"
#include "context.h"

namespace toy2d {

std::unique_ptr<DescriptorSetManager> DescriptorSetManager::m_instance = nullptr;

void DescriptorSetManager::Init(uint32_t maxFlight) {
    m_instance.reset(new DescriptorSetManager(maxFlight));
}

void DescriptorSetManager::Quit() {
    m_instance.reset();
}

DescriptorSetManager& DescriptorSetManager::GetInstance() {
    return *m_instance;
}

DescriptorSetManager::DescriptorSetManager(uint32_t maxFlight) : m_maxFlightCount(maxFlight) {
    createBufferDescriptorPool();
    createImageSetPool(); // 暂时定10个容量, 未写伸缩扩容
}

DescriptorSetManager::~DescriptorSetManager() {
    auto& device = Context::GetInstance().GetDevice();

    device.destroyDescriptorPool(bufferSetPool_.pool_);
    for (auto pool : fulledImageSetPool_) {
        device.destroyDescriptorPool(pool.pool_);
    }
    for (auto pool : avalibleImageSetPool_) {
        device.destroyDescriptorPool(pool.pool_);
    }
}

void DescriptorSetManager::createBufferDescriptorPool() {
    vk::DescriptorPoolCreateInfo createInfo;
    std::vector<vk::DescriptorPoolSize> poolSizes(1);
    poolSizes[0].setType(vk::DescriptorType::eUniformBuffer)
        .setDescriptorCount(m_maxFlightCount * 2);

    createInfo.setMaxSets(m_maxFlightCount) // 创建个数
        .setPoolSizes(poolSizes); // 可以传递多个

    auto& device = Context::GetInstance().GetDevice();
    auto pool = device.createDescriptorPool(createInfo);
    bufferSetPool_.pool_ = std::move(pool);
    bufferSetPool_.remainNum_ = m_maxFlightCount;
}

std::vector<DescriptorSetManager::SetInfo> DescriptorSetManager::allocBufferDescriptorSet(uint32_t num) {
    std::vector layouts(m_maxFlightCount, Context::GetInstance().m_shader->GetDescriptorSetLayouts()[0]);
    vk::DescriptorSetAllocateInfo allocInfo;
    allocInfo.setDescriptorPool(bufferSetPool_.pool_)
        .setDescriptorSetCount(num)
        .setSetLayouts(layouts);
    auto sets = Context::GetInstance().GetDevice().allocateDescriptorSets(allocInfo);

    std::vector<SetInfo> result(num);
    for (int i = 0; i < num; i++) {
        result[i].set = sets[i];
        result[i].pool = bufferSetPool_.pool_;
    }

    return result;
}

void DescriptorSetManager::createImageSetPool() {
    constexpr uint32_t MaxSetNum = 10;

    vk::DescriptorPoolSize size;
    size.setType(vk::DescriptorType::eCombinedImageSampler)
        .setDescriptorCount(MaxSetNum);
    vk::DescriptorPoolCreateInfo createInfo;
    createInfo.setMaxSets(MaxSetNum)
        .setPoolSizes(size)
        .setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);
    auto pool = Context::GetInstance().GetDevice().createDescriptorPool(createInfo);
    avalibleImageSetPool_.push_back({ pool, MaxSetNum });
}

DescriptorSetManager::SetInfo DescriptorSetManager::AllocImageSet() {
    std::vector<vk::DescriptorSetLayout> layouts{ Context::GetInstance().m_shader->GetDescriptorSetLayouts()[1] };
    vk::DescriptorSetAllocateInfo allocInfo;

    if (avalibleImageSetPool_.empty()) {
        throw std::runtime_error("avalibleImageSetPool_ is empty!");
    }
    auto& poolInfo = avalibleImageSetPool_.back();

    allocInfo.setDescriptorPool(poolInfo.pool_)
        .setDescriptorSetCount(1)
        .setSetLayouts(layouts);
    auto sets = Context::GetInstance().GetDevice().allocateDescriptorSets(allocInfo);

    SetInfo result;
    result.pool = poolInfo.pool_;
    result.set = sets[0];

    poolInfo.remainNum_ = std::max<int>(static_cast<int>(poolInfo.remainNum_) - sets.size(), 0);

    return result;
}

void DescriptorSetManager::FreeImageSet(const SetInfo& info) {
    auto it = std::find_if(fulledImageSetPool_.begin(), fulledImageSetPool_.end(),
        [&](const PoolInfo& poolInfo) {
        return poolInfo.pool_ == info.pool;
    });
    if (it != fulledImageSetPool_.end()) {
        it->remainNum_++;
        avalibleImageSetPool_.push_back(*it);
        fulledImageSetPool_.erase(it);
        return;
    }

    it = std::find_if(avalibleImageSetPool_.begin(), avalibleImageSetPool_.end(),
        [&](const PoolInfo& poolInfo) {
        return poolInfo.pool_ == info.pool;
    });
    if (it != avalibleImageSetPool_.end()) {
        it->remainNum_++;
        return;
    }
    Context::GetInstance().GetDevice().freeDescriptorSets(info.pool, info.set);
}


}
