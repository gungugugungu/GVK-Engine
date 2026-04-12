#pragma once
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <span>
#include <array>
#include <functional>
#include <deque>
#include <SDL3/SDL.h>
#include <vulkan/vulkan.h>
#include "VkBootstrap.h"
#include <glm/glm.hpp>
#include <imgui.h>
#include "include/stb/stb_image.h"
#include "SDL3/SDL_vulkan.h"

namespace gvk {
    void init();
    void quit();
}

#ifdef GVK_IMPLEMENTATION
#define GVK_IMPLEMENTATION_INCLUDED

namespace gvk {
    inline SDL_Window* window = nullptr;

    inline VkInstance _vk_instance;
    inline VkPhysicalDevice _chosen_GPU;
    inline VkDevice _vk_device;
    inline VkDebugUtilsMessengerEXT _debug_messenger;
    inline VkSurfaceKHR _vk_surface;
    inline VkAllocationCallbacks* _vk_allocation_callbacks = nullptr;

    inline VkSwapchainKHR _swapchain;
    inline VkFormat _swapchain_image_format;

    inline std::vector<VkImage> _swapchain_images;
    inline std::vector<VkImageView> _swapchain_image_views;
    inline VkExtent2D _swapchain_extent;

    struct FrameData {
        VkCommandPool _commandPool;
        VkCommandBuffer _mainCommandBuffer;
    };
    constexpr unsigned int FRAME_OVERLAP = 2;

    inline uint32_t _frame_number{0};
    inline FrameData _frames[FRAME_OVERLAP];
    inline FrameData& get_current_frame() { return _frames[_frame_number % FRAME_OVERLAP]; };
    inline VkQueue _graphics_queue;
    inline uint32_t _graphics_queue_family;

    void init_vulkan() {
        vkb::InstanceBuilder builder;

        auto inst_ret = builder.set_app_name("GVK RENDERER")
            .request_validation_layers(true) // TODO: When the project is done, set to false
            .use_default_debug_messenger()
            .require_api_version(1, 3, 0)
            .build();

        vkb::Instance vkb_inst = inst_ret.value();

        _vk_instance = vkb_inst.instance;
        _debug_messenger = vkb_inst.debug_messenger;

        SDL_Vulkan_CreateSurface(window, _vk_instance, _vk_allocation_callbacks, &_vk_surface);

        VkPhysicalDeviceVulkan13Features features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
        features.dynamicRendering = true;
        features.synchronization2 = true;

        VkPhysicalDeviceVulkan12Features features12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
        features12.bufferDeviceAddress = true;
        features12.descriptorIndexing = true;

        vkb::PhysicalDeviceSelector selector{vkb_inst};
        vkb::PhysicalDevice physical_device = selector
            .set_minimum_version(1, 3)
            .set_required_features_13(features)
            .set_required_features_12(features12)
            .set_surface(_vk_surface)
            .select()
            .value();

        vkb::DeviceBuilder device_builder{physical_device};
        vkb::Device vkb_device = device_builder.build().value();

        _vk_device = vkb_device.device;
        _chosen_GPU = physical_device.physical_device;

        _graphics_queue = vkb_device.get_queue(vkb::QueueType::graphics).value();
        _graphics_queue_family = vkb_device.get_queue_index(vkb::QueueType::graphics).value();
    }

    void create_swapchain(uint32_t width, uint32_t height) {
        vkb::SwapchainBuilder swapchain_builder{_chosen_GPU, _vk_device, _vk_surface};

        _swapchain_image_format = VK_FORMAT_B8G8R8A8_SRGB;

        vkb::Swapchain vkb_swapchain = swapchain_builder
            .set_desired_format(VkSurfaceFormatKHR{.format = _swapchain_image_format, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
            .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
            .set_desired_extent(width, height)
            .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
            .build()
            .value();

        _swapchain_extent = vkb_swapchain.extent;
        _swapchain = vkb_swapchain.swapchain;
        _swapchain_images = vkb_swapchain.get_images().value();
        _swapchain_image_views = vkb_swapchain.get_image_views().value();
    }

    void destroy_swapchain() {
        vkDestroySwapchainKHR(_vk_device, _swapchain, nullptr);
        for (int i = 0; i < _swapchain_image_views.size(); i++) {

            vkDestroyImageView(_vk_device, _swapchain_image_views[i], nullptr);
        }
    }

    void init_swapchain() {
        int w_width, w_height;
        SDL_GetWindowSize(window, &w_width, &w_height);
        create_swapchain(w_width, w_height);
    }
    void init_commands() {
        VkCommandPoolCreateInfo command_pool_info = {};
        command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        command_pool_info.pNext = nullptr;
        command_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        command_pool_info.queueFamilyIndex = _graphics_queue_family;

        for (int i = 0; i < FRAME_OVERLAP; i++) {
        }
    }
    void init_sync_structures();

    void init() {
        // --- SDL SETUP ---
        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_AUDIO);
        SDL_Rect display_bounds;
        SDL_GetDisplayBounds(SDL_GetPrimaryDisplay(), &display_bounds);
        window = SDL_CreateWindow("GVK RENDERER", display_bounds.w, display_bounds.h, SDL_WINDOW_VULKAN | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_FULLSCREEN);

        // --- VULKAN SETUP ---
        init_vulkan();
        init_swapchain();
        /*init_commands();
        init_sync_structures();*/
    }

    void quit() {
        destroy_swapchain();

        vkDestroySurfaceKHR(_vk_instance, _vk_surface, nullptr);
        vkDestroyDevice(_vk_device, nullptr);

        vkb::destroy_debug_utils_messenger(_vk_instance, _debug_messenger);
        vkDestroyInstance(_vk_instance, nullptr);

        SDL_DestroyWindow(window);
        SDL_Quit();
    }
}

#endif