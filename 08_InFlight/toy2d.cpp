#include "toy2d.h"
#include "context.h"
#include "shader.hpp"

namespace toy2d{
    void Init(const std::vector<const char*>& extensions, CreateSurfaceFunc func, const int w, const int h)
    {
        Context::Init(extensions, func);
        auto& ctx = Context::GetInstance();
        ctx.InitSwapchain(w, h);
        Shader::Init(ReadWholeFile(S_PATH("./bin/vert.spv")), ReadWholeFile(S_PATH("./bin/frag.spv")));
        ctx.m_renderProcess->InitLayout();
        ctx.m_renderProcess->InitRenderPass();
        ctx.m_swapchain->createFramebuffers(w, h);
        ctx.m_renderProcess->InitPipeline(w, h);
        ctx.InitCommandPool();
        ctx.InitRenderer();
    }

    void Quit()
    {
        Context::GetInstance().GetDevice().waitIdle(); // 让 cpu 等待所有操作完成
        Context::GetInstance().DestroyRenderer();
        Context::GetInstance().m_renderProcess.reset();
        Shader::Quit();

        Context::GetInstance().m_commandManager.reset();

        // 需要先销毁 swapchain, 再销毁 device, 因为 swapchain 根据 device 创建的
        Context::GetInstance().DestroySwapchain();

        Context::Quit();
    }

    Renderer& GetRenderer() {
        return *Context::GetInstance().m_renderer;
    };
}