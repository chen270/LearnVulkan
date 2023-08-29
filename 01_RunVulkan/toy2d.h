#ifndef __TOY2D_H__
#define __TOY2D_H__

#include <vector>
#include <functional>
#include "vulkan/vulkan.hpp"

namespace toy2d
{
    std::function<void(vk::Instance instance)> fun;

    void Init(const std::vector<const char*>& extensions);
    void Quit();
}

#endif // __TOY2D_H__
