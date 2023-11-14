#ifndef __TEXTURE2D_H__
#define __TEXTURE2D_H__

#include <string_view>

#include "vulkan/vulkan.hpp"
#include "buffer.hpp"
#include "descriptor_manager.hpp"


namespace toy2d {
    class Texture
    {
    public:
        Texture(std::string_view filename);
        ~Texture();

        vk::Image m_image;
        vk::DeviceMemory m_memory;
        vk::ImageView m_view;

        DescriptorSetManager::SetInfo m_setInfo;
    private:
        void createImage(uint32_t w, uint32_t h);
        void allocMemory();
        void createImageView();
        void updateDescriptorSet();

        void transitionImageLayoutFromUndefine2Dst();
        void transitionImageLayoutFromDst2Optimal();
        void transformData2Image(Buffer&, uint32_t w, uint32_t h);

    };

    class TextureManager final {
    public:
        static TextureManager& Instance() {
            if (!instance_) {
                instance_.reset(new TextureManager);
            }
            return *instance_;
        }

        Texture* Load(const std::string& filename){
            std::unique_ptr<Texture> ptr(new Texture(filename));
            datas_.push_back(std::move(ptr));
            return datas_.back().get();
        }
        void Destroy(Texture* texture);

        void Clear() {
            datas_.clear();
        }

    private:
        static std::unique_ptr<TextureManager> instance_;
        std::vector<std::unique_ptr<Texture>> datas_;
    };
}




#endif // __TEXTURE2D_H__