#include "command_manager.hpp"
#include "context.h"

namespace toy2d {

CommandManager::CommandManager() {
    m_pool = createCommandPool();
}

CommandManager::~CommandManager() {
    auto& device = Context::GetInstance().GetDevice();
    device.destroyCommandPool(m_pool);
}

void CommandManager::ResetCmds() {
    Context::GetInstance().GetDevice().resetCommandPool(m_pool);
}

vk::CommandPool CommandManager::createCommandPool() {
    auto& ctx = Context::GetInstance();

    vk::CommandPoolCreateInfo createInfo;

    createInfo.setQueueFamilyIndex(ctx.GetQueueFamilyIndices().grapghicsQueue.value())
              .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);

    return ctx.GetDevice().createCommandPool(createInfo);
}

std::vector<vk::CommandBuffer> CommandManager::CreateCommandBuffers(std::uint32_t count) {
    vk::CommandBufferAllocateInfo allocInfo;
    allocInfo.setCommandPool(m_pool)
             .setCommandBufferCount(1)
             .setLevel(vk::CommandBufferLevel::ePrimary);

    return Context::GetInstance().GetDevice().allocateCommandBuffers(allocInfo);
}

vk::CommandBuffer CommandManager::CreateOneCommandBuffer() {
    return CreateCommandBuffers(1)[0];
}

void CommandManager::FreeCmd(vk::CommandBuffer buf) {
    auto& device = Context::GetInstance().GetDevice();
    device.freeCommandBuffers(m_pool, buf);
}

void CommandManager::ExecuteCmd(vk::Queue queue, RecordCmdFunc func) {
    auto cmdBuf = CreateOneCommandBuffer();

    vk::CommandBufferBeginInfo beginInfo;
    beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    cmdBuf.begin(beginInfo);
    if (func) func(cmdBuf);
    cmdBuf.end();

    vk::SubmitInfo submitInfo;
    submitInfo.setCommandBuffers(cmdBuf);
    queue.submit(submitInfo);
    queue.waitIdle();
    Context::GetInstance().GetDevice().waitIdle();
    FreeCmd(cmdBuf);
}

}
