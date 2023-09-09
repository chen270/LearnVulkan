#include "shader.hpp"
#include "context.h"

namespace toy2d{

std::unique_ptr<Shader> Shader::m_instancePtr = nullptr;

Shader::Shader(const std::string& vertexSource, const std::string& fragSource)
{
    // 创建
    vk::ShaderModuleCreateInfo createInfo;
    createInfo.codeSize = vertexSource.size();

    // reinterpret_cast 在编译时解决, 并不会产生性能问题
    //createInfo.pCode = (uint32_t*)(vertexSource.data());
    createInfo.pCode = reinterpret_cast<const uint32_t*>(vertexSource.data());
    m_vertModule = Context::GetInstance().GetDevice().createShaderModule(createInfo);

    createInfo.codeSize = fragSource.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(fragSource.data());
    m_fragModule = Context::GetInstance().GetDevice().createShaderModule(createInfo);

    InitStage();
}

Shader::~Shader()
{
    auto& device = Context::GetInstance().GetDevice();
    device.destroyShaderModule(m_vertModule);
    device.destroyShaderModule(m_fragModule);
}

void Shader::Init(const std::string& vertexSource, const std::string& fragSource)
{
    m_instancePtr.reset(new Shader(vertexSource, fragSource));
}

void Shader::Quit()
{
    m_instancePtr.reset();
}

Shader& Shader::GetInstance()
{
    return *m_instancePtr;
}

std::vector<vk::PipelineShaderStageCreateInfo> Shader::GetStage()
{
    return m_stageInfo;
}

void Shader::InitStage()
{
    m_stageInfo.resize(2);
    m_stageInfo[0].setStage(vk::ShaderStageFlagBits::eVertex)
        .setModule(m_vertModule)
        .setPName("main"); // 入口函数

    m_stageInfo[1].setStage(vk::ShaderStageFlagBits::eFragment)
        .setModule(m_fragModule)
        .setPName("main"); // 入口函数
}

}