#ifndef __SHADER_H__
#define __SHADER_H__

#include <memory>
#include <string>
#include "vulkan/vulkan.hpp"

namespace toy2d{

class Shader final
{
public:
    Shader(const std::string& vertexSource, const std::string& fragSource);
    ~Shader();

    vk::ShaderModule GetVertexModule() const {
        return m_vertModule;
    }
    vk::ShaderModule GetFragModule() const {
        return m_fragModule;
    }

    const std::vector<vk::DescriptorSetLayout>& GetDescriptorSetLayouts() const { return m_layouts; }

    vk::PushConstantRange GetPushConstantRange() const;
private:
    void initDescriptorSetLayouts();

    vk::ShaderModule m_vertModule;
    vk::ShaderModule m_fragModule;

    std::vector<vk::DescriptorSetLayout> m_layouts;
};
}

#endif // __SHADER_H__
