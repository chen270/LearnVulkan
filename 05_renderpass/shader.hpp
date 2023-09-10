#ifndef __SHADER_H__
#define __SHADER_H__

#include <memory>
#include <string>
#include "vulkan/vulkan.hpp"

namespace toy2d{

class Shader final
{
public:
    ~Shader();

    static void Init(const std::string & vertexSource, const std::string & fragSource);
    static void Quit();
    static Shader &GetInstance();

    std::vector<vk::PipelineShaderStageCreateInfo> GetStage();

    vk::ShaderModule& GetVertexModule() {
        return m_vertModule;
    }
    vk::ShaderModule& GetFragModule() {
        return m_fragModule;
    }

private:
    Shader(const std::string& vertexSource, const std::string& fragSource);

    void InitStage();

    vk::ShaderModule m_vertModule;
    vk::ShaderModule m_fragModule;

    std::vector<vk::PipelineShaderStageCreateInfo> m_stageInfo;

    static std::unique_ptr<Shader> m_instancePtr;
};




}

#endif // __SHADER_H__