cmake_minimum_required(VERSION 3.8.0)

project(02_ImageView
        LANGUAGES CXX
        DESCRIPTION "a test program made in vulkan")
set(TARGET "02_ImageView")

include(../cmake/FindVulkan.cmake)
include(../cmake/FindSDL.cmake)

set(INSTALL_PATH "${PROJECT_SOURCE_DIR}/bin")

file(GLOB SRC_LIST "./*.cpp")
source_group("source" FILES ${SRC_LIST})

file(GLOB  HEAD_LIST "./*.h" "./*.hpp")
source_group("include" FILES ${HEAD_LIST})

set(ALL_SOURCES
${SRC_LIST}
${HEAD_LIST}
)

add_executable(${TARGET} ${ALL_SOURCES})

# 设置MSVC的启动项，否则Cmake生成的工程默认启动的工程永远是ALL_BUILD
if (MSVC)
    if (NOT ${CMAKE_VERSION} VERSION_LESS "3.6.0")
          set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${TARGET})
    endif ()
endif (MSVC)

target_link_libraries(${TARGET} PUBLIC Vulkan::Vulkan SDL2::SDL2)

set_target_properties(${TARGET}
    PROPERTIES
    CXX_STANDARD 17
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${INSTALL_PATH}")