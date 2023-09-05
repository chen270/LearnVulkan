#ifndef __TOOLS_H__
#define __TOOLS_H__

#include <algorithm>
#include <vector>
#include <functional>
#include <string>
#include <fstream>
#include <iostream>
#include "vulkan/vulkan.hpp"

using CreateSurfaceFunc = std::function<vk::SurfaceKHR(vk::Instance)>;

namespace toy2d {

#ifndef DIR_PATH
#error shader path not define!
#else
#define S_PATH(str) DIR_PATH##str
#endif

template <typename T, typename U>
void RemoveNosupportedElems(std::vector<T>& elems, const std::vector<U>& supportedElems,
                            std::function<bool(const T&, const U&)> eq) {
    int i = 0;
    while (i < elems.size()) {
        if (std::find_if(supportedElems.begin(), supportedElems.end(),
                         [&](const U& e) {
                            return eq(elems[i], e);
                         })
            == supportedElems.end()) {
            elems.erase(elems.begin() + i);
        } else {
            i++;
        }
    }
}

static std::string ReadWholeFile(const std::string& filename) {
    // spv 文件为二进制文件
    // std::ios::binary 标志指示以二进制模式打开文件
    // std::ios::ate 标志指示将文件指针移到文件末尾, 方便获取文件的大小。
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    if (!file.is_open()) {
        std::cout << "read " << filename << " failed" << std::endl;
        return std::string{};
    }

    auto size = file.tellg();
    std::string content;
    content.resize(size);
    file.seekg(0);

    file.read(content.data(), content.size());

    return content;
}

}

#endif // __TOOLS_H__
