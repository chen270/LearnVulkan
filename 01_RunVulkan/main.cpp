#include "toy2d.h"
#include "SDL.h"


#undef main // SDL内部也有main函数
int main()
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Renderer* renderer = NULL;
    SDL_Texture* texture = NULL;
    SDL_Rect rect;
    rect.h = 50;
    rect.w = 50;

    SDL_Window* window = SDL_CreateWindow("chen270",
        SDL_WINDOWPOS_UNDEFINED, // 默认居中
        SDL_WINDOWPOS_UNDEFINED,
        640, 480,
        SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    if (!window) {
        SDL_Log("can not create window, err:%s\n", SDL_GetError());
        return 1;
    }

    bool b_exit = true;
    SDL_Event event;

    toy2d::Init();

    while (b_exit)
    {
        SDL_WaitEvent(&event);
        switch (event.type)
        {
        case SDL_QUIT:
            SDL_Log("receive quit event\n");
            b_exit = false;
            break;
        }
    }

    toy2d::Quit();

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}