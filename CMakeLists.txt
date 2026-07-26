cmake_minimum_required(VERSION 3.20)

project(Vulkan_tutorial VERSION 1.0 LANGUAGES CXX)


set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)


find_package(Vulkan REQUIRED)


add_subdirectory(external)


add_executable(${PROJECT_NAME}
    src/main.cpp
    src/vk_engine.cpp
    src/vk_initializers.cpp
    src/vk_images.cpp
)


target_include_directories(${PROJECT_NAME}
PRIVATE
    include
)



if(MSVC)

    target_compile_options(${PROJECT_NAME}
        PRIVATE
            /W4
    )

else()

    target_compile_options(${PROJECT_NAME}
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
    )

endif()


if(APPLE)

    target_compile_definitions(${PROJECT_NAME}
        PRIVATE
            VK_ENABLE_BETA_EXTENSIONS
    )

    find_library(COCOA_FRAMEWORK Cocoa)
    find_library(IOKIT_FRAMEWORK IOKit)
    find_library(COREVIDEO_FRAMEWORK CoreVideo)
    find_library(METAL_FRAMEWORK Metal)
    find_library(QUARTZCORE_FRAMEWORK QuartzCore)

endif()


target_link_libraries(${PROJECT_NAME}
PRIVATE
    Vulkan::Vulkan

    vkbootstrap
    vma
    glm
    tinyobjloader
    imgui
    stb_image
    sdl2

    ${COCOA_FRAMEWORK}
    ${IOKIT_FRAMEWORK}
    ${COREVIDEO_FRAMEWORK}
    ${METAL_FRAMEWORK}
    ${QUARTZCORE_FRAMEWORK}
)



if(UNIX AND NOT APPLE)

    target_link_libraries(${PROJECT_NAME}
    PRIVATE
        dl
        pthread
        X11
        Xrandr
        Xi
        Xxf86vm
    )

endif()


set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set_property(
    TARGET ${PROJECT_NAME}
    PROPERTY VS_DEBUGGER_WORKING_DIRECTORY
    $<TARGET_FILE_DIR:${PROJECT_NAME}>
)


message(STATUS "")
message(STATUS "-------------------------------------")
message(STATUS "Project       : ${PROJECT_NAME}")
message(STATUS "System        : ${CMAKE_SYSTEM_NAME}")
message(STATUS "Compiler      : ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Build Type    : ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard  : ${CMAKE_CXX_STANDARD}")
message(STATUS "-------------------------------------")
message(STATUS "")