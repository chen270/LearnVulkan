#ifndef __RENDER_PROCESS_H__
#define __RENDER_PROCESS_H__

#include "vulkan/vulkan.hpp"
#include "shader.hpp"

namespace toy2d {
    class Render_process final
    {
    public:
        Render_process(/* args */);
        ~Render_process();

        void InitPipeline(const Shader& shader);
        vk::RenderPass& GetRenderPass() { return m_renderPass; }
        vk::Pipeline& GetPipeline() { return m_pipeline; }
        //vk::DescriptorSetLayout createSetLayout();

        vk::PipelineLayout m_layout;

        void RecreateGraphicsPipeline(const Shader& shader);
    private:
        vk::Pipeline m_pipeline;
        vk::RenderPass m_renderPass;

        void InitLayout();
        void InitRenderPass();

        void DestroyPipeline();
        void DestroyLayout();
        void DestroyRenderPass();
    };

}

#endif // __RENDER_PROCESS_H__
