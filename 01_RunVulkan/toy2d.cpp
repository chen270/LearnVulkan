#include "toy2d.h"
#include "context.h"

namespace toy2d{
    void Init(const std::vector<const char*>& extensions, CreateSurfaceFunc func)
    {
        Context::Init(extensions, func);
    }

    void Quit()
    {
        Context::Quit();
    }
}