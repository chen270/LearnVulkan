#ifndef __RENDERER_H__
#define __RENDERER_H__

#include "vulkan/vulkan.hpp"

namespace toy2d {
    class Renderer final
    {
    public:
        Renderer();
        ~Renderer();

        void Render();

    private:
        void AllocateCmdBuffer();
        void InitCmdPool();
        void createSems();
        void createFence();

        vk::CommandPool m_cmdPool;
        vk::CommandBuffer m_cmdBuffer;
        vk::Semaphore m_imageAvaliable;
        vk::Semaphore m_imageDrawFinish;
        vk::Fence m_cmdFence;
    };
}

#endif // __RENDERER_H__
