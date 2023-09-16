#ifndef __RENDERER_H__
#define __RENDERER_H__

#include "vulkan/vulkan.hpp"

namespace toy2d {
    class Renderer final
    {
    public:
        Renderer();
        ~Renderer();

    private:
        void AllocateCmdBuffer();
        void InitCmdPool();

        vk::CommandPool m_cmdPool;
        vk::CommandBuffer m_cmdBuffer;
    };
}

#endif // __RENDERER_H__
