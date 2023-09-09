#include "toy2d.h"
#include "context.h"
#include "shader.hpp"

namespace toy2d{
    void Init(const std::vector<const char*>& extensions, CreateSurfaceFunc func, const int w, const int h)
    {
        Context::Init(extensions, func);
        Context::GetInstance().InitSwapchain(w, h);
        Shader::Init(ReadWholeFile(S_PATH("./bin/vert.spv")), ReadWholeFile(S_PATH("./bin/frag.spv")));
        Context::GetInstance().m_renderProcess->InitPipeline(w, h);
    }

    void Quit()
    {
        Context::GetInstance().m_renderProcess->DestroyPipeline();
        Shader::Quit();

        // 需要先销毁 swapchain, 再销毁 device, 因为 swapchain 根据 device 创建的
        Context::GetInstance().DestroySwapchain();

        Context::Quit();
    }
}