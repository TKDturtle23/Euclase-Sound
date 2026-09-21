cmake_minimum_required(VERSION 3.20)

project(EuclaseSound LANGUAGES C CXX)

find_package(PkgConfig REQUIRED)
find_package(Vulkan REQUIRED)
pkg_check_modules(WAYLAND REQUIRED wayland-client)

find_program(WAYLAND_SCANNER wayland-scanner REQUIRED)

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

        COMMAND ${WAYLAND_SCANNER}
        client-header
        ${XDG_SHELL_XML}
        ${XDG_SHELL_HEADER}

        COMMAND ${WAYLAND_SCANNER}
        private-code
        ${XDG_SHELL_XML}
        ${XDG_SHELL_CODE}

        DEPENDS
        ${XDG_SHELL_XML}
)

add_custom_target(xdg-shell
        DEPENDS
        ${XDG_SHELL_HEADER}
        ${XDG_SHELL_CODE}
)
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

        COMMAND ${WAYLAND_SCANNER}
        client-header
        ${XDG_DECORATION_XML}
        ${XDG_DECORATION_HEADER}

        COMMAND ${WAYLAND_SCANNER}
        private-code
        ${XDG_DECORATION_XML}
        ${XDG_DECORATION_CODE}

        DEPENDS
        ${XDG_DECORATION_XML}
)

add_custom_target(xdg-decoration
        DEPENDS
        ${XDG_DECORATION_HEADER}
        ${XDG_DECORATION_CODE}
)

add_executable(EuclaseSound
        platform/wayland/Platform_Wayland.cpp
        platform/Platform.cpp
        main.cpp
        platform/wayland/WaylandBuffer.cpp
        platform/wayland/WaylandDisplay.cpp
        platform/wayland/WaylandWindow.cpp
        platform/Event.cpp
        ${XDG_SHELL_CODE}
        ${XDG_DECORATION_CODE}
        Renderer/Vulkan/vulkanContext.cpp
        Renderer/Vulkan/vulkanRenderer.cpp
        Renderer/Vulkan/platform/vulkanPlatform.h
        Renderer/Vulkan/platform/waylandVulkan.cpp
        Renderer/Vulkan/platform/waylandVulkan.h
               Renderer/Vulkan/vulkanDevice.cpp
        Renderer/Vulkan/vulkanDevice.h
)
target_compile_definitions(EuclaseSound
        PRIVATE
        VK_USE_PLATFORM_WAYLAND_KHR
)
add_dependencies(EuclaseSound xdg-shell xdg-decoration)

target_include_directories(EuclaseSound PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
        ${WAYLAND_INCLUDE_DIRS}
)

target_link_libraries(EuclaseSound PRIVATE
        ${WAYLAND_LIBRARIES} Vulkan::Vulkan
)