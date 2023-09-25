#include "context.h"
#include <mutex>
#include <iostream>

namespace toy2d
{
    Context* Context::m_instance = nullptr;

    static std::once_flag g_flag;
    Context& Context::GetInstance()
    {
        return *m_instance;
    }

    Context::Context(const std::vector<const char*>& extensions, CreateSurfaceFunc func)
    {
        createVulkanInstance(extensions);
        pickupPhysicalDevice();

        m_surface = nullptr;
        m_surface = func(m_vkInstance); // C 与 C++ 接口可以互相转换，所以可以
        if (m_surface == nullptr)
        {
            // TODO...
        }

        queryQueueFamilyIndices();
        createDevice();
        getQueues();
        m_renderProcess.reset(new Render_process());
    }

    Context::~Context()
    {
        m_shader.reset();
        m_commandManager.reset();
        m_renderProcess.reset();
        m_swapchain.reset();
        m_vkInstance.destroySurfaceKHR(m_surface);
        m_Device.destroy();
        m_vkInstance.destroy();
    }

    static bool checkValidationLayerSupport(const std::vector<const char*>& validationLayers) {
        auto layers = vk::enumerateInstanceLayerProperties(); // 拿到所有的 layer 名字
        for (const char* layerName : validationLayers) {
            bool layerFound = false;

            for (const auto& layerProperties : layers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound) {
                return false;
            }
        }

        return true;
    }

#if 0
    void Context::createVulkanInstance(const std::vector<const char*>& extensions)
    {
        vk::InstanceCreateInfo createInfo;

        // extension
        //auto extensions = vk::enumerateInstanceExtensionProperties();
        //for (const auto& extension : extensions)
        //{
        //    std::cout << extension.extensionName << std::endl;
        //}

        //std::cout << "------------------" << std::endl;

        // layer: 调试使用
        //auto layers = vk::enumerateInstanceLayerProperties(); // 拿到所有的 layer 名字
        //for (const auto& layer : layers)
        //{
        //    std::cout << layer.layerName << std::endl;
        //}

        std::vector<const char*>validationLayers = { "VK_LAYER_KHRONOS_validation" };

        RemoveNosupportedElems<const char*, vk::LayerProperties>(validationLayers, vk::enumerateInstanceLayerProperties(),
            [](const char* e1, const vk::LayerProperties& e2) {
            return std::strcmp(e1, e2.layerName) == 0;
        });

        checkValidationLayerSupport(validationLayers);
        createInfo.setPEnabledLayerNames(validationLayers)
            .setPpEnabledExtensionNames(extensions.data());
        //VK_LAYER_KHRONOS_validation
        // Extention: 如果显卡是 rtx, 那么会有这个扩展
        //createInfo.setPEnabledExtensionNames

        // 参数打包
        vk::ApplicationInfo appInfo;

        // 注意这里的参数不是 VK_VERSION_1_3
        appInfo.setApiVersion(VK_API_VERSION_1_3); // 设置想使用的 api 版本号, 向前兼容
        createInfo.setPApplicationInfo(&appInfo);

        m_vkInstance = vk::createInstance(createInfo);
    }
#else

    void Context::createVulkanInstance(const std::vector<const char*>& extensions)
    {
        vk::InstanceCreateInfo createInfo;
        // 参数打包
        vk::ApplicationInfo appInfo;

        appInfo.setApiVersion(VK_API_VERSION_1_3); // 设置想使用的 api 版本号, 向前兼容
        createInfo.setPApplicationInfo(&appInfo);

        std::vector<const char*>validationLayers = { "VK_LAYER_KHRONOS_validation" };

        RemoveNosupportedElems<const char*, vk::LayerProperties>(validationLayers, vk::enumerateInstanceLayerProperties(),
            [](const char* e1, const vk::LayerProperties& e2) {
            return std::strcmp(e1, e2.layerName) == 0;
        });

        checkValidationLayerSupport(validationLayers);
        createInfo.setPEnabledLayerNames(validationLayers)
            .setPEnabledExtensionNames(extensions);



        m_vkInstance = vk::createInstance(createInfo);
    }


    //void Context::createVulkanInstance(const std::vector<const char*>& extensions) {
    //    vk::InstanceCreateInfo createInfo;
    //    vk::ApplicationInfo appInfo;
    //    appInfo.setApiVersion(VK_API_VERSION_1_3);

    //    createInfo.setPApplicationInfo(&appInfo);
    //    //instance = vk::createInstance(createInfo);

    //    std::vector<const char*> layers = { "VK_LAYER_KHRONOS_validation" };

    //    RemoveNosupportedElems<const char*, vk::LayerProperties>(layers, vk::enumerateInstanceLayerProperties(),
    //        [](const char* e1, const vk::LayerProperties& e2) {
    //        return std::strcmp(e1, e2.layerName) == 0;
    //    });
    //    createInfo.setPEnabledLayerNames(layers)
    //        .setPEnabledExtensionNames(extensions);

    //    m_vkInstance = vk::createInstance(createInfo);
    //}
#endif

