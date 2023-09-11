#include "render_process.hpp"
#include "shader.hpp"
#include "context.h"
#include "swapchain.h"

namespace toy2d {
    Render_process::Render_process(/* args */)
    {
    }

    Render_process::~Render_process()
    {
        DestroyRenderPass();
        DestroyLayout();
        DestroyPipeline();
    }

    void Render_process::InitPipeline(const int width, const int height)
    {
        vk::GraphicsPipelineCreateInfo createInfo;

        // 以下为渲染管线的流程

        // 1.vertex input
        vk::PipelineVertexInputStateCreateInfo inputStateInfo;
        createInfo.setPVertexInputState(&inputStateInfo);

        // 2.Vertex Assembly 图元设置
        vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
        inputAssemblyInfo.setPrimitiveRestartEnable(false)
            .setTopology(vk::PrimitiveTopology::eTriangleList);
        createInfo.setPInputAssemblyState(&inputAssemblyInfo);

        // 3.shader
        auto stages = Shader::GetInstance().GetStage(); // 这里需要拷贝，提高兼容性
        createInfo.setStages(stages);

        // 4.viewport
        vk::PipelineViewportStateCreateInfo viewportState;
        vk::Viewport viewport(0, 0, width, height, 0, 1); // 多个viewport是否支持需要，查询，有些电脑不支持
        vk::Rect2D rect{ {0,0},{static_cast<uint32_t>(width), static_cast<uint32_t>(height)} };
        viewportState.setViewports(viewport).setScissors(rect);
        createInfo.setPViewportState(&viewportState);

        // 5.光栅化
        vk::PipelineRasterizationStateCreateInfo rastInfo;
        rastInfo.setRasterizerDiscardEnable(false)
            .setCullMode(vk::CullModeFlagBits::eBack)
            .setFrontFace(vk::FrontFace::eCounterClockwise)
            .setPolygonMode(vk::PolygonMode::eFill)
            .setLineWidth(1);
        createInfo.setPRasterizationState(&rastInfo);

        // 6.multisample
        vk::PipelineMultisampleStateCreateInfo multiInfo;
        // 这里关闭 multisample, 但是还是需要指定setRasterizationSamples，需要对点进行采样
        multiInfo.setSampleShadingEnable(false)
            .setRasterizationSamples(vk::SampleCountFlagBits::e1);
        createInfo.setPMultisampleState(&multiInfo);

        // 7.Test stencil test, depth test
        // 暂时不用，跳过


        // 8.color Blending, 暂不开启
        vk::PipelineColorBlendStateCreateInfo colorBlendInfo;
        vk::PipelineColorBlendAttachmentState attachState;
        attachState.setBlendEnable(false)
            .setColorWriteMask(vk::ColorComponentFlagBits::eA |
                vk::ColorComponentFlagBits::eB |
                vk::ColorComponentFlagBits::eG |
                vk::ColorComponentFlagBits::eR);

        colorBlendInfo.setLogicOpEnable(false)
            .setAttachments(attachState);
        createInfo.setPColorBlendState(&colorBlendInfo);

        // 9.renderPass, Layout
        createInfo.setRenderPass(m_renderPass)
            .setLayout(m_layout);

        auto res = Context::GetInstance().GetDevice().createGraphicsPipeline(nullptr, createInfo);
        if (res.result != vk::Result::eSuccess) {
            throw std::runtime_error("create graphics failed!");
        }

        m_pipeline = res.value;
    }

    void Render_process::DestroyPipeline()
    {
        Context::GetInstance().GetDevice().destroyPipeline(m_pipeline);
    }

    void Render_process::InitLayout()
    {
        // 设置 uniform 的布局
        vk::PipelineLayoutCreateInfo layoutInfo;
        m_layout = Context::GetInstance().GetDevice().createPipelineLayout(layoutInfo);
    }

    void Render_process::DestroyLayout()
    {
        auto& device = Context::GetInstance().GetDevice();
        device.destroyPipelineLayout(m_layout);
    }

    void Render_process::InitRenderPass()
    {
        vk::RenderPassCreateInfo renderPassInfo;

        vk::AttachmentDescription attachDesc;
        attachDesc.setFormat(Context::GetInstance().m_swapchain->m_swapchainInfo.format.format)
            .setInitialLayout(vk::ImageLayout::eUndefined)
            .setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal)
            .setLoadOp(vk::AttachmentLoadOp::eClear)
            .setStoreOp(vk::AttachmentStoreOp::eStore)
            .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
            .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
            .setSamples(vk::SampleCountFlagBits::e1);
        renderPassInfo.setAttachments(attachDesc);

        // 这里使用一个 subpass
        vk::SubpassDescription subpassDesc;
        vk::AttachmentReference attachRef;
        attachRef.setLayout(vk::ImageLayout::eColorAttachmentOptimal)
            .setAttachment(0);
        subpassDesc.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
            .setColorAttachments(attachRef);
        renderPassInfo.setSubpasses(subpassDesc);

        // initsubpass -> subpass1 -> subpass2 -> ...
        vk::SubpassDependency dependency;
        dependency.setSrcSubpass(VK_SUBPASS_EXTERNAL)
            .setDstSubpass(0)
            .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
            .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
            .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
        renderPassInfo.setDependencies(dependency);

        m_renderPass = Context::GetInstance().GetDevice().createRenderPass(renderPassInfo);
    }


    void Render_process::DestroyRenderPass()
    {
        auto& device = Context::GetInstance().GetDevice();
        device.destroyRenderPass(m_renderPass);
    }

}
