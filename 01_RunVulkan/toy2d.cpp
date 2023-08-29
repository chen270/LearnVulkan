#include "toy2d.h"
#include "context.h"

namespace toy2d{
    void Init(const std::vector<const char*>& extensions)
    {
        Context::Init(extensions);
    }

    void Quit()
    {
        Context::Quit();
    }
}