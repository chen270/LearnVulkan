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

private:
    Shader(const std::string& vertexSource, const std::string& fragSource);

    vk::ShaderModule m_vertModule;
    vk::ShaderModule m_fragModule;

    static std::unique_ptr<Shader> m_instancePtr;
};




}

#endif // __SHADER_H__