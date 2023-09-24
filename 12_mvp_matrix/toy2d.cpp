#include "toy2d.h"
#include "context.h"
#include "shader.hpp"

namespace toy2d{
    void Init(const std::vector<const char*>& extensions, CreateSurfaceFunc func, const int w, const int h)
    {
        Context::Init(extensions, func);
        auto& ctx = Context::GetInstance();
        ctx.InitSwapchain(w, h);
        ctx.initShaderModules(ReadWholeFile(S_PATH("./bin/vert.spv")), ReadWholeFile(S_PATH("./bin/frag.spv")));
        ctx.m_renderProcess->InitLayout();
        ctx.m_renderProcess->InitRenderPass();
        ctx.m_swapchain->createFramebuffers(w, h);
        ctx.initGraphicsPipeline();
        ctx.InitCommandPool();
        ctx.InitRenderer();
        Context::GetInstance().m_renderer->SetProject(w, 0, 0, h, -1, 1);
    }

    void Quit()
    {
        Context::GetInstance().GetDevice().waitIdle(); // 让 cpu 等待所有操作完成
        Context::GetInstance().DestroyRenderer();
        Context::Quit();
    }

    Renderer& GetRenderer() {
        return *Context::GetInstance().m_renderer;
    };
}