#ifndef __CONTEXT_H__
#define __CONTEXT_H__

#include <memory>
#include <optional>
#include "vulkan/vulkan.hpp"
#include "toy2d.h"

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
            std::optional<int32_t> grapghicsQueue;
        };

    private:
        Context(const std::vector<const char*>& extensions, CreateSurfaceFunc func);

        void createVulkanInstance(const std::vector<const char*>& extensions);
        void pickupPhysicalDevice();
        void createDevice();
        void queryQueueFamilyIndices();
        void getQueues();

        /* data */
        static std::unique_ptr<Context> m_instance;

        vk::Instance m_vkInstance;
        vk::PhysicalDevice m_phyDevice;
        vk::Device m_Device;

        vk::Queue m_graphicsQueue;
        QueueFamilyIndices queueFamilyIndices;

        // surface
        vk::SurfaceKHR m_surface;
    };

}



#endif // __CONTEXT_H__
