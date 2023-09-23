cmake_minimum_required(VERSION 3.8.0)

project(12_IndexBuffer_PushConstant
        LANGUAGES CXX
        DESCRIPTION "a test program made in vulkan")
set(TARGET "12_IndexBuffer_PushConstant")

include(../cmake/FindVulkan.cmake)
include(../cmake/FindSDL.cmake)


set(INSTALL_PATH "${PROJECT_SOURCE_DIR}/bin")
add_definitions(-DDIR_PATH="${CMAKE_SOURCE_DIR}/")

find_program(GLSLC_PROGRAM glslc REQUIRED)
execute_process(COMMAND ${GLSLC_PROGRAM} ${CMAKE_SOURCE_DIR}/shader/shader.vert -o ${INSTALL_PATH}/vert.spv)
execute_process(COMMAND ${GLSLC_PROGRAM} ${CMAKE_SOURCE_DIR}/shader/shader.frag -o ${INSTALL_PATH}/frag.spv)


file(GLOB SRC_LIST "./*.cpp" "./math/*.cpp")
source_group("source" FILES ${SRC_LIST})

file(GLOB  HEAD_LIST "./*.h" "./*.hpp" "./math/*.hpp")
source_group("include" FILES ${HEAD_LIST})

file(GLOB  SHADER_LIST "./shader/*.*")
source_group("shader" FILES ${SHADER_LIST})

set(ALL_SOURCES
${SRC_LIST}
${HEAD_LIST}
${SHADER_LIST}
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