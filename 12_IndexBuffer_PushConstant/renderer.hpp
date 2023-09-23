#ifndef __RENDERER_H__
#define __RENDERER_H__

#include "vulkan/vulkan.hpp"
#include "vertex.hpp"
#include "buffer.hpp"
#include "math/math.hpp"

namespace toy2d {
    class Renderer final
    {
    public:
        Renderer(int maxFlightCount = 2);
        ~Renderer();

        void DrawTriangle(const Rect& rect);
        void SetProject(int right, int left, int bottom, int top, int far, int near);

    private:
        void CreateCmdBuffer();
        void createSems();
        void createFence();
        void createVertexBuffer();
        void bufferVertexData();
        void createColorBuffer();
        void bufferColorData();
        void copyBuffer(vk::Buffer& src, vk::Buffer& dst, size_t size, size_t srcOffset, size_t dstOffset);
        void createDescriptorPool();
        void allocateSets();
        void updateSets();
        void createMVPBuffer();
        void bufferMVPData(const Mat4& model);
        void initMats();

        std::vector<vk::CommandBuffer> m_cmdBuffers;
        std::vector<vk::Semaphore> m_imageAvaliables;
        std::vector<vk::Semaphore> m_imageDrawFinishs;
        std::vector<vk::Fence> m_cmdFences;

        std::unique_ptr<Buffer> m_hostVertexBuffer; // CPU
        std::unique_ptr<Buffer> m_deviceVertexBuffer; // GPU

        std::vector<std::unique_ptr<Buffer>> m_hostColorBuffers; // CPU
        std::vector<std::unique_ptr<Buffer>> m_deviceColorBuffers; // GPU

        std::vector<std::unique_ptr<Buffer>> m_hostMVPBuffers; // CPU
        std::vector<std::unique_ptr<Buffer>> m_deviceMVPBuffers; // GPU
        Mat4 projectMat_;
        Mat4 viewMat_;
        struct MVP {
            Mat4 project;
            Mat4 view;
            Mat4 model;
        };

        //vk::DescriptorPool m_descriptorPool;
        //std::vector<vk::DescriptorSet> m_sets; // 描述符集

        int m_maxFlightCount;
        int m_curFrame;

        vk::DescriptorPool descriptorPool1_;
        vk::DescriptorPool descriptorPool2_;
        std::pair<std::vector<vk::DescriptorSet>, std::vector<vk::DescriptorSet>> m_descriptorSets;
    };
}

#endif // __RENDERER_H__
