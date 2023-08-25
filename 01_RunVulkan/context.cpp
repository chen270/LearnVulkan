#include "context.h"
#include <mutex>
#include <iostream>

std::unique_ptr<Context> Context::m_instance = nullptr;

static std::once_flag g_flag;
Context &Context::GetInstance()
{
    std::call_once(g_flag, [&]() { Init(); });
    return *m_instance;
}

Context::Context(/* args */)
{
    vk::InstanceCreateInfo createInfo;

    // layer: 调试使用
    auto layers = vk::enumerateInstanceLayerProperties(); // 拿到所有的 layer 名字
    for (auto& layer : layers)
    {
        std::cout << layer.layerName << std::endl;
    }
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

Context::~Context()
{
    m_vkInstance.destroy();
}

void Context::Init()
{
    m_instance.reset(new Context());
}

void Context::Quit()
{
    m_instance.reset();
}