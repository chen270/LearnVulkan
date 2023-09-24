#ifndef __TEXTURE2D_H__
#define __TEXTURE2D_H__

#include <string_view>

#include "vulkan/vulkan.hpp"
#include "buffer.hpp"

namespace toy2d {
    class Texture
    {
    public:
        Texture(std::string_view filename);
        ~Texture();

        vk::Image m_image;
        vk::DeviceMemory m_memory;
        vk::ImageView m_view;
    private:
        void createImage(uint32_t w, uint32_t h);
        void allocMemory();
        void createImageView();

        void transitionImageLayoutFromUndefine2Dst();
        void transitionImageLayoutFromDst2Optimal();
        void transformData2Image(Buffer&, uint32_t w, uint32_t h);
    };

}




#endif // __TEXTURE2D_H__