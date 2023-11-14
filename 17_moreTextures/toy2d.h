#ifndef __TOY2D_H__
#define __TOY2D_H__

#include <vector>
#include <functional>
#include "vulkan/vulkan.hpp"
#include "tools.hpp"
#include "renderer.hpp"

namespace toy2d
{
    void Init(const std::vector<const char*>& extensions, CreateSurfaceFunc func, const int w, const int h);
    void Quit();
    Renderer& GetRenderer();
    Texture* LoadTexture(const std::string& filename);
}

#endif // __TOY2D_H__
