#ifndef __RENDER_PROCESS_H__
#define __RENDER_PROCESS_H__

#include "vulkan/vulkan.hpp"

namespace toy2d {
    class Render_process final
    {
    public:
        Render_process(/* args */);
        ~Render_process();

        void InitPipeline(const int width, const int height);
        void DestroyPipeline();

    private:
        vk::Pipeline m_pipeline;
    };

}

#endif // __RENDER_PROCESS_H__
