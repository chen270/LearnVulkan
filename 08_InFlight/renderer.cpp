#include "renderer.hpp"
#include "context.h"

namespace toy2d {
    Renderer::Renderer(int maxFlightCount) :m_maxFlightCount(maxFlightCount), m_curFrame(0)
    {
        createSems();
        createFence();
        CreateCmdBuffer();
    }

    Renderer::~Renderer() {
        for (auto& i : m_cmdBuffers) {
            Context::GetInstance().m_commandManager->FreeCmd(i);
        }

        auto& device = Context::GetInstance().GetDevice();
        for (auto& i : m_cmdFences) {
            device.destroyFence(i);
        }
        
        for (auto& i : m_imageAvaliables) {
            device.destroySemaphore(i);
        }
        
        for (auto& i : m_imageDrawFinishs) {
            device.destroySemaphore(i);
        }
    }

    void Renderer::CreateCmdBuffer() {
        m_cmdBuffers.resize(m_maxFlightCount);

        for (auto& cmd : m_cmdBuffers) {
            cmd = Context::GetInstance().m_commandManager->CreateOneCommandBuffer();
        }
    }

    void Renderer::createSems() {
        vk::SemaphoreCreateInfo createInfo;

        m_imageAvaliables.resize(m_maxFlightCount);
        for (auto& i : m_imageAvaliables) {
            i = Context::GetInstance().GetDevice().createSemaphore(createInfo);
        }

        m_imageDrawFinishs.resize(m_maxFlightCount);
        for (auto& i : m_imageDrawFinishs) {
            i = Context::GetInstance().GetDevice().createSemaphore(createInfo);
        }
    }

    void Renderer::createFence() {
        m_cmdFences.resize(m_maxFlightCount, nullptr);

        for (auto& fence : m_cmdFences) {
            vk::FenceCreateInfo fenceCreateInfo;
            fenceCreateInfo.setFlags(vk::FenceCreateFlagBits::eSignaled);
            fence = Context::GetInstance().GetDevice().createFence(fenceCreateInfo);
        }
    }

    void Renderer::DrawTriangle()
    {
        // 开始绘制三角形
        auto& device = Context::GetInstance().GetDevice();
        auto& _swapchain = Context::GetInstance().m_swapchain;
        auto& _render_process = Context::GetInstance().m_renderProcess;

        if (device.waitForFences(m_cmdFences[m_curFrame], true, std::numeric_limits<std::uint64_t>::max()) != vk::Result::eSuccess) {
            throw std::runtime_error("wait for fence failed");
        }
        device.resetFences(m_cmdFences[m_curFrame]);

        // 该接口会阻塞程序, 第二个参数为等待时间，这里设置为无限等待
        auto result = device.acquireNextImageKHR(_swapchain->m_swapchain, std::numeric_limits<uint64_t>::max(), m_imageAvaliables[m_curFrame]);
        //device.acquireNextImage2KHR
        if (result.result != vk::Result::eSuccess) {
            std::cout << "acquireNextImageKHR error" << std::endl;
        }
        
        // 拿到 image 下标
        auto imageIndex = result.value;

        // 清空命令buffer, 与之前代码 
        // createInfo.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer); 对应
        // 否则需要清空整个pool
        m_cmdBuffers[m_curFrame].reset();

        vk::CommandBufferBeginInfo cmdbeginInfo;
        cmdbeginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit); // 这里设置为每次重置
        m_cmdBuffers[m_curFrame].begin(cmdbeginInfo);
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
            m_cmdBuffers[m_curFrame].beginRenderPass(renderPassBeginInfo, {});
            {
                m_cmdBuffers[m_curFrame].bindPipeline(vk::PipelineBindPoint::eGraphics, _render_process->GetPipeline());
                m_cmdBuffers[m_curFrame].draw(3, 1, 0, 0);
            }
            m_cmdBuffers[m_curFrame].endRenderPass();
        }
        m_cmdBuffers[m_curFrame].end();

        // 命令传入 GPU
        vk::PipelineStageFlags const pipe_stage_flags = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        vk::SubmitInfo submitInfo;
        submitInfo.setCommandBuffers(m_cmdBuffers[m_curFrame])
            .setWaitSemaphores(m_imageAvaliables[m_curFrame])
            .setSignalSemaphores(m_imageDrawFinishs[m_curFrame])
            .setWaitDstStageMask(pipe_stage_flags);

        //vk::SubmitInfo submitInfo;
        Context::GetInstance().m_graphicsQueue.submit(submitInfo, m_cmdFences[m_curFrame]);

        // 开始显示
        vk::PresentInfoKHR presentInfo;
        presentInfo.setImageIndices(imageIndex)
            .setSwapchains(_swapchain->m_swapchain)
            .setWaitSemaphores(m_imageDrawFinishs[m_curFrame]);
        if (Context::GetInstance().m_presentQueue.presentKHR(presentInfo) != vk::Result::eSuccess) {
            std::cout << "presentKHR error" << std::endl;
        }

        m_curFrame = (m_curFrame + 1) % m_maxFlightCount;
    }
}
