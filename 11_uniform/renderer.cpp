#include "renderer.hpp"
#include "context.h"
#include "uniform.hpp"

namespace toy2d {
    // 顶点设置
    static std::array<Vertex, 3> kVertices = {
        Vertex{0.0, -0.5} ,
        Vertex{0.5, 0.5},
        Vertex{-0.5, 0.5}
    };
    static const Uniform kUniform{ Color{1, 0, 0} };


    Renderer::Renderer(int maxFlightCount) :m_maxFlightCount(maxFlightCount), m_curFrame(0)
    {
        createSems();
        createFence();
        CreateCmdBuffer();
        createVertexBuffer();
        bufferVertexData();
    }

    Renderer::~Renderer() {
        m_hostVertexBuffer.reset();
        m_deviceVertexBuffer.reset();

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

    void Renderer::copyBuffer(vk::Buffer & src, vk::Buffer& dst, size_t size, size_t srcOffset, size_t dstOffset) {
        auto cmdBuffer = Context::GetInstance().m_commandManager->CreateOneCommandBuffer();

        vk::CommandBufferBeginInfo cmdbeginInfo;
        cmdbeginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit); // 这里设置为每次重置
        cmdBuffer.begin(cmdbeginInfo); {
            vk::BufferCopy region;
            region.setSize(size)
                .setSrcOffset(srcOffset)
                .setDstOffset(dstOffset);
            cmdBuffer.copyBuffer(src, dst, region);
        }
        cmdBuffer.end();

        vk::SubmitInfo submitInfo;
        submitInfo.setCommandBuffers(cmdBuffer);
        Context::GetInstance().m_graphicsQueue.submit(submitInfo);

        Context::GetInstance().GetDevice().waitIdle(); // 这里没有用信号，直接粗暴等待

        Context::GetInstance().m_commandManager->FreeCmd(cmdBuffer);
    }

    void Renderer::createUniformBuffer() {
        m_hostUniformBuffers.resize(m_maxFlightCount);
        m_deviceUniformBuffers.resize(m_maxFlightCount);

        for (auto& buffer : m_hostUniformBuffers) {
            // CPU
            buffer.reset(new Buffer(sizeof(kUniform),
                vk::BufferUsageFlagBits::eTransferSrc,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));
        }

        for (auto& buffer : m_deviceUniformBuffers) {
            // GPU
            buffer.reset(new Buffer(sizeof(kUniform),
                vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer,
                vk::MemoryPropertyFlagBits::eDeviceLocal));
        }
    }

    void Renderer::bufferUniformData() {
        auto& device = Context::GetInstance().GetDevice();

        for (int i = 0; i < m_hostUniformBuffers.size(); ++i) {
            auto& host_buffer = m_hostUniformBuffers[i];

            // 传输到 GPU
            void* ptr = device.mapMemory(host_buffer->m_memory, 0, host_buffer->m_size);
            {
                memcpy(ptr, &kUniform, sizeof(kUniform));
            }
            device.unmapMemory(host_buffer->m_memory);

            copyBuffer(host_buffer->m_buffer, m_deviceUniformBuffers[i]->m_buffer, host_buffer->m_size, 0, 0);
        }
    }

    void Renderer::createVertexBuffer() {
        // CPU
        m_hostVertexBuffer.reset(new Buffer(sizeof(kVertices),
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));

        // GPU
        m_deviceVertexBuffer.reset(new Buffer(sizeof(kVertices),
            vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
            vk::MemoryPropertyFlagBits::eDeviceLocal));
    }

    void Renderer::bufferVertexData() {
        // 传输到 GPU
        auto& device = Context::GetInstance().GetDevice();
        void* ptr = device.mapMemory(m_hostVertexBuffer->m_memory, 0, m_hostVertexBuffer->m_size);
        {
            memcpy(ptr, kVertices.data(), sizeof(kVertices));
        }
        device.unmapMemory(m_hostVertexBuffer->m_memory);

        copyBuffer(m_hostVertexBuffer->m_buffer, m_deviceVertexBuffer->m_buffer, m_hostVertexBuffer->m_size, 0, 0);
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
        // device.acquireNextImage2KHR
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
                static vk::DeviceSize offset = 0;
                m_cmdBuffers[m_curFrame].bindVertexBuffers(0, m_deviceVertexBuffer->m_buffer, offset);
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

        // vk::SubmitInfo submitInfo;
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