    void Context::pickupPhysicalDevice()
    {
        // Device
        std::cout << "------------------" << std::endl;

        m_phyDevice = VK_NULL_HANDLE;

        uint32_t deviceCount = 0;
        // 列举可用的物理设备
        std::vector<vk::PhysicalDevice> physicalDevices = m_vkInstance.enumeratePhysicalDevices();
        // 输出设备信息
        for (const auto& device : physicalDevices) {
            vk::PhysicalDeviceProperties deviceProperties = device.getProperties();
            std::cout << "Device Name: " << deviceProperties.deviceName << std::endl;

            vk::PhysicalDeviceType type = deviceProperties.deviceType;
            //vk::PhysicalDeviceType::eCpu // 软渲染，cpu当作显卡

            auto feature = device.getFeatures();
            if (feature.geometryShader) // 用这个判断显卡是否支持vulkan不够准确
            {
                // TODO...
            }

            // 可能还有其他设备属性信息...
        }

        // 获取设备属性和特性
        vk::PhysicalDeviceProperties deviceProperties = physicalDevices[0].getProperties();
        vk::PhysicalDeviceFeatures deviceFeatures = physicalDevices[0].getFeatures();

        // 查询 PushConstants 限制大小
        auto maxPushConstantsSize = deviceProperties.limits.maxPushConstantsSize;
        std::cout << "maxPushConstantsSize: " << maxPushConstantsSize << std::endl;

        // 查询是否支持按位操作混合
        auto isLogicOp = deviceFeatures.logicOp;
        std::cout << "isLogicOp: " << isLogicOp << std::endl;

        m_phyDevice = physicalDevices[0]; // 自信输入
    }

    void Context::createDevice()
    {
        // swapchain
        std::array extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };


        vk::DeviceCreateInfo createInfo;

        // 这里也可以设置扩展，但是如果在 instance 上创建扩展了，这里便会顺延下来，但也有独有的扩展和层
        //createInfo.setPEnabledExtensionNames();

        // queue
        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;

        float propertity = 1.0f;

        if (queueFamilyIndices.grapghicsQueue.value() == queueFamilyIndices.presentQueue.value())
        {
            vk::DeviceQueueCreateInfo queueCreateInfo;
            queueCreateInfo.setPQueuePriorities(&propertity)
                .setQueueCount(1)
                .setQueueFamilyIndex(queueFamilyIndices.grapghicsQueue.value());

            queueCreateInfos.push_back(std::move(queueCreateInfo));
        }
        else
        {
            vk::DeviceQueueCreateInfo queueCreateInfo1;
            queueCreateInfo1.setPQueuePriorities(&propertity)
                .setQueueCount(1)
                .setQueueFamilyIndex(queueFamilyIndices.grapghicsQueue.value());

            queueCreateInfos.push_back(std::move(queueCreateInfo1));

            vk::DeviceQueueCreateInfo queueCreateInfo2;
            queueCreateInfo2.setPQueuePriorities(&propertity)
                .setQueueCount(1)
                .setQueueFamilyIndex(queueFamilyIndices.presentQueue.value());

            queueCreateInfos.push_back(std::move(queueCreateInfo2));
        }

        vk::PhysicalDeviceFeatures deviceFeatures = m_phyDevice.getFeatures();
        createInfo.setQueueCreateInfos(queueCreateInfos)
            .setPEnabledExtensionNames(extensions).
            setPEnabledFeatures(&deviceFeatures);


        m_Device = m_phyDevice.createDevice(createInfo);
    }

    void Context::queryQueueFamilyIndices()
    {
        auto properties = m_phyDevice.getQueueFamilyProperties();
        for (int i = 0; i < properties.size(); ++i)
        {
            const auto& property = properties[i];
            if (property.queueFlags & vk::QueueFlagBits::eGraphics)
            {
                queueFamilyIndices.grapghicsQueue = i;
            }

            if (m_phyDevice.getSurfaceSupportKHR(i, m_surface))
            {
                queueFamilyIndices.presentQueue = i;
            }

            if (queueFamilyIndices)
                break;
        }
    }

    void Context::getQueues()
    {
        // vk::Queue
        m_graphicsQueue = m_Device.getQueue(queueFamilyIndices.grapghicsQueue.value(), 0);
        m_presentQueue = m_Device.getQueue(queueFamilyIndices.presentQueue.value(), 0);
    }

    void Context::InitRenderer()
    {
        m_renderer.reset(new Renderer());
    }

    void Context::DestroyRenderer()
    {
        m_renderer.reset();
    }

    void Context::InitSwapchain(const int w, const int h)
    {
        m_swapchain.reset(new swapchain(w, h));
    }

    void Context::InitCommandPool()
    {
        m_commandManager = std::make_unique<CommandManager>();
    }

    void Context::initShaderModules(const std::string& vertexSource, const std::string& fragSource) {
        m_shader = std::make_unique<Shader>(vertexSource, fragSource);
    }

    void Context::initGraphicsPipeline() {
        m_renderProcess->RecreateGraphicsPipeline(*m_shader);
    }

    void Context::Init(const std::vector<const char*>& extensions, CreateSurfaceFunc func)
    {
        m_instance = new Context(extensions, func);
    }

    void Context::Quit()
    {
        delete m_instance;
    }
}
