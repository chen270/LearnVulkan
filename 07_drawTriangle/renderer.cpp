#include "renderer.hpp"
#include "context.h"

namespace toy2d {
    Renderer::Renderer()
    {
        InitCmdPool();
        AllocateCmdBuffer();
        createSems();
        createFence();
    }

    Renderer::~Renderer() {
        auto& device = Context::GetInstance().GetDevice();
        device.freeCommandBuffers(m_cmdPool, m_cmdBuffer);
        device.destroyCommandPool(m_cmdPool);

        device.destroyFence(m_cmdFence);
        device.destroySemaphore(m_imageAvaliable);
        device.destroySemaphore(m_imageDrawFinish);
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

    void Renderer::createSems() {
        vk::SemaphoreCreateInfo createInfo;

        m_imageAvaliable = Context::GetInstance().GetDevice().createSemaphore(createInfo);
        m_imageDrawFinish = Context::GetInstance().GetDevice().createSemaphore(createInfo);
    }

    void Renderer::createFence() {
        vk::FenceCreateInfo createInfo;
        m_cmdFence = Context::GetInstance().GetDevice().createFence(createInfo);
    }

    void Renderer::Render()
    {
        // 开始绘制三角形
        auto& device = Context::GetInstance().GetDevice();
        auto& _swapchain = Context::GetInstance().m_swapchain;
        auto& _render_process = Context::GetInstance().m_renderProcess;

        // 该接口会阻塞程序, 第二个参数为等待时间，这里设置为无限等待
        auto result = device.acquireNextImageKHR(_swapchain->m_swapchain, std::numeric_limits<uint64_t>::max(), m_imageAvaliable);
        //device.acquireNextImage2KHR
        if (result.result != vk::Result::eSuccess) {
            std::cout << "acquireNextImageKHR error" << std::endl;
        }
        
        // 拿到 image 下标
        auto imageIndex = result.value;

        // 清空命令buffer, 与之前代码 
        // createInfo.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer); 对应
        // 否则需要清空整个pool
        m_cmdBuffer.reset();

        vk::CommandBufferBeginInfo cmdbeginInfo;
        cmdbeginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit); // 这里设置为每次重置
        m_cmdBuffer.begin(cmdbeginInfo);
        {
            vk::RenderPassBeginInfo renderPassBeginInfo;
            vk::Rect2D area;
            area.setOffset({ 0, 0 }).setExtent(_swapchain->m_swapchainInfo.imageExtent);

            vk::ClearValue clearValue; // 可以设置 0-255 uint32, 也可以设置 0~1 float
            clearValue.color = vk::ClearColorValue(std::array < float, 4>{0.1f, 0.1f, 1.0f, 1.0f});

            renderPassBeginInfo.setRenderPass(_render_process->GetRenderPass())
                .setRenderArea(area)
                .setFramebuffer(_swapchain->m_framebuffers[imageIndex])
                .setClearValues(clearValue);
            m_cmdBuffer.beginRenderPass(renderPassBeginInfo, {});
            {
                m_cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _render_process->GetPipeline());
                m_cmdBuffer.draw(3, 1, 0, 0);
            }
            m_cmdBuffer.endRenderPass();
        }
        m_cmdBuffer.end();

        // 命令传入 GPU
        vk::PipelineStageFlags const pipe_stage_flags = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        vk::SubmitInfo submitInfo;
        submitInfo.setCommandBuffers(m_cmdBuffer)
            .setWaitSemaphores(m_imageAvaliable)
            .setSignalSemaphores(m_imageDrawFinish)
            .setWaitDstStageMask(pipe_stage_flags);

        //vk::SubmitInfo submitInfo;
        Context::GetInstance().m_graphicsQueue.submit(submitInfo, m_cmdFence);

        // 开始显示
        vk::PresentInfoKHR presentInfo;
        presentInfo.setImageIndices(imageIndex)
            .setSwapchains(_swapchain->m_swapchain)
            .setWaitSemaphores(m_imageDrawFinish);
        if (Context::GetInstance().m_presentQueue.presentKHR(presentInfo) != vk::Result::eSuccess) {
            std::cout << "presentKHR error" << std::endl;
        }

        if (device.waitForFences(m_cmdFence, true, std::numeric_limits<uint64_t>::max()) != vk::Result::eSuccess) {
            std::cout << "wait for fence failed" << std::endl;
        }

        device.resetFences(m_cmdFence);
    }
}
