#include "shader.hpp"
#include "context.h"

namespace toy2d{

Shader::Shader(const std::string& vertexSource, const std::string& fragSource)
{
    // 创建
    vk::ShaderModuleCreateInfo createInfo;
    createInfo.codeSize = vertexSource.size();

    createInfo.pCode = reinterpret_cast<const uint32_t*>(vertexSource.data());
    m_vertModule = Context::GetInstance().GetDevice().createShaderModule(createInfo);

    createInfo.codeSize = fragSource.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(fragSource.data());
    m_fragModule = Context::GetInstance().GetDevice().createShaderModule(createInfo);

    initDescriptorSetLayouts();
}

Shader::~Shader()
{
    auto& device = Context::GetInstance().GetDevice();
    for (auto& layout : m_layouts) {
        device.destroyDescriptorSetLayout(layout);
    }
    m_layouts.clear();
    device.destroyShaderModule(m_vertModule);
    device.destroyShaderModule(m_fragModule);
}

void Shader::initDescriptorSetLayouts() {
    auto& device = Context::GetInstance().GetDevice();
    vk::DescriptorSetLayoutCreateInfo createInfo;
    vk::DescriptorSetLayoutBinding binding;
    binding.setBinding(0)
        .setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eUniformBuffer)
        .setStageFlags(vk::ShaderStageFlagBits::eVertex);
    createInfo.setBindings(binding);
    m_layouts.push_back(device.createDescriptorSetLayout(createInfo));

    binding.setBinding(0)
        .setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eUniformBuffer)
        .setStageFlags(vk::ShaderStageFlagBits::eFragment);
    createInfo.setBindings(binding);
    m_layouts.push_back(device.createDescriptorSetLayout(createInfo));
}

vk::PushConstantRange Shader::GetPushConstantRange() const {
    vk::PushConstantRange range;
    range.setOffset(0)
        .setSize(sizeof(Mat4))
        .setStageFlags(vk::ShaderStageFlagBits::eVertex); // 指定哪个着色器使用
    return range;
}

}