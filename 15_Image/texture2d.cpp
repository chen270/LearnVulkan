#include "texture2d.hpp"

#include <stdexcept>
#include <memory>


#define STB_IMAGE_IMPLEMENTATION
#include "third_party/stb_image.h"

#include "context.h"

namespace toy2d {
    Texture::Texture(std::string_view filename) {
        int w, h, channel;

        // channel: 1-gray, 3-rgb, 4-rgba
        // STBI_rgb_alpha 可以指定转换成哪个通道数, 如果给 0 那就不做转换
        stbi_uc* pixels = stbi_load(filename.data(), &w, &h, &channel, STBI_rgb_alpha);
        size_t size = w * h * 4;

        if (!pixels) {
            throw std::runtime_error("image load failed");
        }

        // 把数据给到 cpu buffer 里面
        std::unique_ptr<Buffer> buffer(new Buffer(size, vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible));
        memcpy(buffer->m_map, pixels, size);

        createImage(w, h);
        allocMemory();
        Context::GetInstance().GetDevice().bindImageMemory(m_image, m_memory, 0);

        transitionImageLayoutFromUndefine2Dst();
        transformData2Image(*buffer, w, h);
        transitionImageLayoutFromDst2Optimal();

        createImageView();

        stbi_image_free(pixels);
    }

    Texture::~Texture()
    {
        auto& device = Context::GetInstance().GetDevice();

        device.freeMemory(m_memory);
        device.destroyImage(m_image);
    }

    void Texture::transformData2Image(Buffer& buffer, uint32_t w, uint32_t h) {
        Context::GetInstance().m_commandManager->ExecuteCmd(Context::GetInstance().m_graphicsQueue,
            [&](vk::CommandBuffer cmdBuf) {
            vk::BufferImageCopy region;
            vk::ImageSubresourceLayers subsource;
            subsource.setAspectMask(vk::ImageAspectFlagBits::eColor)
                .setBaseArrayLayer(0)
                .setMipLevel(0)
                .setLayerCount(1);
            region.setBufferImageHeight(0)
                .setBufferOffset(0)
                .setImageOffset(0)
                .setImageExtent({ w, h, 1 })
                .setBufferRowLength(0)
                .setImageSubresource(subsource);
            cmdBuf.copyBufferToImage(buffer.m_buffer, m_image,
                vk::ImageLayout::eTransferDstOptimal,
                region);
        });
    }

    void Texture::transitionImageLayoutFromUndefine2Dst() {
        Context::GetInstance().m_commandManager->ExecuteCmd(Context::GetInstance().m_graphicsQueue,
            [&](vk::CommandBuffer cmdBuf) {
            vk::ImageMemoryBarrier barrier;
            vk::ImageSubresourceRange range;
            range.setLayerCount(1)
                .setBaseArrayLayer(0)
                .setLevelCount(1)
                .setBaseMipLevel(0)
                .setAspectMask(vk::ImageAspectFlagBits::eColor);
            barrier.setImage(m_image)
                .setOldLayout(vk::ImageLayout::eUndefined)
                .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
                .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                .setDstAccessMask((vk::AccessFlagBits::eTransferWrite))
                .setSubresourceRange(range);
            cmdBuf.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer,
                {}, {}, nullptr, barrier);
        });
    }

    void Texture::transitionImageLayoutFromDst2Optimal() {
        Context::GetInstance().m_commandManager->ExecuteCmd(Context::GetInstance().m_graphicsQueue,
            [&](vk::CommandBuffer cmdBuf) {
            vk::ImageMemoryBarrier barrier;
            vk::ImageSubresourceRange range;
            range.setLayerCount(1)
                .setBaseArrayLayer(0)
                .setLevelCount(1)
                .setBaseMipLevel(0)
                .setAspectMask(vk::ImageAspectFlagBits::eColor);
            barrier.setImage(m_image)
                .setOldLayout(vk::ImageLayout::eTransferDstOptimal)
                .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                .setSrcAccessMask((vk::AccessFlagBits::eTransferWrite))
                .setDstAccessMask((vk::AccessFlagBits::eShaderRead))
                .setSubresourceRange(range);
            cmdBuf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
                {}, {}, nullptr, barrier);
        });
    }

    void Texture::createImage(uint32_t w, uint32_t h) {
        vk::ImageCreateInfo createInfo;
        createInfo.setImageType(vk::ImageType::e2D) // 2d 纹理
            .setArrayLayers(1) // 1 份图像
            .setMipLevels(1) // 1 表示自己本身
            .setExtent({ w, h, 1 }) // 宽度高度和深度, 3d纹理需要深度
            .setFormat(vk::Format::eR8G8B8A8Srgb)
            .setTiling(vk::ImageTiling::eOptimal)
            .setInitialLayout(vk::ImageLayout::eUndefined)
            .setUsage(vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled)
            .setSamples(vk::SampleCountFlagBits::e1);
        m_image = Context::GetInstance().GetDevice().createImage(createInfo);
    }

    void Texture::allocMemory() {
        auto& device = Context::GetInstance().GetDevice();
        vk::MemoryAllocateInfo allocInfo;

        auto requirements = device.getImageMemoryRequirements(m_image);
        allocInfo.setAllocationSize(requirements.size);

        auto index = Buffer::QueryBufferMemTypeIndex(requirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
        allocInfo.setMemoryTypeIndex(index);

        m_memory = device.allocateMemory(allocInfo);
    }

    void Texture::createImageView() {
        vk::ImageViewCreateInfo createInfo;
        vk::ComponentMapping mapping;
        vk::ImageSubresourceRange range;
        range.setAspectMask(vk::ImageAspectFlagBits::eColor)
            .setBaseArrayLayer(0)
            .setLayerCount(1)
            .setLevelCount(1)
            .setBaseMipLevel(0);
        createInfo.setImage(m_image)
            .setViewType(vk::ImageViewType::e2D)
            .setComponents(mapping)
            .setFormat(vk::Format::eR8G8B8A8Srgb)
            .setSubresourceRange(range);
        m_view = Context::GetInstance().GetDevice().createImageView(createInfo);
    }
}
