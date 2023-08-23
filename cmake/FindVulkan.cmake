# vulkan config
find_package(Vulkan REQUIRED)
set(VULKAN_USER_DIR "" CACHE PATH "Vulkan root dir, example: C:/VulkanSDK/1.3.216.0")
if(NOT Vulkan_FOUND)
    message(WARNING "Error: Vulkan not found, use user setting location!")
    set(VULKAN_INCLUDE_DIR "${VULKAN_USER_DIR}/Include")
    set(VULKAN_LIB_LOCATION "${VULKAN_USER_DIR}/Lib/vulkan-1.lib")
    add_library(Vulkan::Vulkan STATIC IMPORTED GLOBAL)
    set_target_properties(
        Vulkan::Vulkan
        PROPERTIES
            IMPORTED_LOCATION ${VULKAN_LIB_LOCATION}
            INTERFACE_INCLUDE_DIRECTORIES ${VULKAN_INCLUDE_DIR}
    )
else()
    message("vulkan find!")
endif()