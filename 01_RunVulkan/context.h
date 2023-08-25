#ifndef __CONTEXT_H__
#define __CONTEXT_H__

#include <memory>
#include "vulkan/vulkan.hpp"

/**
 * @brief vulkan 渲染相关接口
 * 
 */
class Context final
{
public:
    ~Context();

    static Context &GetInstance();
    static void Init();
    static void Quit();

private:
    Context(/* args */);

    /* data */
    static std::unique_ptr<Context> m_instance;

    vk::Instance m_vkInstance;
};



#endif // __CONTEXT_H__
