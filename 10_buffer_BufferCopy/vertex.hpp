#ifndef __VERTEX_H__
#define __VERTEX_H__

#include "vulkan/vulkan.hpp"

namespace toy2d {
    struct Vertex final
    {
        float x, y;

        // 跟类有关，跟对象无关
        static vk::VertexInputAttributeDescription GetAttribute() {
            vk::VertexInputAttributeDescription attr;
            attr.setBinding(0) // GLES 绑定点
                .setFormat(vk::Format::eR32G32Sfloat) // 底层数据类型
                .setLocation(0) // 和 顶点着色器的 location 相对应
                .setOffset(0); //内存偏移
            return attr;
        }

        static vk::VertexInputBindingDescription GetBinding() {
            vk::VertexInputBindingDescription binding;
            binding.setBinding(0) // 上同
                .setInputRate(vk::VertexInputRate::eVertex) // 顶点输入的数目
                .setStride(sizeof(Vertex)); // 顶点数据之间的偏移
            return binding;
        }
    };
}

#endif // __VERTEX_H__
