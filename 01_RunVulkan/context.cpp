﻿#include "context.h"
#include <mutex>
#include <iostream>

std::unique_ptr<Context> Context::m_instance = nullptr;

static std::once_flag g_flag;
Context &Context::GetInstance()
{
    return *m_instance;
}

Context::Context(const std::vector<const char*>& extensions)
{
    createVulkanInstance(extensions);
    pickupPhysicalDevice();
    queryQueueFamilyIndices();
    createDevice();
    getQueues();
}

Context::~Context()
{
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

    m_phyDevice = physicalDevices[0]; // 自信输入
}

void Context::createDevice()
{
    vk::DeviceCreateInfo createInfo;

    // 这里也可以设置扩展，但是如果在 instance 上创建扩展了，这里便会顺延下来，但也有独有的扩展和层
    //createInfo.setPEnabledExtensionNames();

    // queue
    vk::DeviceQueueCreateInfo queueCreateInfo;

    float propertity = 1.0f;
    queueCreateInfo.setPQueuePriorities(&propertity)
        .setQueueCount(1)
        .setQueueFamilyIndex(queueFamilyIndices.grapghicsQueue.value());
    createInfo.setQueueCreateInfos(queueCreateInfo);

    m_Device = m_phyDevice.createDevice(createInfo);
}

void Context::queryQueueFamilyIndices()
{
    auto properties = m_phyDevice.getQueueFamilyProperties();
    for (int i = 0; i < properties.size(); ++i)
    {
        if (properties[i].queueFlags & vk::QueueFlagBits::eGraphics)
        {
            queueFamilyIndices.grapghicsQueue = i;
            break;
        }
    }
}

void Context::getQueues()
{
    m_graphicsQueue = m_Device.getQueue(queueFamilyIndices.grapghicsQueue.value(), 0);
}

void Context::Init(const std::vector<const char*>& extensions)
{
    m_instance.reset(new Context(extensions));
}

void Context::Quit()
{
    m_instance.reset();
}