#ifndef __RENDER_PROCESS_H__
#define __RENDER_PROCESS_H__

#include "vulkan/vulkan.hpp"

namespace toy2d {
    class Render_process final
    {
    public:
        Render_process(/* args */);
        ~Render_process();

        void InitLayout();
        void InitPipeline(const int width, const int height);
        void InitRenderPass();
        vk::RenderPass& GetRenderPass() { return m_renderPass; }
        vk::Pipeline& GetPipeline() { return m_pipeline; }
        vk::DescriptorSetLayout createSetLayout();

    private:
        vk::Pipeline m_pipeline;
        vk::PipelineLayout m_layout;
        vk::RenderPass m_renderPass;

        void DestroyPipeline();
        void DestroyLayout();
        void DestroyRenderPass();
    };

}

#endif // __RENDER_PROCESS_H__
