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
#include <vma/vk_mem_alloc.h>
#include <vulkan/vk_enum_string_helper.h>
#include "VkBootstrap.h"
#include "VkBootstrapDispatch.h"
#include <glm/glm.hpp>
#include "include/fmt/include/fmt/core.h"
#include <imgui.h>
#include "include/stb/stb_image.h"
#include "SDL3/SDL_vulkan.h"

#define VK_CHECK(x) \
do { \
VkResult err = x; \
if (err) { \
fmt::print("Detected Vulkan error: {}", string_VkResult(err)); \
abort(); \
} \
} while (0)

VkCommandPoolCreateInfo init_command_pool_create_info(uint32_t queueFamilyIndex,
    VkCommandPoolCreateFlags flags)
{
    VkCommandPoolCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.pNext = nullptr;
    info.queueFamilyIndex = queueFamilyIndex;
    info.flags = flags;
    return info;
}

VkFenceCreateInfo create_fence_info(VkFenceCreateFlags flags) {
    VkFenceCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = flags;
    return info;
}

VkSemaphoreCreateInfo create_semaphore_info(VkSemaphoreCreateFlags flags) {
    VkSemaphoreCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = flags;
    return info;
}

VkCommandBufferAllocateInfo init_command_buffer_allocate_info(
    VkCommandPool pool, uint32_t count)
{
    VkCommandBufferAllocateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.pNext = nullptr;

    info.commandPool = pool;
    info.commandBufferCount = count;
    info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    return info;
}

VkCommandBufferBeginInfo create_command_buffer_begin_info(VkCommandBufferUsageFlags flags) {
    VkCommandBufferBeginInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.pNext = nullptr;

    info.pInheritanceInfo = nullptr;
    info.flags = flags;
    return info;
}

VkImageSubresourceRange image_subresource_range(VkImageAspectFlags aspect_mask) {
    VkImageSubresourceRange sub_image = {};
    sub_image.aspectMask = aspect_mask;
    sub_image.baseMipLevel = 0;
    sub_image.levelCount = VK_REMAINING_MIP_LEVELS;
    sub_image.baseArrayLayer = 0;
    sub_image.layerCount = VK_REMAINING_ARRAY_LAYERS;
    return sub_image;
}

void transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout old_layout, VkImageLayout new_layout) {
    VkImageMemoryBarrier2 image_barrier = {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
    image_barrier.pNext = nullptr;

    image_barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    image_barrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
    image_barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    image_barrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

    image_barrier.oldLayout = old_layout;
    image_barrier.newLayout = new_layout;

    VkImageAspectFlags aspect_mask = (new_layout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    image_barrier.subresourceRange = image_subresource_range(aspect_mask);
    image_barrier.image = image;

    VkDependencyInfo dep_info = {};
    dep_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep_info.pNext = nullptr;

    dep_info.imageMemoryBarrierCount = 1;
    dep_info.pImageMemoryBarriers = &image_barrier;

    vkCmdPipelineBarrier2(cmd, &dep_info);
}

VkSemaphoreSubmitInfo make_semaphore_submit_info(VkPipelineStageFlags2 stage_mask, VkSemaphore semaphore)
{
    VkSemaphoreSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    submit_info.pNext = nullptr;
    submit_info.semaphore = semaphore;
    submit_info.stageMask = stage_mask;
    submit_info.deviceIndex = 0;
    submit_info.value = 1;

    return submit_info;
};

VkCommandBufferSubmitInfo make_command_buffer_submit_info(VkCommandBuffer cmd)
{
    VkCommandBufferSubmitInfo info{};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    info.pNext = nullptr;
    info.commandBuffer = cmd;
    info.deviceMask = 0;

    return info;
}

VkSubmitInfo2 submit_info(VkCommandBufferSubmitInfo* cmd, VkSemaphoreSubmitInfo* signal_semaphore_info, VkSemaphoreSubmitInfo* wait_semaphore_info)
{
    VkSubmitInfo2 info = {};
    info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    info.pNext = nullptr;

    info.waitSemaphoreInfoCount = wait_semaphore_info == nullptr ? 0 : 1;
    info.pWaitSemaphoreInfos = wait_semaphore_info;

    info.signalSemaphoreInfoCount = signal_semaphore_info == nullptr ? 0 : 1;
    info.pSignalSemaphoreInfos = signal_semaphore_info;

    info.commandBufferInfoCount = 1;
    info.pCommandBufferInfos = cmd;

    return info;
}

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
        VkSemaphore _swapchain_semaphore, _render_semaphore;
        VkFence _render_fence;
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
        VkCommandPoolCreateInfo command_pool_info = init_command_pool_create_info(_graphics_queue_family, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

        for (int i = 0; i < FRAME_OVERLAP; i++) {
            VK_CHECK(vkCreateCommandPool(_vk_device, &command_pool_info, nullptr, &_frames[i]._commandPool));

            VkCommandBufferAllocateInfo cmd_alloc_info = init_command_buffer_allocate_info(_frames[i]._commandPool, 1);

            VK_CHECK(vkAllocateCommandBuffers(_vk_device, &cmd_alloc_info, &_frames[i]._mainCommandBuffer));
        }
    }

    void init_sync_structures() {
        VkFenceCreateInfo fence_create_info = create_fence_info(VK_FENCE_CREATE_SIGNALED_BIT);
        VkSemaphoreCreateInfo semaphore_create_info = create_semaphore_info(0);

        for (int i = 0; i < FRAME_OVERLAP; i++) {
            VK_CHECK(vkCreateFence(_vk_device, &fence_create_info, nullptr, &_frames[i]._render_fence));

            VK_CHECK(vkCreateSemaphore(_vk_device, &semaphore_create_info, nullptr, &_frames[i]._swapchain_semaphore));
            VK_CHECK(vkCreateSemaphore(_vk_device, &semaphore_create_info, nullptr, &_frames[i]._render_semaphore));
        }
    }

    void init() {
        // --- SDL SETUP ---
        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_AUDIO);
        SDL_Rect display_bounds;
        SDL_GetDisplayBounds(SDL_GetPrimaryDisplay(), &display_bounds);
        window = SDL_CreateWindow("GVK RENDERER", display_bounds.w, display_bounds.h, SDL_WINDOW_VULKAN | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_FULLSCREEN);

        // --- VULKAN SETUP ---
        init_vulkan();
        init_swapchain();
        init_commands();
        init_sync_structures();
    }

    void draw() {
        VK_CHECK(vkWaitForFences(_vk_device, 1, &get_current_frame()._render_fence, true, 1000000000));
        VK_CHECK(vkResetFences(_vk_device, 1, &get_current_frame()._render_fence));

        uint32_t swapchain_image_index;
        VK_CHECK(vkAcquireNextImageKHR(_vk_device, _swapchain, 1000000000, get_current_frame()._swapchain_semaphore, nullptr, &swapchain_image_index));

        VkCommandBuffer cmd = get_current_frame()._mainCommandBuffer;
        VK_CHECK(vkResetCommandBuffer(cmd, 0));
        VkCommandBufferBeginInfo cmd_begin_info = create_command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        VK_CHECK(vkBeginCommandBuffer(cmd, &cmd_begin_info));

        // turn swapchain image writeable
        transition_image(cmd, _swapchain_images[swapchain_image_index], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

        // useless testing stuff
        VkClearColorValue clear_color_value;
        float flash = std::abs(std::sin(_frame_number / 120.f));
        clear_color_value = {{0.0f, 0.0f, flash, 1.0f}};

        VkImageSubresourceRange clear_range = image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);

        // clear image
        vkCmdClearColorImage(cmd, _swapchain_images[swapchain_image_index], VK_IMAGE_LAYOUT_GENERAL, &clear_color_value, 1, &clear_range);

        // turn swapchain image presentable
        transition_image(cmd, _swapchain_images[swapchain_image_index], VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        // finalize command buffer
        VK_CHECK(vkEndCommandBuffer(cmd));

        // prepare submission to queue
        VkCommandBufferSubmitInfo cmd_info = make_command_buffer_submit_info(cmd);
        VkSemaphoreSubmitInfo wait_info = make_semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,get_current_frame()._swapchain_semaphore);
        VkSemaphoreSubmitInfo signal_info = make_semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, get_current_frame()._render_semaphore);
        VkSubmitInfo2 submit = submit_info(&cmd_info, &signal_info, &wait_info);

        // submit command buffer to the queue and execute it; commands are blocked until this is over
        VK_CHECK(vkQueueSubmit2(_graphics_queue, 1, &submit, get_current_frame()._render_fence));

        // prepare presetation
        VkPresentInfoKHR present_info = {};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.pNext = nullptr;
        present_info.pSwapchains = &_swapchain;
        present_info.swapchainCount = 1;

        present_info.pWaitSemaphores = &get_current_frame()._render_semaphore;
        present_info.waitSemaphoreCount = 1;

        present_info.pImageIndices = &swapchain_image_index;

        VK_CHECK(vkQueuePresentKHR(_graphics_queue, &present_info));

        _frame_number++;
    }

    void quit() {
        destroy_swapchain();

        vkDeviceWaitIdle(_vk_device);
        for (int i = 0; i < FRAME_OVERLAP; i++) {
            vkDestroyCommandPool(_vk_device, _frames[i]._commandPool, nullptr);

            vkDestroyFence(_vk_device, _frames[i]._render_fence, nullptr);
            vkDestroySemaphore(_vk_device, _frames[i]._render_semaphore, nullptr);
            vkDestroySemaphore(_vk_device, _frames[i]._swapchain_semaphore, nullptr);
        }

        vkDestroySurfaceKHR(_vk_instance, _vk_surface, nullptr);
        vkDestroyDevice(_vk_device, nullptr);

        vkb::destroy_debug_utils_messenger(_vk_instance, _debug_messenger);
        vkDestroyInstance(_vk_instance, nullptr);

        SDL_DestroyWindow(window);
        SDL_Quit();
    }
}

#endif