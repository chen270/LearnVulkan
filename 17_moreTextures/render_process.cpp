#include "render_process.hpp"
#include "shader.hpp"
#include "context.h"
#include "swapchain.h"
#include "uniform.hpp"

namespace toy2d {
    Render_process::Render_process(/* args */)
    {
        InitLayout();
        InitRenderPass();
        m_pipeline = nullptr;
    }

    Render_process::~Render_process()
    {
        DestroyRenderPass();
        DestroyLayout();
        DestroyPipeline();
    }

    void Render_process::RecreateGraphicsPipeline(const Shader& shader) {
        if (m_pipeline) {
            DestroyPipeline();
        }
        InitPipeline(shader);
    }

    void Render_process::InitPipeline(const Shader& shader)
    {
        const int width = Context::GetInstance().m_swapchain->GetExtent().width;
        const int height = Context::GetInstance().m_swapchain->GetExtent().height;

        vk::GraphicsPipelineCreateInfo createInfo;

        // 以下为渲染管线的流程

        // 1.vertex input
        auto attr = Vec::GetAttributeDescription();
        auto binding = Vec::GetBindingDescription();

        vk::PipelineVertexInputStateCreateInfo vertexInputCreateInfo;
        vertexInputCreateInfo.setVertexAttributeDescriptions(attr)
            .setVertexBindingDescriptions(binding);
        createInfo.setPVertexInputState(&vertexInputCreateInfo);

        // 2.Vertex Assembly 图元设置
        vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
        inputAssemblyInfo.setPrimitiveRestartEnable(false)
            .setTopology(vk::PrimitiveTopology::eTriangleList);
        createInfo.setPInputAssemblyState(&inputAssemblyInfo);

        // 3. shader prepare
        std::array<vk::PipelineShaderStageCreateInfo, 2> stageCreateInfos;
        stageCreateInfos[0].setModule(shader.GetVertexModule())
            .setPName("main")
            .setStage(vk::ShaderStageFlagBits::eVertex);
        stageCreateInfos[1].setModule(shader.GetFragModule())
            .setPName("main")
            .setStage(vk::ShaderStageFlagBits::eFragment);
        createInfo.setStages(stageCreateInfos);

        // 4.viewport
        vk::PipelineViewportStateCreateInfo viewportState;
        vk::Viewport viewport(0, 0, static_cast<float>(width), static_cast<float>(height), 0, 1); // 多个viewport是否支持需要，查询，有些电脑不支持
        vk::Rect2D rect{ {0,0},{static_cast<uint32_t>(width), static_cast<uint32_t>(height)} };
        viewportState.setViewports(viewport).setScissors(rect);
        createInfo.setPViewportState(&viewportState);

        // 5.光栅化
        vk::PipelineRasterizationStateCreateInfo rastInfo;
        rastInfo.setRasterizerDiscardEnable(false)
            .setCullMode(vk::CullModeFlagBits::eBack)
            .setFrontFace(vk::FrontFace::eClockwise)
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
#if 1
        // 公式法开启
        vk::PipelineColorBlendStateCreateInfo colorBlendInfo;
        vk::PipelineColorBlendAttachmentState blendAttachmentState;
        blendAttachmentState.setBlendEnable(true)
            .setColorWriteMask(vk::ColorComponentFlagBits::eA |
                vk::ColorComponentFlagBits::eB |
                vk::ColorComponentFlagBits::eG |
                vk::ColorComponentFlagBits::eR)
            .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
            .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
            .setColorBlendOp(vk::BlendOp::eAdd)
            .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
            .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
            .setAlphaBlendOp(vk::BlendOp::eAdd);


        colorBlendInfo.setLogicOpEnable(false)
            .setAttachments(blendAttachmentState);
        createInfo.setPColorBlendState(&colorBlendInfo);

#elif 0
        // 按位操作混合
        vk::PipelineColorBlendStateCreateInfo colorBlendInfo;
        vk::PipelineColorBlendAttachmentState blendAttachmentState;
        blendAttachmentState.setBlendEnable(true)
            .setColorWriteMask(vk::ColorComponentFlagBits::eA |
                vk::ColorComponentFlagBits::eB |
                vk::ColorComponentFlagBits::eG |
                vk::ColorComponentFlagBits::eR);

        colorBlendInfo.setLogicOpEnable(true)
            .setLogicOp(vk::LogicOp::eCopy)
            .setAttachments(blendAttachmentState);
        createInfo.setPColorBlendState(&colorBlendInfo);
#else
        // 不开启
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
#endif

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
        auto layouts = Context::GetInstance().m_shader->GetDescriptorSetLayouts();

        // 设置 uniform 的布局
        vk::PipelineLayoutCreateInfo layoutInfo;
        auto range = Context::GetInstance().m_shader->GetPushConstantRange();
        layoutInfo.setSetLayouts(layouts)
            .setPushConstantRanges(range);
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
            .setFinalLayout(vk::ImageLayout::ePresentSrcKHR)
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

    //vk::DescriptorSetLayout Render_process::createSetLayout() {
    //    vk::DescriptorSetLayoutCreateInfo createInfo;
    //    auto binding = Uniform::GetBinding();
    //    createInfo.setBindings(binding);

    //    return Context::GetInstance().GetDevice().createDescriptorSetLayout(createInfo);
    //}
}
