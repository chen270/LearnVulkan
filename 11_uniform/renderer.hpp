#ifndef __RENDERER_H__
#define __RENDERER_H__

#include "vulkan/vulkan.hpp"
#include "vertex.hpp"
#include "buffer.hpp"

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
        void createVertexBuffer();
        void bufferVertexData();
        void createUniformBuffer();
        void bufferUniformData();
        void copyBuffer(vk::Buffer& src, vk::Buffer& dst, size_t size, size_t srcOffset, size_t dstOffset);

        std::vector<vk::CommandBuffer> m_cmdBuffers;
        std::vector<vk::Semaphore> m_imageAvaliables;
        std::vector<vk::Semaphore> m_imageDrawFinishs;
        std::vector<vk::Fence> m_cmdFences;

        std::unique_ptr<Buffer> m_hostVertexBuffer; // CPU
        std::unique_ptr<Buffer> m_deviceVertexBuffer; // GPU

        std::vector<std::unique_ptr<Buffer>> m_hostUniformBuffers; // CPU
        std::vector<std::unique_ptr<Buffer>> m_deviceUniformBuffers; // GPU

        int m_maxFlightCount;
        int m_curFrame;
    };
}

#endif // __RENDERER_H__
