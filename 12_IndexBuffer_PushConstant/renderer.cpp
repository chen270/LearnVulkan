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
    static const  Color kColor{0, 1, 0} ;


    Renderer::Renderer(int maxFlightCount) :m_maxFlightCount(maxFlightCount), m_curFrame(0)
    {
        createSems();
        createFence();
        CreateCmdBuffer();
        createVertexBuffer();
        bufferVertexData();
        createColorBuffer();
        bufferColorData();
        createMVPBuffer();
        createDescriptorPool();
        allocateSets();
        updateSets();
        initMats();
    }

    Renderer::~Renderer() {
        m_hostVertexBuffer.reset();
        m_deviceVertexBuffer.reset();
        m_hostColorBuffers.clear();
        m_deviceColorBuffers.clear();
        m_hostMVPBuffers.clear();
        m_deviceMVPBuffers.clear();

        auto& device = Context::GetInstance().GetDevice();

        device.destroyDescriptorPool(descriptorPool1_);
        device.destroyDescriptorPool(descriptorPool2_);

        for (auto& i : m_cmdBuffers) {
            Context::GetInstance().m_commandManager->FreeCmd(i);
        }

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

    void Renderer::createColorBuffer() {
        m_hostColorBuffers.resize(m_maxFlightCount);
        m_deviceColorBuffers.resize(m_maxFlightCount);

        for (auto& buffer : m_hostColorBuffers) {
            // CPU
            buffer.reset(new Buffer(sizeof(Color),
                vk::BufferUsageFlagBits::eTransferSrc,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));
        }

        for (auto& buffer : m_deviceColorBuffers) {
            // GPU
            buffer.reset(new Buffer(sizeof(Color),
                vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer,
                vk::MemoryPropertyFlagBits::eDeviceLocal));
        }
    }

    void Renderer::bufferColorData() {
        auto& device = Context::GetInstance().GetDevice();

        for (int i = 0; i < m_hostColorBuffers.size(); ++i) {
            auto& host_buffer = m_hostColorBuffers[i];

            // 传输到 GPU
            memcpy(host_buffer->m_map, &kColor, sizeof(Color));

            copyBuffer(host_buffer->m_buffer, m_deviceColorBuffers[i]->m_buffer, host_buffer->m_size, 0, 0);
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
        memcpy(m_hostVertexBuffer->m_map, kVertices.data(), sizeof(kVertices));

        copyBuffer(m_hostVertexBuffer->m_buffer, m_deviceVertexBuffer->m_buffer, m_hostVertexBuffer->m_size, 0, 0);
    }

    void Renderer::DrawTriangle(const Rect& rect)
    {
        // 开始绘制三角形
        auto& device = Context::GetInstance().GetDevice();
        auto& _swapchain = Context::GetInstance().m_swapchain;
        auto& _render_process = Context::GetInstance().m_renderProcess;

        if (device.waitForFences(m_cmdFences[m_curFrame], true, std::numeric_limits<std::uint64_t>::max()) != vk::Result::eSuccess) {
            throw std::runtime_error("wait for fence failed");
        }
        device.resetFences(m_cmdFences[m_curFrame]);

        auto model = Mat4::CreateTranslate(rect.position).Mul(Mat4::CreateScale(rect.size));
        bufferMVPData(model);

        // 该接口会阻塞程序, 第二个参数为等待时间，这里设置为无限等待
        auto result = device.acquireNextImageKHR(_swapchain->m_swapchain, std::numeric_limits<uint64_t>::max(), m_imageAvaliables[m_curFrame]);
        // device.acquireNextImage2KHR
        if (result.result != vk::Result::eSuccess) {
            std::cout << "acquireNextImageKHR error" << std::endl;
        }

        // 拿到 image 下标
        auto imageIndex = result.value;
        auto& cmd = m_cmdBuffers[m_curFrame];

        // 清空命令buffer, 与之前代码
        // createInfo.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer); 对应
        // 否则需要清空整个pool
        cmd.reset();

        vk::CommandBufferBeginInfo cmdbeginInfo;
        cmdbeginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit); // 这里设置为每次重置
        cmd.begin(cmdbeginInfo);
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
            cmd.beginRenderPass(renderPassBeginInfo, {});
            {
                cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, _render_process->GetPipeline());
                static vk::DeviceSize offset = 0;
                cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                    Context::GetInstance().m_renderProcess->m_layout,
                    0, m_descriptorSets.first[m_curFrame], {});
                cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                    Context::GetInstance().m_renderProcess->m_layout,
                    1, m_descriptorSets.second[m_curFrame], {});
                cmd.bindVertexBuffers(0, m_deviceVertexBuffer->m_buffer, offset);
                cmd.draw(3, 1, 0, 0);
            }
            cmd.endRenderPass();
        }
        cmd.end();

        // 命令传入 GPU
        vk::PipelineStageFlags const pipe_stage_flags = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        vk::SubmitInfo submitInfo;
        submitInfo.setCommandBuffers(cmd)
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

    void Renderer::createDescriptorPool() {
        vk::DescriptorPoolCreateInfo createInfo;
        vk::DescriptorPoolSize poolSize;
        poolSize.setType(vk::DescriptorType::eUniformBuffer)
            .setDescriptorCount(m_maxFlightCount);

        std::vector<vk::DescriptorPoolSize> sizes(2, poolSize);
        createInfo.setMaxSets(m_maxFlightCount) // 创建个数
            .setPoolSizes(sizes); // 可以传递多个

        auto& device = Context::GetInstance().GetDevice();
        descriptorPool1_ = device.createDescriptorPool(createInfo);
        descriptorPool2_ = device.createDescriptorPool(createInfo);
    }

    void Renderer::allocateSets() {
        auto& device = Context::GetInstance().GetDevice();

        // 不需要销毁，因为是从 m_descriptorPool 里面创建的，会一起销毁
        std::vector layouts(m_maxFlightCount, Context::GetInstance().m_shader->GetDescriptorSetLayouts()[0]);
        vk::DescriptorSetAllocateInfo allocInfo;
        allocInfo.setDescriptorPool(descriptorPool1_)
            .setSetLayouts(layouts);
        m_descriptorSets.first = device.allocateDescriptorSets(allocInfo);

        layouts = std::vector(m_maxFlightCount, Context::GetInstance().m_shader->GetDescriptorSetLayouts()[1]);
        allocInfo.setDescriptorPool(descriptorPool2_)
            .setSetLayouts(layouts);
        m_descriptorSets.second = device.allocateDescriptorSets(allocInfo);
    }

    void Renderer::updateSets() {
        for (int i = 0; i < m_descriptorSets.first.size(); i++) {
            // bind MVP buffer
            vk::DescriptorBufferInfo bufferInfo1;
            bufferInfo1.setBuffer(m_deviceMVPBuffers[i]->m_buffer)
                .setOffset(0)
                .setRange(sizeof(float) * 4 * 4 * 3);

            std::vector<vk::WriteDescriptorSet> writeInfos(2);
            writeInfos[0].setBufferInfo(bufferInfo1)
                .setDstBinding(0)
                .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                .setDescriptorCount(1)
                .setDstArrayElement(0)
                .setDstSet(m_descriptorSets.first[i]);

            // bind Color buffer
            vk::DescriptorBufferInfo bufferInfo2;
            bufferInfo2.setBuffer(m_deviceColorBuffers[i]->m_buffer)
                .setOffset(0)
                .setRange(sizeof(float) * 3);

            writeInfos[1].setBufferInfo(bufferInfo2)
                .setDstBinding(0)
                .setDstArrayElement(0)
                .setDescriptorCount(1)
                .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                .setDstSet(m_descriptorSets.second[i]);

            Context::GetInstance().GetDevice().updateDescriptorSets(writeInfos, {});
        }
    }

    void Renderer::createMVPBuffer() {
        m_hostMVPBuffers.resize(m_maxFlightCount);
        m_deviceMVPBuffers.resize(m_maxFlightCount);

        for (auto& buffer : m_hostMVPBuffers) {
            // CPU
            buffer.reset(new Buffer(sizeof(MVP),
                vk::BufferUsageFlagBits::eTransferSrc,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));
        }

        for (auto& buffer : m_deviceMVPBuffers) {
            // GPU
            buffer.reset(new Buffer(sizeof(MVP),
                vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer,
                vk::MemoryPropertyFlagBits::eDeviceLocal));
        }
    }

    void Renderer::bufferMVPData(const Mat4& model) {
        MVP mvp;
        mvp.project = projectMat_;
        mvp.view = viewMat_;
        mvp.model = model;
        auto& device = Context::GetInstance().GetDevice();
        for (int i = 0; i < m_hostMVPBuffers.size(); i++) {
            auto& host_buffer = m_hostMVPBuffers[i];
            memcpy(host_buffer->m_map, (void*)&mvp, sizeof(mvp));
            copyBuffer(host_buffer->m_buffer, m_deviceMVPBuffers[i]->m_buffer, host_buffer->m_size, 0, 0);
        }
    }

    void Renderer::initMats() {
        viewMat_ = Mat4::CreateIdentity();
        projectMat_ = Mat4::CreateIdentity();
    }

    void Renderer::SetProject(int right, int left, int bottom, int top, int far, int near) {
        projectMat_ = Mat4::CreateOrtho(left, right, top, bottom, near, far);
    }
}
