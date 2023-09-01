#ifndef __TOY2D_H__
#define __TOY2D_H__

#include <vector>
#include <functional>
#include "vulkan/vulkan.hpp"
#include "tools.hpp"

namespace toy2d
{
    //std::function<void(vk::Instance instance)> fun;
    //using CreateSurfaceFunc = std::function<VkSurfaceKHR(vk::Instance)>;

    void Init(const std::vector<const char*>& extensions, CreateSurfaceFunc func);
    void Quit();
}

#endif // __TOY2D_H__
