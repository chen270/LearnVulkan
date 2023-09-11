#ifndef __SWAPCHAIN_H__
#define __SWAPCHAIN_H__

#include "vulkan/vulkan.hpp"

namespace toy2d{
class swapchain
{
public:
    swapchain(const int w, const int h);
    ~swapchain();

    struct SwapchainInfo {
        vk::Extent2D imageExtent; // 交换链图像的分辨率
        uint32_t imageCount; // 图像数量
        vk::SurfaceFormatKHR format; // 物理设备surface格式
        vk::SurfaceTransformFlagsKHR transform; // surface 变换操作, 如镜像翻转,旋转等
        vk::PresentModeKHR presentMode;
    } m_swapchainInfo;

    void createFramebuffers(const int w, const int h);

private:
    vk::SwapchainKHR m_swapchain;

    std::vector<vk::Image> m_images;
    std::vector<vk::ImageView>m_imageViews;
    std::vector<vk::Framebuffer>m_framebuffers;


    void queryInfo(const int w, const int h);
    void getImages();
    void createImageViews();
    void destroyFramebuffers();
};

}

#endif // __SWAPCHAIN_H__
