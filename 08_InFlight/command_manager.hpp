#ifndef __COMMAND_MANAGER_H__
#define __COMMAND_MANAGER_H__

#include "vulkan/vulkan.hpp"

namespace toy2d {

class CommandManager final {
public:
    CommandManager();
    ~CommandManager();

    vk::CommandBuffer CreateOneCommandBuffer();
    std::vector<vk::CommandBuffer> CreateCommandBuffers(std::uint32_t count);
    void ResetCmds();
    void FreeCmd(vk::CommandBuffer);

private:
    vk::CommandPool pool_;

    vk::CommandPool createCommandPool();
};

}


#endif // __COMMAND_MANAGER_H__