cmake_minimum_required(VERSION 3.20)

project(EuclaseSound LANGUAGES C CXX)

find_package(Vulkan REQUIRED)
set(SHADERC_SKIP_TESTS ON CACHE BOOL "" FORCE)
set(SHADERC_SKIP_EXAMPLES ON CACHE BOOL "" FORCE)
add_subdirectory(submodules/SPIRV-Headers)
add_subdirectory(submodules/SPIRV-Tools)
add_subdirectory(submodules/glslang)

add_subdirectory(submodules/shaderc)

# -------------------------------------------------------------------------
# Common sources (platform-independent)
# -------------------------------------------------------------------------
set(COMMON_SOURCES
        main.cpp
        platform/Platform.cpp
        platform/Event.cpp

        Renderer/Vulkan/vulkanContext.cpp
        Renderer/Vulkan/vulkanRenderer.cpp
        Renderer/Vulkan/vulkanDevice.cpp
        Renderer/Vulkan/vulkanDevice.h

        Renderer/Vulkan/platform/vulkanPlatform.h
        Renderer/Vulkan/platform/vulkanPlatform.cpp

        Renderer/Vulkan/VulkanSwapchain.cpp
        Renderer/Vulkan/VulkanSwapchain.h

        Renderer/Vulkan/VulkanCommandBuffer.cpp
        Renderer/Vulkan/VulkanCommandBuffer.h

        Renderer/Vulkan/vulkanPipeline.cpp
        Renderer/Vulkan/vulkanPipeline.h

        Renderer/CommandBuffer.h
        Renderer/GraphicsTypes.h
        Renderer/GraphicsDevice.h
        Renderer/GraphicsRenderer.h
        Renderer/GraphicsPipeline.h

        Renderer/RendererFactory.cpp
        Renderer/RendererFactory.h
)

add_executable(EuclaseSound ${COMMON_SOURCES})

target_include_directories(EuclaseSound PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(EuclaseSound PRIVATE
        Vulkan::Vulkan
        shaderc_combined
)

# -------------------------------------------------------------------------
# Linux / Wayland
# -------------------------------------------------------------------------
if(UNIX AND NOT APPLE)

    find_package(PkgConfig REQUIRED)

    pkg_check_modules(WAYLAND REQUIRED wayland-client)

    find_program(WAYLAND_SCANNER wayland-scanner REQUIRED)

    # ---------------------------------------------------------------------
    # xdg-shell
    # ---------------------------------------------------------------------

    set(XDG_SHELL_XML
            /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml
    )

    set(XDG_SHELL_HEADER
            ${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-client-protocol.h
    )

    set(XDG_SHELL_CODE
            ${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-protocol.c
    )

    add_custom_command(
            OUTPUT
            ${XDG_SHELL_HEADER}
            ${XDG_SHELL_CODE}

            COMMAND
            ${WAYLAND_SCANNER}
            client-header
            ${XDG_SHELL_XML}
            ${XDG_SHELL_HEADER}

            COMMAND
            ${WAYLAND_SCANNER}
            private-code
            ${XDG_SHELL_XML}
            ${XDG_SHELL_CODE}

            DEPENDS
            ${XDG_SHELL_XML}
    )

    add_custom_target(
            xdg-shell
            DEPENDS
            ${XDG_SHELL_HEADER}
            ${XDG_SHELL_CODE}
    )

    # ---------------------------------------------------------------------
    # xdg-decoration
    # ---------------------------------------------------------------------

    set(XDG_DECORATION_XML
            /usr/share/wayland-protocols/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml
    )

    set(XDG_DECORATION_HEADER
            ${CMAKE_CURRENT_BINARY_DIR}/xdg-decoration-client-protocol.h
    )

    set(XDG_DECORATION_CODE
            ${CMAKE_CURRENT_BINARY_DIR}/xdg-decoration-protocol.c
    )

    add_custom_command(
            OUTPUT
            ${XDG_DECORATION_HEADER}
            ${XDG_DECORATION_CODE}

            COMMAND
            ${WAYLAND_SCANNER}
            client-header
            ${XDG_DECORATION_XML}
            ${XDG_DECORATION_HEADER}

            COMMAND
            ${WAYLAND_SCANNER}
            private-code
            ${XDG_DECORATION_XML}
            ${XDG_DECORATION_CODE}

            DEPENDS
            ${XDG_DECORATION_XML}
    )

    add_custom_target(
            xdg-decoration
            DEPENDS
            ${XDG_DECORATION_HEADER}
            ${XDG_DECORATION_CODE}
    )

    target_sources(EuclaseSound PRIVATE
            platform/wayland/Platform_Wayland.cpp
            platform/wayland/WaylandBuffer.cpp
            platform/wayland/WaylandDisplay.cpp
            platform/wayland/WaylandWindow.cpp

            Renderer/Vulkan/platform/waylandVulkan.cpp
            Renderer/Vulkan/platform/waylandVulkan.h

            ${XDG_SHELL_CODE}
            ${XDG_DECORATION_CODE}
    )

    target_compile_definitions(EuclaseSound PRIVATE
            VK_USE_PLATFORM_WAYLAND_KHR
    )

    add_dependencies(EuclaseSound
            xdg-shell
            xdg-decoration
    )

    target_include_directories(EuclaseSound PRIVATE
            ${WAYLAND_INCLUDE_DIRS}
    )

    target_link_libraries(EuclaseSound PRIVATE
            ${WAYLAND_LIBRARIES}
    )

    # -------------------------------------------------------------------------
    # Windows / Win32
    # -------------------------------------------------------------------------
elseif(WIN32)

    target_sources(EuclaseSound PRIVATE
            platform/win32/Platform_Win32.cpp
            platform/win32/Platform_Win32.h

            Renderer/Vulkan/platform/win32Vulkan.cpp
            Renderer/Vulkan/platform/win32Vulkan.h
    )

    target_compile_definitions(EuclaseSound PRIVATE
            VK_USE_PLATFORM_WIN32_KHR
            NOMINMAX
            WIN32_LEAN_AND_MEAN
    )

    target_link_libraries(EuclaseSound PRIVATE
            user32
            gdi32
    )

else()

    message(FATAL_ERROR
            "EuclaseSound: unsupported platform"
    )

endif()