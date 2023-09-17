#ifndef __RENDERER_H__
#define __RENDERER_H__

#include "vulkan/vulkan.hpp"
#include "vertex.hpp"

namespace toy2d {
    class Renderer final
    {
    public:
        Renderer(int maxFlightCount = 2);
        ~Renderer();

        void DrawTriangle();

    private:
        void CreateCmdBuffer();
        void createSems();
        void createFence();

        std::vector<vk::CommandBuffer> m_cmdBuffers;
        std::vector<vk::Semaphore> m_imageAvaliables;
        std::vector<vk::Semaphore> m_imageDrawFinishs;
        std::vector<vk::Fence> m_cmdFences;

        int m_maxFlightCount;
        int m_curFrame;
    };
}

#endif // __RENDERER_H__
