#include "toy2d.h"
#include "SDL.h"
#include "SDL_vulkan.h"
#include <vector>
#include <iostream>
//#include "renderer.hpp"


#undef main // SDL内部也有main函数
int main()
{
    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_Renderer* renderer = NULL;
    SDL_Texture* texture = NULL;
    SDL_Rect rect;
    rect.h = 50;
    rect.w = 50;

    const int width = 640;
    const int height = 640;
    SDL_Window* window = SDL_CreateWindow("chen270",
        SDL_WINDOWPOS_UNDEFINED, // 默认居中
        SDL_WINDOWPOS_UNDEFINED,
        width, height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);
    if (!window) {
        SDL_Log("can not create window, err:%s\n", SDL_GetError());
        return 1;
    }

    // 典型的 vulkan C 接口写法, 两次调用
    unsigned int count = 0;
    SDL_Vulkan_GetInstanceExtensions(window, &count, nullptr);
    std::vector<const char*>extensions(count);
    SDL_Vulkan_GetInstanceExtensions(window, &count, extensions.data());

    //SDL_Vulkan_CreateSurface()

    std::cout << "get extensions:" << std::endl;
    for (const auto& extension : extensions)
    {
        std::cout << extension << std::endl;
    }
    std::cout << "--------" << std::endl;

    toy2d::Init(extensions, [&](vk::Instance instance) {
        VkSurfaceKHR surface;
        if (!SDL_Vulkan_CreateSurface(window, instance, &surface)) {
            throw std::runtime_error("SDL can not create surface!");
        }
        return surface;
    }, width, height);

    auto& toyRenderer = toy2d::GetRenderer();

    bool b_exit = true;
    SDL_Event event;
    float x = 100, y = 100;
    toyRenderer.SetDrawColor(toy2d::Color{ 1, 1, 1 });
    while (b_exit)
    {
        SDL_WaitEvent(&event);
        if (event.type == SDL_QUIT) {
            b_exit = false;
            break;
        }
        else if (event.type == SDL_KEYDOWN) {
            // 注意：需要英文输入法
            if (event.key.keysym.sym == SDLK_a) {
                x -= 10;
            }
            if (event.key.keysym.sym == SDLK_d) {
                x += 10;
            }
            if (event.key.keysym.sym == SDLK_w) {
                y -= 10;
            }
            if (event.key.keysym.sym == SDLK_s) {
                y += 10;
            }
            if (event.key.keysym.sym == SDLK_0) {
                toyRenderer.SetDrawColor(toy2d::Color{ 1, 0, 0 });
            }
            if (event.key.keysym.sym == SDLK_1) {
                toyRenderer.SetDrawColor(toy2d::Color{ 0, 1, 0 });
            }
            if (event.key.keysym.sym == SDLK_2) {
                toyRenderer.SetDrawColor(toy2d::Color{ 0, 0, 1 });
            }
            if (event.key.keysym.sym == SDLK_3) {
                toyRenderer.SetDrawColor(toy2d::Color{ 1, 1, 1 });
            }
        }
        toyRenderer.DrawRect(toy2d::Rect{ toy2d::Vec{x, y},
                                       toy2d::Size{200, 200} });
    }

    toy2d::Quit();

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}