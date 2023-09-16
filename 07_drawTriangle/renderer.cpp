#include "renderer.hpp"
#include "context.h"

namespace toy2d {
    Renderer::Renderer()
    {
        InitCmdPool();
        AllocateCmdBuffer();
    }

    Renderer::~Renderer() {
        auto& device = Context::GetInstance().GetDevice();
        device.freeCommandBuffers(m_cmdPool, m_cmdBuffer);
        device.destroyCommandPool(m_cmdPool);
    }

    void Renderer::AllocateCmdBuffer() {
        vk::CommandBufferAllocateInfo allocInfo;
        allocInfo.setCommandPool(m_cmdPool)
            .setCommandBufferCount(1)
            .setLevel(vk::CommandBufferLevel::ePrimary);

        m_cmdBuffer = Context::GetInstance().GetDevice().allocateCommandBuffers(allocInfo)[0];
    }

    void Renderer::InitCmdPool() {
        vk::CommandPoolCreateInfo createInfo;
        createInfo.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);

        m_cmdPool = Context::GetInstance().GetDevice().createCommandPool(createInfo);
    }
}
