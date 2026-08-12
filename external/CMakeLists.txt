cmake_minimum_required(VERSION 3.20)

find_package(Vulkan REQUIRED)
find_package(SDL2 CONFIG REQUIRED)

# =====================================================
# vkbootstrap
# =====================================================

add_library(vkbootstrap STATIC)

target_sources(vkbootstrap PRIVATE
    vkbootstrap/VkBootstrap.cpp
    vkbootstrap/VkBootstrap.h
)

target_include_directories(vkbootstrap PUBLIC
    vkbootstrap
)

target_link_libraries(vkbootstrap PUBLIC
    Vulkan::Vulkan
    $<$<BOOL:UNIX>:${CMAKE_DL_LIBS}>
)

# =====================================================
# Vulkan Memory Allocator
# =====================================================

add_library(vma INTERFACE)

target_include_directories(vma INTERFACE
    vma
)

# =====================================================
# GLM
# =====================================================

add_library(glm INTERFACE)

target_include_directories(glm INTERFACE
    glm
)

# =====================================================
# stb_image
# =====================================================

add_library(stb_image INTERFACE)

target_include_directories(stb_image INTERFACE
    stb_image
)

# =====================================================
# tinyobjloader
# =====================================================

add_library(tinyobjloader STATIC)

target_sources(tinyobjloader PRIVATE
    tinyobjloader/tiny_obj_loader.cc
    tinyobjloader/tiny_obj_loader.h
)

target_include_directories(tinyobjloader PUBLIC
    tinyobjloader
)

# =====================================================
# SDL2 Interface
# =====================================================

add_library(sdl2 INTERFACE)

target_link_libraries(sdl2 INTERFACE
    SDL2::SDL2
)

# =====================================================
# ImGui
# =====================================================

add_library(imgui STATIC)

target_include_directories(imgui PUBLIC imgui)

target_sources(imgui PRIVATE
    imgui/imgui.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_widgets.cpp

    imgui/imgui_impl_sdl2.cpp
    imgui/imgui_impl_vulkan.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
)

target_link_libraries(imgui PUBLIC
    Vulkan::Vulkan
    sdl2
)

target_include_directories(imgui PUBLIC
    imgui
    imgui/backends
)


# add_subdirectory(fmt EXCLUDE_FROM_ALL)