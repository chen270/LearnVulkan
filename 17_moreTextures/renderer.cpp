#include "renderer.hpp"
#include "context.h"
#include "uniform.hpp"

namespace toy2d {
    // 顶点设置
    static std::array<Vertex, 4> kVertices = {
        Vertex{Vec{-0.5, -0.5},Vec{0, 0}},
        Vertex{Vec{0.5, -0.5} ,Vec{1, 0}},
        Vertex{Vec{0.5, 0.5}  ,Vec{1, 1}},
        Vertex{Vec{-0.5, 0.5} ,Vec{0, 1}},
    };

    std::uint32_t kIndices[] = {
        0, 1, 3,
        1, 2, 3,
    };
    static const  Color kColor{0, 1, 0} ;


    Renderer::Renderer(int maxFlightCount) :m_maxFlightCount(maxFlightCount), m_curFrame(0)
    {
        createSems();
        createFence();
        CreateCmdBuffer();
        createVertexBuffer();
        bufferVertexData();
        createIndexBuffer();
        bufferIndexData();
        createColorBuffer();
        SetDrawColor(kColor);

        // mvp
        createMVPBuffer();
        initMats();

        createSampler();

        descriptorSets_ = DescriptorSetManager::GetInstance().allocBufferDescriptorSet(m_maxFlightCount);
        updateBufferSets();
    }

    Renderer::~Renderer() {
        m_hostVertexBuffer.reset();
        m_deviceVertexBuffer.reset();
        m_hostIndexBuffer.reset();
        m_deviceIndexBuffer.reset();
        m_hostColorBuffers.clear();
        m_deviceColorBuffers.clear();
        m_hostMVPBuffers.clear();
        m_deviceMVPBuffers.clear();

        auto& device = Context::GetInstance().GetDevice();

        device.destroySampler(m_sampler);

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

    void Renderer::DrawTexture(const Rect& rect, Texture& texture) {
        auto& ctx = Context::GetInstance();
        auto& device = ctx.GetDevice();
        auto& cmd = m_cmdBuffers[m_curFrame];
        vk::DeviceSize offset = 0;
        cmd.bindVertexBuffers(0, m_deviceVertexBuffer->m_buffer, offset);
        cmd.bindIndexBuffer(m_deviceIndexBuffer->m_buffer, 0, vk::IndexType::eUint32);

        auto& layout = Context::GetInstance().m_renderProcess->m_layout;
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
            layout,
            0, { descriptorSets_[m_curFrame].set, texture.m_setInfo.set }, {});
        auto model = Mat4::CreateTranslate(rect.position).Mul(Mat4::CreateScale(rect.size));
        cmd.pushConstants(layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(Mat4), model.GetData());
        cmd.drawIndexed(6, 1, 0, 0, 0);
    }

