#ifndef __RENDERER_H__
#define __RENDERER_H__

#include <unordered_map>
#include "vulkan/vulkan.hpp"
//#include "vertex.hpp"
#include "buffer.hpp"
#include "math/math.hpp"
#include "texture2d.hpp"


namespace toy2d {
    class Renderer final
    {
    public:
        Renderer(int maxFlightCount = 2);
        ~Renderer();

        void DrawRect(const Rect& rect);
        void SetProject(int right, int left, int bottom, int top, int far, int near);
        void SetDrawColor(Color kColor);
        vk::Sampler GetSampler() { return m_sampler; };

        void DrawTexture(const Rect& rect, Texture& texture);
        void StartRender();
        void EndRender();

    private:
        void CreateCmdBuffer();
        void createSems();
        void createFence();
        void createVertexBuffer();
        void bufferVertexData();
        void createIndexBuffer();
        void bufferIndexData();
        void createColorBuffer();
        void copyBuffer(vk::Buffer& src, vk::Buffer& dst, size_t size, size_t srcOffset, size_t dstOffset);
        void updateBufferSets();
        void updateImageSets(std::unique_ptr<Texture>& texture);
        void createMVPBuffer();
        void bufferMVPData(/*const Mat4& model*/);
        void initMats();
        void createSampler();
        void createTexture();

        std::vector<vk::CommandBuffer> m_cmdBuffers;
        std::vector<vk::Semaphore> m_imageAvaliables;
        std::vector<vk::Semaphore> m_imageDrawFinishs;
        std::vector<vk::Fence> m_cmdFences;

        std::unique_ptr<Buffer> m_hostVertexBuffer; // CPU
        std::unique_ptr<Buffer> m_deviceVertexBuffer; // GPU

        std::unique_ptr<Buffer> m_hostIndexBuffer; // CPU
        std::unique_ptr<Buffer> m_deviceIndexBuffer; // GPU

        std::vector<std::unique_ptr<Buffer>> m_hostColorBuffers; // CPU
        std::vector<std::unique_ptr<Buffer>> m_deviceColorBuffers; // GPU

        std::vector<std::unique_ptr<Buffer>> m_hostMVPBuffers; // CPU
        std::vector<std::unique_ptr<Buffer>> m_deviceMVPBuffers; // GPU
        Mat4 projectMat_;
        Mat4 viewMat_;
        struct MVP {
            Mat4 project;
            Mat4 view;
            //Mat4 model;
        };

        int m_maxFlightCount;
        int m_curFrame;

        std::vector<DescriptorSetManager::SetInfo> descriptorSets_;
        vk::Sampler m_sampler;

        uint32_t m_imageIndex;
    };
}

#endif // __RENDERER_H__
