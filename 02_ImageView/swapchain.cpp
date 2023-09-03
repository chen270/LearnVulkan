#include "swapchain.h"
#include "context.h"
#include <set>

namespace toy2d{

swapchain::swapchain(const int w, const int h)
{
    // 需要先查询一些信息，用于填充后面的 createInfo
    queryInfo(w, h);

    vk::SwapchainCreateInfoKHR createInfo;
    createInfo.setClipped(true)
        .setImageArrayLayers(1)
        .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
        .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
        .setSurface(Context::GetInstance().GetSurface())
        .setImageColorSpace(m_swapchainInfo.format.colorSpace)
        .setImageFormat(m_swapchainInfo.format.format)
        .setImageExtent(m_swapchainInfo.imageExtent)
        .setMinImageCount(m_swapchainInfo.imageCount)
        .setPresentMode(m_swapchainInfo.presentMode);

    auto& queueFamilyIndices = Context::GetInstance().GetQueueFamilyIndices();
    if (queueFamilyIndices.grapghicsQueue.value() == queueFamilyIndices.presentQueue.value()) {
        createInfo.setQueueFamilyIndices(queueFamilyIndices.grapghicsQueue.value())
            .setImageSharingMode(vk::SharingMode::eExclusive);
    }
    else {
        std::vector<uint32_t> myVector = { queueFamilyIndices.grapghicsQueue.value(), queueFamilyIndices.presentQueue.value() };
        createInfo.setQueueFamilyIndices(myVector)
            .setImageSharingMode(vk::SharingMode::eConcurrent);
    }

    m_swapchain = Context::GetInstance().GetDevice().createSwapchainKHR(createInfo);

    getImages();
    createImageViews();
}

swapchain::~swapchain()
{
    // 销毁 imageview
    for (auto& view : m_imageViews) {
        Context::GetInstance().GetDevice().destroyImageView(view);
    }

    Context::GetInstance().GetDevice().destroySwapchainKHR(m_swapchain);
}

void swapchain::queryInfo(const int w, const int h)
{
    // surface 查询
    const auto& phyDevice = Context::GetInstance().GetPhyDevice();
    const auto& surface = Context::GetInstance().GetSurface();

    // 格式色彩空间查询
    auto formats = phyDevice.getSurfaceFormatsKHR(surface);
    m_swapchainInfo.format = formats[0];
    for (const auto& format : formats) {
        if (format.format == vk::Format::eR8G8B8A8Srgb &&
            format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            m_swapchainInfo.format = format;
            break;
        }
    }

    // 与 surface 相关的能力信息，包括最小和最大图像数量、图像尺寸范围
    auto capabilities = phyDevice.getSurfaceCapabilitiesKHR(surface);

    // 这里设置图像数量为2, 设置为双缓冲
    m_swapchainInfo.imageCount = std::clamp<uint32_t>(2, capabilities.minImageCount, capabilities.maxImageCount);

    m_swapchainInfo.imageExtent.width = 
        std::clamp<uint32_t>(w, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    m_swapchainInfo.imageExtent.height = 
        std::clamp<uint32_t>(h, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    // 跟 opengl 类似, 还有 eIdentity 等选项
    m_swapchainInfo.transform = capabilities.currentTransform;

    auto presents = phyDevice.getSurfacePresentModesKHR(surface);
    m_swapchainInfo.presentMode = vk::PresentModeKHR::eFifo;
    for (const auto& present : presents) {
        if (present == vk::PresentModeKHR::eMailbox) {
            m_swapchainInfo.presentMode = present;
            break;
        }
    }
}

void swapchain::getImages()
{
   m_images = Context::GetInstance().GetDevice().getSwapchainImagesKHR(m_swapchain);
}

void swapchain::createImageViews()
{
    m_imageViews.resize(m_images.size());

    for (int i = 0; i < m_images.size(); ++i)
    {
        vk::ImageViewCreateInfo createInfo;
        vk::ComponentMapping mapping;
        vk::ImageSubresourceRange range;
        range.setBaseMipLevel(0)
            .setLevelCount(1)
            .setBaseArrayLayer(0)
            .setLayerCount(1)
            .setAspectMask(vk::ImageAspectFlagBits::eColor);

        createInfo.setImage(m_images[i])
            .setViewType(vk::ImageViewType::e2D)
            .setComponents(mapping)
            .setFormat(m_swapchainInfo.format.format)
            .setSubresourceRange(range);

        m_imageViews[i] = Context::GetInstance().GetDevice().createImageView(createInfo);
    }
}

}