    void Renderer::StartRender() {
        auto& ctx = Context::GetInstance();
        auto& device = ctx.GetDevice();
        if (device.waitForFences(m_cmdFences[m_curFrame], true, std::numeric_limits<std::uint64_t>::max()) != vk::Result::eSuccess) {
            throw std::runtime_error("wait for fence failed");
        }
        device.resetFences(m_cmdFences[m_curFrame]);

        auto& swapchain = ctx.m_swapchain;
        auto resultValue = device.acquireNextImageKHR(swapchain->m_swapchain, std::numeric_limits<std::uint64_t>::max(), m_imageAvaliables[m_curFrame], nullptr);
        if (resultValue.result != vk::Result::eSuccess) {
            throw std::runtime_error("wait for image in swapchain failed");
        }
        m_imageIndex = resultValue.value;

        auto& cmdMgr = ctx.m_commandManager;
        auto& cmd = m_cmdBuffers[m_curFrame];
        cmd.reset();

        vk::CommandBufferBeginInfo beginInfo;
        beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        cmd.begin(beginInfo);
        vk::ClearValue clearValue;
        clearValue.setColor(vk::ClearColorValue(std::array<float, 4>{0.1, 0.1, 0.1, 1}));
        vk::RenderPassBeginInfo renderPassBegin;
        renderPassBegin.setRenderPass(ctx.m_renderProcess->GetRenderPass())
            .setFramebuffer(swapchain->m_framebuffers[m_imageIndex])
            .setClearValues(clearValue)
            .setRenderArea(vk::Rect2D({}, swapchain->GetExtent()));
        cmd.beginRenderPass(&renderPassBegin, vk::SubpassContents::eInline);
        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, ctx.m_renderProcess->GetPipeline());
    }

    void Renderer::EndRender() {
        auto& ctx = Context::GetInstance();
        auto& swapchain = ctx.m_swapchain;
        auto& cmd = m_cmdBuffers[m_curFrame];
        cmd.endRenderPass();
        cmd.end();

        vk::SubmitInfo submit;
        vk::PipelineStageFlags flags = vk::PipelineStageFlagBits::eColorAttachmentOutput;

        submit.setCommandBuffers(cmd)
            .setWaitSemaphores(m_imageAvaliables[m_curFrame])
            .setSignalSemaphores(m_imageDrawFinishs[m_curFrame])
            .setWaitDstStageMask(flags);
        ctx.m_graphicsQueue.submit(submit, m_cmdFences[m_curFrame]);

        vk::PresentInfoKHR presentInfo;
        presentInfo.setImageIndices(m_imageIndex)
            .setSwapchains(swapchain->m_swapchain)
            .setWaitSemaphores(m_imageDrawFinishs[m_curFrame]);
        if (ctx.m_presentQueue.presentKHR(presentInfo) != vk::Result::eSuccess) {
            throw std::runtime_error("present queue execute failed");
        }

        m_curFrame = (m_curFrame + 1) % m_maxFlightCount;
    }

    void Renderer::createFence() {
        m_cmdFences.resize(m_maxFlightCount, nullptr);

        for (auto& fence : m_cmdFences) {
            vk::FenceCreateInfo fenceCreateInfo;
            fenceCreateInfo.setFlags(vk::FenceCreateFlagBits::eSignaled);
            fence = Context::GetInstance().GetDevice().createFence(fenceCreateInfo);
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


    void Renderer::CreateCmdBuffer() {
        m_cmdBuffers.resize(m_maxFlightCount);

        for (auto& cmd : m_cmdBuffers) {
            cmd = Context::GetInstance().m_commandManager->CreateOneCommandBuffer();
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

    void Renderer::createIndexBuffer() {
        // CPU
        m_hostIndexBuffer.reset(new Buffer(sizeof(kIndices),
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));

        // GPU
        m_deviceIndexBuffer.reset(new Buffer(sizeof(kIndices),
            vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
            vk::MemoryPropertyFlagBits::eDeviceLocal));
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

    void Renderer::SetDrawColor(Color color) {
        auto& device = Context::GetInstance().GetDevice();

        for (int i = 0; i < m_hostColorBuffers.size(); ++i) {
            auto& host_buffer = m_hostColorBuffers[i];

            // 传输到 GPU
            memcpy(host_buffer->m_map, &color, sizeof(Color));

            copyBuffer(host_buffer->m_buffer, m_deviceColorBuffers[i]->m_buffer, host_buffer->m_size, 0, 0);
        }
    }

    void Renderer::bufferVertexData() {
        // 传输到 GPU
        auto& device = Context::GetInstance().GetDevice();
        memcpy(m_hostVertexBuffer->m_map, kVertices.data(), sizeof(kVertices));

        copyBuffer(m_hostVertexBuffer->m_buffer, m_deviceVertexBuffer->m_buffer, m_hostVertexBuffer->m_size, 0, 0);
    }

    void Renderer::bufferIndexData() {
        // 传输到 GPU
        auto& device = Context::GetInstance().GetDevice();
        memcpy(m_hostIndexBuffer->m_map, kIndices, sizeof(kIndices));

        copyBuffer(m_hostIndexBuffer->m_buffer, m_deviceIndexBuffer->m_buffer, m_hostIndexBuffer->m_size, 0, 0);
    }

    void Renderer::DrawRect(const Rect& rect)
    {
        // 开始绘制三角形
        auto& device = Context::GetInstance().GetDevice();
        auto& _swapchain = Context::GetInstance().m_swapchain;
        auto& _render_process = Context::GetInstance().m_renderProcess;

        if (device.waitForFences(m_cmdFences[m_curFrame], true, std::numeric_limits<std::uint64_t>::max()) != vk::Result::eSuccess) {
            throw std::runtime_error("wait for fence failed");
        }
        device.resetFences(m_cmdFences[m_curFrame]);

        //auto model = Mat4::CreateTranslate(rect.position).Mul(Mat4::CreateScale(rect.size));
        //bufferMVPData(model);

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
                cmd.bindVertexBuffers(0, m_deviceVertexBuffer->m_buffer, offset);
                cmd.bindIndexBuffer(m_deviceIndexBuffer->m_buffer, 0, vk::IndexType::eUint32);

                auto& layout = Context::GetInstance().m_renderProcess->m_layout;
                cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                    layout,
                    0, descriptorSets_[m_curFrame].set, {});

                auto model = Mat4::CreateTranslate(rect.position).Mul(Mat4::CreateScale(rect.size));
                cmd.pushConstants(layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(Mat4), model.GetData());

                cmd.drawIndexed(6, 1, 0, 0, 0);
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

    void Renderer::updateBufferSets() {
        for (int i = 0; i < descriptorSets_.size(); i++) {
            // bind MVP buffer
            vk::DescriptorBufferInfo bufferInfo1;
            bufferInfo1.setBuffer(m_deviceMVPBuffers[i]->m_buffer)
                .setOffset(0)
                .setRange(sizeof(Mat4) * 2); // 改成 2 个矩阵大小

            std::vector<vk::WriteDescriptorSet> writeInfos(2);
            writeInfos[0].setBufferInfo(bufferInfo1)
                .setDstBinding(0)
                .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                .setDescriptorCount(1)
                .setDstArrayElement(0)
                .setDstSet(descriptorSets_[i].set);

            // bind Color buffer
            vk::DescriptorBufferInfo bufferInfo2;
            bufferInfo2.setBuffer(m_deviceColorBuffers[i]->m_buffer)
                .setOffset(0)
                .setRange(sizeof(Color));

            writeInfos[1].setBufferInfo(bufferInfo2)
                .setDstBinding(1) // 根据shader中 bind 修改为1
                .setDstArrayElement(0)
                .setDescriptorCount(1)
                .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                .setDstSet(descriptorSets_[i].set);

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

    void Renderer::bufferMVPData(/*const Mat4& model*/) {
        MVP mvp;
        mvp.project = projectMat_;
        mvp.view = viewMat_;
        //mvp.model = model;
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
        bufferMVPData();
    }

    void Renderer::createSampler() {
        vk::SamplerCreateInfo createInfo;
        createInfo.setMagFilter(vk::Filter::eLinear)
            .setMinFilter(vk::Filter::eLinear)
            .setAddressModeU(vk::SamplerAddressMode::eRepeat)
            .setAddressModeV(vk::SamplerAddressMode::eRepeat)
            .setAddressModeW(vk::SamplerAddressMode::eRepeat)
            .setAnisotropyEnable(false)
            .setBorderColor(vk::BorderColor::eIntOpaqueBlack)
            .setUnnormalizedCoordinates(false)
            .setCompareEnable(false)
            .setMipmapMode(vk::SamplerMipmapMode::eLinear);
        m_sampler = Context::GetInstance().GetDevice().createSampler(createInfo);
    }

}
