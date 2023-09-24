#ifndef __COMMAND_MANAGER_H__
#define __COMMAND_MANAGER_H__

#include "vulkan/vulkan.hpp"
#include <functional>
namespace toy2d {

class CommandManager final {
public:
    CommandManager();
    ~CommandManager();

    vk::CommandBuffer CreateOneCommandBuffer();
    std::vector<vk::CommandBuffer> CreateCommandBuffers(std::uint32_t count);
    void ResetCmds();
    void FreeCmd(vk::CommandBuffer);

    using RecordCmdFunc = std::function<void(vk::CommandBuffer&)>;
    void ExecuteCmd(vk::Queue queue, RecordCmdFunc func);

private:
    vk::CommandPool m_pool;

    vk::CommandPool createCommandPool();
};

}


#endif // __COMMAND_MANAGER_H__