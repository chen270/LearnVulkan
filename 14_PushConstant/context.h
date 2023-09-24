#ifndef __CONTEXT_H__
#define __CONTEXT_H__

#include <memory>
#include <optional>
#include "vulkan/vulkan.hpp"
#include "toy2d.h"
#include "swapchain.h"
#include "render_process.hpp"
#include "renderer.hpp"
#include "command_manager.hpp"
#include "shader.hpp"

namespace toy2d
{
    /**
     * @brief vulkan 渲染相关接口
     *
     */
    class Context final
    {
    public:
        ~Context();

        static Context& GetInstance();
        static void Init(const std::vector<const char*>& extensions, CreateSurfaceFunc func);
        static void Quit();


        struct QueueFamilyIndices final
        {
            std::optional<uint32_t> grapghicsQueue;
            std::optional<uint32_t> presentQueue;

            operator bool() const {
                return grapghicsQueue.has_value() && presentQueue.has_value();
            }
        };

        vk::SurfaceKHR& GetSurface() { return this->m_surface; };
        vk::Device& GetDevice() { return this->m_Device; };
        vk::PhysicalDevice& GetPhyDevice() { return this->m_phyDevice; };
        QueueFamilyIndices& GetQueueFamilyIndices() { return this->queueFamilyIndices; };

        void InitSwapchain(const int w, const int h);

        void InitRenderer();
        void DestroyRenderer();

        void InitCommandPool();

        void initShaderModules(const std::string& vertexSource, const std::string& fragSource);
        void initGraphicsPipeline();

    private:
        Context(const std::vector<const char*>& extensions, CreateSurfaceFunc func);

        void createVulkanInstance(const std::vector<const char*>& extensions);
        void pickupPhysicalDevice();
        void createDevice();
        void queryQueueFamilyIndices();
        void getQueues();

        /* data */
        static Context* m_instance;

        vk::Instance m_vkInstance;
        vk::PhysicalDevice m_phyDevice;
        vk::Device m_Device;

        QueueFamilyIndices queueFamilyIndices;

        // surface
        vk::SurfaceKHR m_surface;

    public:
        vk::Queue m_graphicsQueue;
        vk::Queue m_presentQueue;

        std::unique_ptr<swapchain>m_swapchain;
        std::unique_ptr<Render_process>m_renderProcess;
        std::unique_ptr<toy2d::Renderer>m_renderer;
        std::unique_ptr<CommandManager> m_commandManager;
        std::unique_ptr<Shader> m_shader;
    };

}



#endif // __CONTEXT_H__
