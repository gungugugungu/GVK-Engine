#pragma once
#define VMA_IMPLEMENTATION
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <span>
#include <array>
#include <functional>
#include <deque>
#include <fstream>
#include <SDL3/SDL.h>
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vk_enum_string_helper.h>
#include "VkBootstrap.h"
#include "VkBootstrapDispatch.h"
#include <glm/glm.hpp>
#include "include/fmt/include/fmt/core.h"
#include <imgui.h>
#include "fmt/chrono.h"
#include "include/stb/stb_image.h"
#include "include/stb/stb_truetype.h"
#include "include/stb/stb_image_resize2.h"
#include "SDL3/SDL_vulkan.h"
#include "imgui.h"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_vulkan.h"
#include <unordered_map>
#include <filesystem>
#include <iostream>
#include <glm/gtx/quaternion.hpp>
#include "include/glm/glm/gtc/matrix_access.hpp"
#include "include/tinygltf/tiny_gltf.h"

#define VK_CHECK(x) \
do { \
VkResult err = x; \
if (err) { \
fmt::println("Detected Vulkan error: {}", string_VkResult(err)); \
abort(); \
} \
} while (0)

using namespace std;

// hard-coded cube for skybxo
std::vector<uint32_t> skybox_indices {
    //Top
    2, 6, 7,
    2, 3, 7,

    //Bottom
    0, 4, 5,
    0, 1, 5,

    //Left
    0, 2, 6,
    0, 4, 6,

    //Right
    1, 3, 7,
    1, 5, 7,

    //Front
    0, 2, 3,
    0, 1, 3,

    //Back
    4, 6, 7,
    4, 5, 7
};


std::vector<float> skybox_vertices {
    -1, -1,  0.5, //0
     1, -1,  0.5, //1
    -1,  1,  0.5, //2
     1,  1,  0.5, //3
    -1, -1, -0.5, //4
     1, -1, -0.5, //5
    -1,  1, -0.5, //6
     1,  1, -0.5  //7
};

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

void transition_image_mip(VkCommandBuffer cmd, VkImage image, VkImageLayout old_layout, VkImageLayout new_layout, uint32_t mip_level) {
    VkImageMemoryBarrier2 barrier = {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
    barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    barrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
    barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    barrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = mip_level;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkDependencyInfo dep = {.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dep.imageMemoryBarrierCount = 1;
    dep.pImageMemoryBarriers    = &barrier;
    vkCmdPipelineBarrier2(cmd, &dep);
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

VkImageCreateInfo image_create_info(VkFormat format, VkImageUsageFlags usage_flags, VkExtent3D extent) {
    VkImageCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.pNext = nullptr;

    info.imageType = VK_IMAGE_TYPE_2D;

    info.format = format;
    info.extent = extent;

    info.mipLevels = 1;
    info.arrayLayers = 1;

    info.samples = VK_SAMPLE_COUNT_1_BIT; // for MSAA
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.usage = usage_flags;

    return info;
}

VkImageViewCreateInfo imageview_create_info(VkFormat format, VkImage image, uint32_t mipmaps, VkImageAspectFlags aspectFlags) {
    VkImageViewCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.pNext = nullptr;

    info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    info.image = image;
    info.format = format;
    info.subresourceRange.baseMipLevel = 0;
    info.subresourceRange.levelCount = mipmaps == 0 ? 1 : mipmaps;
    info.subresourceRange.baseArrayLayer = 0;
    info.subresourceRange.layerCount = 1;
    info.subresourceRange.aspectMask = aspectFlags;

    return info;
}

VkImageViewCreateInfo cubemap_imageview_create_info(VkFormat format, VkImage image) {
    VkImageViewCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.pNext = nullptr;
    info.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    info.image = image;
    info.format = format;
    info.subresourceRange.baseMipLevel = 0;
    info.subresourceRange.levelCount = 1;
    info.subresourceRange.baseArrayLayer = 0;
    info.subresourceRange.layerCount = 6;
    info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    return info;
}

void copy_image_to_image(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize)
{
    VkImageBlit2 blitRegion{ .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2, .pNext = nullptr };

    blitRegion.srcOffsets[1].x = srcSize.width;
    blitRegion.srcOffsets[1].y = srcSize.height;
    blitRegion.srcOffsets[1].z = 1;

    blitRegion.dstOffsets[1].x = dstSize.width;
    blitRegion.dstOffsets[1].y = dstSize.height;
    blitRegion.dstOffsets[1].z = 1;

    blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blitRegion.srcSubresource.baseArrayLayer = 0;
    blitRegion.srcSubresource.layerCount = 1;
    blitRegion.srcSubresource.mipLevel = 0;

    blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blitRegion.dstSubresource.baseArrayLayer = 0;
    blitRegion.dstSubresource.layerCount = 1;
    blitRegion.dstSubresource.mipLevel = 0;

    VkBlitImageInfo2 blitInfo{ .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2, .pNext = nullptr };
    blitInfo.dstImage = destination;
    blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    blitInfo.srcImage = source;
    blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    blitInfo.filter = VK_FILTER_LINEAR;
    blitInfo.regionCount = 1;
    blitInfo.pRegions = &blitRegion;

    vkCmdBlitImage2(cmd, &blitInfo);
}

struct DeletionQueue
{
    deque<function<void()>> deletors;

    void push_function(function<void()>&& function) {
        deletors.push_back(function);
    }

    void flush() {
        for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
            (*it)();
        }

        deletors.clear();
    }
};

struct AllocatedImage {
    VkImage image = VK_NULL_HANDLE;
    VkImageView image_view = VK_NULL_HANDLE;
    VmaAllocation allocation = nullptr;
    VkExtent3D extent = {};
    VkFormat format = VK_FORMAT_UNDEFINED;
    uint32_t mipmaps = 1;
};

struct DescriptorLayoutBuilder {
    vector<VkDescriptorSetLayoutBinding> bindings;

    void add_binding(uint32_t binding, VkDescriptorType type);
    void clear();
    VkDescriptorSetLayout build(VkDevice device, VkShaderStageFlags shaderStages, void* pNext = nullptr, VkDescriptorSetLayoutCreateFlags flags = 0);
};

void DescriptorLayoutBuilder::add_binding(uint32_t binding, VkDescriptorType type)
{
    VkDescriptorSetLayoutBinding newbind {};
    newbind.binding = binding;
    newbind.descriptorCount = 1;
    newbind.descriptorType = type;

    bindings.push_back(newbind);
}

void DescriptorLayoutBuilder::clear()
{
    bindings.clear();
}

VkDescriptorSetLayout DescriptorLayoutBuilder::build(VkDevice device, VkShaderStageFlags shaderStages, void* pNext, VkDescriptorSetLayoutCreateFlags flags)
{
    for (auto& b : bindings) {
        b.stageFlags |= shaderStages;
    }

    VkDescriptorSetLayoutCreateInfo info = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    info.pNext = pNext;

    info.pBindings = bindings.data();
    info.bindingCount = (uint32_t)bindings.size();
    info.flags = flags;

    VkDescriptorSetLayout set;
    VK_CHECK(vkCreateDescriptorSetLayout(device, &info, nullptr, &set));

    return set;
}

struct DescriptorAllocator {

    struct PoolSizeRatio{
        VkDescriptorType type;
        float ratio;
    };

    VkDescriptorPool pool;

    void init_pool(VkDevice device, uint32_t maxSets, span<PoolSizeRatio> poolRatios);
    void clear_descriptors(VkDevice device);
    void destroy_pool(VkDevice device);

    VkDescriptorSet allocate(VkDevice device, VkDescriptorSetLayout layout);
};

void DescriptorAllocator::init_pool(VkDevice device, uint32_t maxSets, span<PoolSizeRatio> poolRatios)
{
    vector<VkDescriptorPoolSize> poolSizes;
    for (PoolSizeRatio ratio : poolRatios) {
        poolSizes.push_back(VkDescriptorPoolSize{
            .type = ratio.type,
            .descriptorCount = uint32_t(ratio.ratio * maxSets)
        });
    }

    VkDescriptorPoolCreateInfo pool_info = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    pool_info.flags = 0;
    pool_info.maxSets = maxSets;
    pool_info.poolSizeCount = (uint32_t)poolSizes.size();
    pool_info.pPoolSizes = poolSizes.data();

    vkCreateDescriptorPool(device, &pool_info, nullptr, &pool);
}

void DescriptorAllocator::clear_descriptors(VkDevice device)
{
    vkResetDescriptorPool(device, pool, 0);
}

void DescriptorAllocator::destroy_pool(VkDevice device)
{
    vkDestroyDescriptorPool(device,pool,nullptr);
}

VkDescriptorSet DescriptorAllocator::allocate(VkDevice device, VkDescriptorSetLayout layout)
{
    VkDescriptorSetAllocateInfo allocInfo = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocInfo.pNext = nullptr;
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;

    VkDescriptorSet ds;
    VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, &ds));

    return ds;
}

VkRenderingAttachmentInfo attachment_info(
    VkImageView view, VkClearValue* clear ,VkImageLayout layout /*= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL*/)
{
    VkRenderingAttachmentInfo colorAttachment {};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.pNext = nullptr;

    colorAttachment.imageView = view;
    colorAttachment.imageLayout = layout;
    colorAttachment.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    if (clear) {
        colorAttachment.clearValue = *clear;
    }

    return colorAttachment;
}

bool load_shader_module(const char* file_path, VkDevice device, VkShaderModule* out_shader_module) {
    ifstream file(file_path, ios::ate | ios::binary);

    if (!file.is_open()) {
        return false;
    }

    size_t file_size = (size_t)file.tellg();

    vector<uint32_t> buffer(file_size / sizeof(uint32_t));

    file.seekg(0);

    file.read((char*)buffer.data(), file_size);

    file.close();

    VkShaderModuleCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.pNext = nullptr;

    create_info.codeSize = buffer.size() * sizeof(uint32_t);
    create_info.pCode = buffer.data();

    VkShaderModule shader_module;
    if (vkCreateShaderModule(device, &create_info, nullptr, &shader_module) != VK_SUCCESS) {
        return false;
    }
    *out_shader_module = shader_module;
    return true;
}

VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info(VkShaderStageFlagBits stage, VkShaderModule shader_module) {
    VkPipelineShaderStageCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info.pNext = nullptr;
    info.stage = stage;
    info.module = shader_module;
    info.pName = "main";
    return info;
}

VkPipelineLayoutCreateInfo pipeline_layout_create_info() {
    VkPipelineLayoutCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = 0;
    info.setLayoutCount = 0;
    info.pSetLayouts = nullptr;
    info.pushConstantRangeCount = 0;
    info.pPushConstantRanges = nullptr;
    return info;
}

VkRenderingInfo rendering_info(VkExtent2D render_extent, VkRenderingAttachmentInfo* color_attachment, VkRenderingAttachmentInfo* depth_attachment) {
    VkRenderingInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    info.pNext = nullptr;
    info.renderArea = VkRect2D{VkOffset2D{0, 0}, render_extent};
    info.layerCount = 1;
    info.colorAttachmentCount = 1;
    info.pColorAttachments = color_attachment;
    info.pDepthAttachment = depth_attachment;
    info.pStencilAttachment = nullptr;
    return info;
}

struct PipelineBuilder {
    vector<VkPipelineShaderStageCreateInfo> _shader_stages;
    VkPipelineInputAssemblyStateCreateInfo _input_assembly;
    VkPipelineRasterizationStateCreateInfo _rasterizer;
    VkPipelineColorBlendAttachmentState _color_blend_attachment;
    VkPipelineMultisampleStateCreateInfo _multisampling;
    VkPipelineLayout _pipeline_layout;
    VkPipelineDepthStencilStateCreateInfo _depth_stencil;
    VkPipelineRenderingCreateInfo _render_info;
    VkFormat _color_attachment_format;

    PipelineBuilder() { clear(); }

    void clear() {
        _input_assembly   = {.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
        _rasterizer       = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
        _color_blend_attachment = {};
        _multisampling    = {.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
        _pipeline_layout  = {};
        _depth_stencil    = {.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
        _render_info      = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
        _shader_stages.clear();
    }

    VkPipeline build_pipeline(VkDevice device) {
        VkPipelineViewportStateCreateInfo viewport_state = {};
        viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_state.viewportCount = 1;
        viewport_state.scissorCount = 1;

        VkPipelineColorBlendStateCreateInfo color_blending = {};
        color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blending.logicOpEnable = VK_FALSE;
        color_blending.logicOp = VK_LOGIC_OP_COPY;
        color_blending.attachmentCount = 1;
        color_blending.pAttachments = &_color_blend_attachment;

        VkPipelineVertexInputStateCreateInfo vertex_input_info = {.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};

        VkGraphicsPipelineCreateInfo pipeline_info = {.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
        pipeline_info.pNext = &_render_info;
        pipeline_info.stageCount = (uint32_t)_shader_stages.size();
        pipeline_info.pStages = _shader_stages.data();
        pipeline_info.pVertexInputState = &vertex_input_info;
        pipeline_info.pInputAssemblyState = &_input_assembly;
        pipeline_info.pViewportState = &viewport_state;
        pipeline_info.pRasterizationState = &_rasterizer;
        pipeline_info.pMultisampleState = &_multisampling;
        pipeline_info.pColorBlendState = &color_blending;
        pipeline_info.pDepthStencilState = &_depth_stencil;
        pipeline_info.layout = _pipeline_layout;

        VkDynamicState dyn_states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamic_info = {.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
        dynamic_info.pDynamicStates = &dyn_states[0];
        dynamic_info.dynamicStateCount = 2;
        pipeline_info.pDynamicState = &dynamic_info;

        VkPipeline new_pipeline;
        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &new_pipeline) != VK_SUCCESS) {
            fmt::println("failed to create graphics pipeline");
            return VK_NULL_HANDLE;
        }
        return new_pipeline;
    }

    void set_shaders(VkShaderModule vert, VkShaderModule frag) {
        _shader_stages.clear();
        _shader_stages.push_back(pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vert));
        _shader_stages.push_back(pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, frag));
    }

    void set_input_topology(VkPrimitiveTopology topology) {
        _input_assembly.topology = topology;
        _input_assembly.primitiveRestartEnable = VK_FALSE;
    }

    void set_polygon_mode(VkPolygonMode mode) {
        _rasterizer.polygonMode = mode;
        _rasterizer.lineWidth = 1.f;
    }

    void set_cull_mode(VkCullModeFlags cull_mode, VkFrontFace front_face) {
        _rasterizer.cullMode = cull_mode;
        _rasterizer.frontFace = front_face;
    }

    void set_multisampling_none() {
        _multisampling.sampleShadingEnable = VK_FALSE;
        _multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        _multisampling.minSampleShading = 1.f;
        _multisampling.pSampleMask = nullptr;
        _multisampling.alphaToCoverageEnable = VK_FALSE;
        _multisampling.alphaToOneEnable = VK_FALSE;
    }

    void enable_multisampling(VkSampleCountFlagBits sampleCount)
    {
        _multisampling.sampleShadingEnable = VK_FALSE;
        _multisampling.rasterizationSamples = sampleCount;
        _multisampling.minSampleShading = 1.0f;
        _multisampling.pSampleMask = nullptr;
        _multisampling.alphaToCoverageEnable = VK_FALSE;
        _multisampling.alphaToOneEnable = VK_FALSE;
    }

    void disable_blending() {
        _color_blend_attachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        _color_blend_attachment.blendEnable = VK_FALSE;
    }

    void set_color_attachment_format(VkFormat format) {
        _color_attachment_format = format;
        _render_info.colorAttachmentCount = 1;
        _render_info.pColorAttachmentFormats = &_color_attachment_format;
    }

    void set_depth_format(VkFormat format) {
        _render_info.depthAttachmentFormat = format;
    }

    void disable_depthtest() {
        _depth_stencil.depthTestEnable = VK_FALSE;
        _depth_stencil.depthWriteEnable = VK_FALSE;
        _depth_stencil.depthCompareOp = VK_COMPARE_OP_NEVER;
        _depth_stencil.depthBoundsTestEnable = VK_FALSE;
        _depth_stencil.stencilTestEnable = VK_FALSE;
        _depth_stencil.front = {};
        _depth_stencil.back = {};
        _depth_stencil.minDepthBounds = 0.f;
        _depth_stencil.maxDepthBounds = 1.f;
    }

    void enable_depthtest(bool depth_write_enable, VkCompareOp op)
    {
        _depth_stencil.depthTestEnable = VK_TRUE;
        _depth_stencil.depthWriteEnable = depth_write_enable;
        _depth_stencil.depthCompareOp = op;
        _depth_stencil.depthBoundsTestEnable = VK_FALSE;
        _depth_stencil.stencilTestEnable = VK_FALSE;
        _depth_stencil.front = {};
        _depth_stencil.back = {};
        _depth_stencil.minDepthBounds = 0.f;
        _depth_stencil.maxDepthBounds = 1.f;
    }

    void enable_blending_additive() {
        _color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        _color_blend_attachment.blendEnable = VK_TRUE;
        _color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        _color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
        _color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
        _color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        _color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        _color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
    }

    void enable_blending_alphablend() {
        _color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        _color_blend_attachment.blendEnable = VK_TRUE;
        _color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        _color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        _color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
        _color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        _color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        _color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
    }
};

VkRenderingAttachmentInfo depth_attachment_info(VkImageView view, VkImageLayout layout /*= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL*/) {
    VkRenderingAttachmentInfo depth_attachment {};
    depth_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depth_attachment.pNext = nullptr;

    depth_attachment.imageView = view;
    depth_attachment.imageLayout = layout;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depth_attachment.clearValue.depthStencil.depth = 0.f;

    return depth_attachment;
}

VkImageResolve msaa_resolve() {
    VkImageResolve resolve_region{};
    resolve_region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    resolve_region.srcSubresource.mipLevel = 0;
    resolve_region.srcSubresource.baseArrayLayer = 0;
    resolve_region.srcSubresource.layerCount = 1;

    resolve_region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    resolve_region.dstSubresource.mipLevel = 0;
    resolve_region.dstSubresource.baseArrayLayer = 0;
    resolve_region.dstSubresource.layerCount = 1;

    resolve_region.srcOffset = {0, 0, 0};
    resolve_region.dstOffset = {0, 0, 0};
    return resolve_region;
}

struct AllocatedBuffer {
    VkBuffer buffer;
    VmaAllocation allocation;
    VmaAllocationInfo info;
};

struct Vertex {
    glm::vec3 position;
    float uv_x;
    glm::vec3 normal;
    float uv_y;
};

void debug_print_vertex_data(Vertex vert) {
    cout << "[GVK DEBUG] vertex data: " << endl;
    cout << "                   position: " << vert.position.x << ", " << vert.position.y << ", " << vert.position.z << endl;
    cout << "                   normal: " << vert.normal.x << ", " << vert.normal.y << ", " << vert.normal.z << endl;
    cout << "                   uv: " << vert.uv_x << ", " << vert.uv_y << endl;
}

struct GPUMeshBuffers {
    AllocatedBuffer index_buffer;
    AllocatedBuffer vertex_buffer;
    VkDeviceAddress vertex_buffer_address;
};

struct GPUDrawPushConstants {
    glm::mat4 world_matrix;
    VkDeviceAddress vertex_buffer;
    VkDeviceAddress index_buffer;
};

struct GeoSurface {
    uint32_t start_index;
    uint32_t count;
};

struct MeshAsset {
    string name;

    vector<GeoSurface> surfaces;
    GPUMeshBuffers mesh_buffers;
    glm::vec3 AABB_min;
    glm::vec3 AABB_max;
};

struct GPUSceneData {
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 viewproj;
    glm::vec4 ambient_color;
    glm::vec4 sunlight_direction;
    glm::vec4 sunlight_color;
};

struct DescriptorAllocatorGrowable {
public:
    struct PoolSizeRatio {
        VkDescriptorType type;
        float ratio;
    };

    void init(VkDevice device, uint32_t max_sets, span<PoolSizeRatio> pool_ratios) {
        ratios.clear();

        for (auto r : pool_ratios) {
            ratios.push_back(r);
        }

        VkDescriptorPool new_pool = create_pool(device, max_sets, pool_ratios);

        sets_per_pool = max_sets*1.5f;

        ready_pools.push_back(new_pool);
    }

    void clear_pools(VkDevice device) {
        for (auto p : ready_pools) {
            vkResetDescriptorPool(device, p, 0);
        }
        for (auto p : full_pools) {
            vkResetDescriptorPool(device, p, 0);
            ready_pools.push_back(p);
        }
        full_pools.clear();
    }

    void destroy_pools(VkDevice device) {
        for (auto p : ready_pools) {
            vkDestroyDescriptorPool(device, p, nullptr);
        }
        ready_pools.clear();
        for (auto p : full_pools) {
            vkDestroyDescriptorPool(device, p, nullptr);
        }
        full_pools.clear();
    }

    VkDescriptorSet allocate(VkDevice device, VkDescriptorSetLayout layout, void* pNext = nullptr) {
        VkDescriptorPool pool_to_use = get_pool(device);

        VkDescriptorSetAllocateInfo allocate_info = {};
        allocate_info.pNext = pNext;
        allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocate_info.descriptorPool = pool_to_use;
        allocate_info.descriptorSetCount = 1;
        allocate_info.pSetLayouts = &layout;

        VkDescriptorSet ds;
        VkResult result = vkAllocateDescriptorSets(device, &allocate_info, &ds);

        if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL) {
            full_pools.push_back(pool_to_use);

            pool_to_use = get_pool(device);
            allocate_info.descriptorPool = pool_to_use;

            VK_CHECK(vkAllocateDescriptorSets(device, &allocate_info, &ds));
        }

        ready_pools.push_back(pool_to_use);
        return ds;
    }

private:
    VkDescriptorPool get_pool(VkDevice device) {
        VkDescriptorPool new_pool;
        if (ready_pools.size() != 0) {
            new_pool = ready_pools.back();
            ready_pools.pop_back();
        } else {
            new_pool = create_pool(device, sets_per_pool, ratios);

            sets_per_pool = sets_per_pool * 1.5;
            if (sets_per_pool > 4096) sets_per_pool = 4096;

        }

        return new_pool;
    }

    VkDescriptorPool create_pool(VkDevice device, uint32_t set_count, span<PoolSizeRatio> pool_ratios) {
        vector<VkDescriptorPoolSize> pool_sizes;
        for (PoolSizeRatio ratio : pool_ratios) {
            pool_sizes.push_back(VkDescriptorPoolSize{
            .type = ratio.type,
            .descriptorCount = static_cast<uint32_t>(ratio.ratio * set_count)
            });
        }

        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = 0;
        pool_info.maxSets = set_count;
        pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
        pool_info.pPoolSizes = pool_sizes.data();

        VkDescriptorPool new_pool;
        vkCreateDescriptorPool(device, &pool_info, nullptr, &new_pool);
        return new_pool;
    }

    vector<PoolSizeRatio> ratios;
    vector<VkDescriptorPool> full_pools;
    vector<VkDescriptorPool> ready_pools;
    uint32_t sets_per_pool;
};

struct DescriptorWriter {
    deque<VkDescriptorImageInfo> image_infos;
    deque<VkDescriptorBufferInfo> buffer_infos;
    vector<VkWriteDescriptorSet> writes;

    void write_image(int binding, VkImageView image, VkSampler sampler, VkImageLayout layout, VkDescriptorType type) {
        VkDescriptorImageInfo& info = image_infos.emplace_back(VkDescriptorImageInfo{
        .sampler = sampler,
        .imageView = image,
        .imageLayout = layout
        });

        VkWriteDescriptorSet write = {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};

        write.dstBinding = binding;
        write.dstSet = VK_NULL_HANDLE;
        write.descriptorCount = 1;
        write.descriptorType = type;
        write.pImageInfo = &info;

        writes.push_back(write);
    }

    void write_buffer(int binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type) {
        VkDescriptorBufferInfo& info = buffer_infos.emplace_back(VkDescriptorBufferInfo{
        .buffer = buffer,
        .offset = offset,
        .range = size
        });

        VkWriteDescriptorSet write = {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};

        write.dstBinding = binding;
        write.dstSet = VK_NULL_HANDLE;
        write.descriptorCount = 1;
        write.descriptorType = type;
        write.pBufferInfo = &info;

        writes.push_back(write);
    }

    void clear() {
        image_infos.clear();
        writes.clear();
        buffer_infos.clear();
    }

    void update_set(VkDevice device, VkDescriptorSet set) {
        for (VkWriteDescriptorSet& write : writes) {
            write.dstSet = set;
        }

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }
};

struct RenderQueueMesh {
    shared_ptr<MeshAsset> mesh;
    AllocatedImage image;
    glm::vec3 position;
    glm::vec3 scale;
    glm::quat rotation;
};

struct Plane {
    glm::vec3 normal;
    float distance;
};

void calculate_AABB(glm::vec3& min, glm::vec3& max, vector<Vertex> vertices) {
    for (Vertex vert : vertices) {
        if (vert.position.x < min.x) min.x = vert.position.x;
        if (vert.position.y < min.y) min.y = vert.position.y;
        if (vert.position.z < min.z) min.z = vert.position.z;

        if (vert.position.x > max.x) max.x = vert.position.x;
        if (vert.position.y > max.y) max.y = vert.position.y;
        if (vert.position.z > max.z) max.z = vert.position.z;
    }
}

void calculate_world_AABB(glm::vec3& min, glm::vec3& max, const RenderQueueMesh& rqm)
{
    const glm::vec3& localMin = rqm.mesh->AABB_min;
    const glm::vec3& localMax = rqm.mesh->AABB_max;

    glm::mat4 model = glm::translate(glm::mat4(1.f), rqm.position) * glm::mat4_cast(rqm.rotation) * glm::scale(glm::mat4(1.f), rqm.scale);

    glm::vec3 corners[8] = {
        glm::vec3(localMin.x, localMin.y, localMin.z),
        glm::vec3(localMax.x, localMin.y, localMin.z),
        glm::vec3(localMin.x, localMax.y, localMin.z),
        glm::vec3(localMax.x, localMax.y, localMin.z),
        glm::vec3(localMin.x, localMin.y, localMax.z),
        glm::vec3(localMax.x, localMin.y, localMax.z),
        glm::vec3(localMin.x, localMax.y, localMax.z),
        glm::vec3(localMax.x, localMax.y, localMax.z)
    };

    min = glm::vec3(FLT_MAX);
    max = glm::vec3(-FLT_MAX);

    for(int i = 0; i < 8; i++) {
        glm::vec4 p = model * glm::vec4(corners[i], 1.0f);
        p /= p.w;
        min.x = glm::min(min.x, p.x);
        min.y = glm::min(min.y, p.y);
        min.z = glm::min(min.z, p.z);
        max.x = glm::max(max.x, p.x);
        max.y = glm::max(max.y, p.y);
        max.z = glm::max(max.z, p.z);
    }
}

bool is_AABB_inside_frustum(glm::vec3 min, glm::vec3 max, Plane left, Plane right, Plane bottom, Plane top, Plane near, Plane far) {
    Plane planes[6] = {left, right, bottom, top, near, far};

    for (int i = 0; i < 6; i++) {
        const Plane& p = planes[i];

        glm::vec3 positive_vertex;
        positive_vertex.x = (p.normal.x >= 0.f) ? max.x : min.x;
        positive_vertex.y = (p.normal.y >= 0.f) ? max.y : min.y;
        positive_vertex.z = (p.normal.z >= 0.f) ? max.z : min.z;

        float distance = p.normal.x * positive_vertex.x + p.normal.y * positive_vertex.y + p.normal.z * positive_vertex.z + p.distance;

        if (distance < 0.f) return false;
    }
    return true;
}

void extract_frustum_planes(const glm::mat4& matrix, Plane& left, Plane& right, Plane& bottom, Plane& top, Plane& near, Plane& far) {
    glm::vec4 row0 = glm::row(matrix, 0);
    glm::vec4 row1 = glm::row(matrix, 1);
    glm::vec4 row2 = glm::row(matrix, 2);
    glm::vec4 row3 = glm::row(matrix, 3);

    glm::vec4 leftPlane = row3+row0;
    glm::vec4 rightPlane = row3-row0;
    glm::vec4 bottomPlane = row3+row1;
    glm::vec4 topPlane = row3-row1;
    glm::vec4 nearPlane = row3+row2;
    glm::vec4 farPlane = row3-row2;

    auto normalize_plane = [](glm::vec4 p) -> Plane {
        float length = glm::length(glm::vec3(p));
        if (length > 0.00001f) {
            p /= length;
        }
        return Plane{ glm::vec3(p), p.w };
    };

    left = normalize_plane(leftPlane);
    right = normalize_plane(rightPlane);
    bottom = normalize_plane(bottomPlane);
    top = normalize_plane(topPlane);
    near = normalize_plane(nearPlane);
    far = normalize_plane(farPlane);
}

struct CubeMap {
    AllocatedImage image;
    VkImageView image_view;
    VkSampler sampler;
};

struct Skybox {
    GPUMeshBuffers mesh_buffers;
    CubeMap cubemap;
    VkPipeline pip;
    VkPipelineLayout piplayout;
    VkDescriptorSetLayout desc_layout;
};

namespace gvk {
    void init();
    void quit();
}

#ifdef GVK_IMPLEMENTATION
#define GVK_IMPLEMENTATION_INCLUDED

namespace gvk {
    inline SDL_Window* window = nullptr;
    inline DeletionQueue _main_deletion_queue;

    inline VkInstance _vk_instance;
    inline VkPhysicalDevice _chosen_GPU;
    inline VkDevice _vk_device;
    inline VkDebugUtilsMessengerEXT _debug_messenger;
    inline VkSurfaceKHR _vk_surface;
    inline VkAllocationCallbacks* _vk_allocation_callbacks = nullptr;

    inline VkSwapchainKHR _swapchain;
    inline VkFormat _swapchain_image_format;

    inline vector<VkImage> _swapchain_images;
    inline vector<VkImageView> _swapchain_image_views;
    inline VkExtent2D _swapchain_extent;

    struct FrameData {
        VkCommandPool _commandPool;
        VkCommandBuffer _mainCommandBuffer;
        VkSemaphore _swapchain_semaphore, _render_semaphore;
        VkFence _render_fence;
        DeletionQueue _deletion_queue;
        DescriptorAllocatorGrowable _frame_descriptors;
    };
    constexpr unsigned int FRAME_OVERLAP = 2;

    inline uint32_t _frame_number{0};
    inline FrameData _frames[FRAME_OVERLAP];
    inline FrameData& get_current_frame() { return _frames[_frame_number % FRAME_OVERLAP]; };
    inline VkQueue _graphics_queue;
    inline uint32_t _graphics_queue_family;

    inline VmaAllocator _allocator;

    // draw resources
    inline AllocatedImage _draw_image;
    inline AllocatedImage _depth_image;
    inline VkExtent2D _draw_extent;

    // descriptors
    DescriptorAllocator global_descriptor_allocator;
    VkDescriptorSet _draw_image_descriptors;
    VkDescriptorSetLayout _draw_image_descriptor_layout;

    // triangle pipeline
    inline VkPipeline _triangle_pipeline;
    inline VkPipelineLayout _triangle_pipeline_layout;

    // imgui resources
    inline VkFence _imm_fence;
    inline VkCommandBuffer _imm_command_buffer;
    inline VkCommandPool _imm_command_pool;

    inline VkPipelineLayout _mesh_pipeline_layout;
    inline VkPipeline _mesh_pipeline;

    inline GPUSceneData scene_data;
    inline VkDescriptorSetLayout _gpu_scene_data_descriptor_layout;

    // you know what these can stay, they'll come in handy
    inline AllocatedImage _white_image;
    inline AllocatedImage _black_image;
    inline AllocatedImage _gray_image;
    inline AllocatedImage _error_checkerboard_image;

    inline VkSampler _default_sampler_linear;
    inline VkSampler _default_sampler_nearest;

    inline VkDescriptorSetLayout _single_image_descriptor_layout;

    struct {
        glm::vec3 position  = {0.f, 0.f, -5.f};
        glm::vec3 direction = {0.f, 0.f,  1.f};
    } camera;

    inline vector<RenderQueueMesh> render_queue;

    inline glm::vec4 clear_color = {0.f, 0.f, 0.f, 1.f};
    inline float fov = 65.f;

    // frustum culling planes
    inline Plane fc_top_plane;
    inline Plane fc_bottom_plane;
    inline Plane fc_left_plane;
    inline Plane fc_right_plane;
    inline Plane fc_far_plane;
    inline Plane fc_near_plane;

    // skybox pass resources
    inline Skybox skybox;
    inline AllocatedImage _skybox_draw_image;
    inline AllocatedImage _skybox_draw_image_msaa;
    inline VkDescriptorSetLayout _skybox_draw_image_descriptor_layout;

    // composite pipeline
    inline AllocatedImage _composite_image;
    inline VkPipeline _composite_pipeline;
    inline VkPipelineLayout _composite_pipeline_layout;
    inline VkDescriptorSetLayout _composite_descriptor_layout;

    // msaa
    inline VkSampleCountFlagBits msaa_sample_count = VK_SAMPLE_COUNT_8_BIT;
    inline AllocatedImage _draw_image_msaa;
    inline AllocatedImage _depth_image_msaa;

    void immediate_submit(function<void(VkCommandBuffer cmd)>&& function) {
        vkWaitForFences(_vk_device, 1, &_imm_fence, VK_TRUE, UINT64_MAX);
        vkResetFences(_vk_device, 1, &_imm_fence);

        VK_CHECK(vkResetFences(_vk_device, 1, &_imm_fence));
        VK_CHECK(vkResetCommandBuffer(_imm_command_buffer, 0));

        VkCommandBuffer cmd = _imm_command_buffer;
        VkCommandBufferBeginInfo cmd_begin_info = create_command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        VK_CHECK(vkBeginCommandBuffer(cmd, &cmd_begin_info));

        function(cmd);

        VK_CHECK(vkEndCommandBuffer(cmd));

        VkCommandBufferSubmitInfo cmdinfo = make_command_buffer_submit_info(cmd);
        VkSubmitInfo2 submit = submit_info(&cmdinfo, nullptr , nullptr);

        VK_CHECK(vkQueueSubmit2(_graphics_queue, 1, &submit, _imm_fence));

        VK_CHECK(vkWaitForFences(_vk_device, 1, &_imm_fence, true, 9999999999));
    }

    AllocatedBuffer create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage) {
        VkBufferCreateInfo buffer_info = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        buffer_info.pNext = nullptr;
        buffer_info.size = allocSize;

        buffer_info.usage = usage;

        VmaAllocationCreateInfo vmaallocinfo = {};
        vmaallocinfo.usage = memoryUsage;
        vmaallocinfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        AllocatedBuffer new_buffer;

        VK_CHECK(vmaCreateBuffer(_allocator, &buffer_info, &vmaallocinfo, &new_buffer.buffer, &new_buffer.allocation, &new_buffer.info));

        return new_buffer;
    }

    void destroy_buffer(const AllocatedBuffer& buffer)
    {
        vmaDestroyBuffer(_allocator, buffer.buffer, buffer.allocation);
    }

    AllocatedImage create_image(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = true) {
        AllocatedImage new_image;
        new_image.format = format;
        new_image.extent = size;
        new_image.mipmaps = static_cast<uint32_t>(floor(log2(max(size.width, size.height))))+1;

        VkImageCreateInfo img_info = image_create_info(format, usage | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, size);
        if (mipmapped) {
            img_info.mipLevels = static_cast<uint32_t>(floor(log2(max(size.width, size.height))))+1;
        }

        VmaAllocationCreateInfo allocinfo = {};
        allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VK_CHECK(vmaCreateImage(_allocator, &img_info, &allocinfo, &new_image.image, &new_image.allocation, nullptr));

        VkImageAspectFlags aspectflag = VK_IMAGE_ASPECT_COLOR_BIT;
        if (format == VK_FORMAT_D32_SFLOAT) {
            aspectflag = VK_IMAGE_ASPECT_DEPTH_BIT;
        }

        VkImageViewCreateInfo view_info = imageview_create_info(format, new_image.image, new_image.mipmaps, aspectflag);
        view_info.subresourceRange.levelCount = img_info.mipLevels;

        VK_CHECK(vkCreateImageView(_vk_device, &view_info, nullptr, &new_image.image_view));

        return new_image;
    }

    AllocatedImage create_image(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = true) {
        size_t data_size = size.depth * size.width * size.height * 4;
        AllocatedBuffer uploadbuffer = create_buffer(data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

        memcpy(uploadbuffer.info.pMappedData, data, data_size);

        AllocatedImage new_image = create_image(size, format, usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, mipmapped);

        new_image.mipmaps = static_cast<uint32_t>(floor(log2(max(size.width, size.height))))+1;

        immediate_submit([&](VkCommandBuffer cmd) {
            transition_image(cmd, new_image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

            VkBufferImageCopy copy_region = {};
            copy_region.bufferOffset = 0;
            copy_region.bufferRowLength = 0;
            copy_region.bufferImageHeight = 0;

            copy_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copy_region.imageSubresource.mipLevel = 0;
            copy_region.imageSubresource.baseArrayLayer = 0;
            copy_region.imageSubresource.layerCount = 1;
            copy_region.imageExtent = size;

            vkCmdCopyBufferToImage(cmd, uploadbuffer.buffer, new_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

            int32_t mip_width = static_cast<int32_t>(size.width);
            int32_t mip_height = static_cast<int32_t>(size.height);

            for (uint32_t mip = 1; mip < new_image.mipmaps; mip++) {
                transition_image_mip(cmd, new_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, mip-1);

                VkImageBlit2 blit = {.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2};

                blit.srcOffsets[0] = {0,0, 0};
                blit.srcOffsets[1] = {mip_width, mip_height, 1};
                blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                blit.srcSubresource.mipLevel = mip-1;
                blit.srcSubresource.baseArrayLayer = 0;
                blit.srcSubresource.layerCount = 1;

                int32_t next_width = mip_width > 1 ? mip_width / 2 : 1;
                int32_t next_height = mip_height > 1 ? mip_height / 2 : 1;

                blit.dstOffsets[0] = {0,0 ,0};
                blit.dstOffsets[1] = {next_width, next_height, 1};
                blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                blit.dstSubresource.mipLevel = mip;
                blit.dstSubresource.baseArrayLayer = 0;
                blit.dstSubresource.layerCount = 1;

                VkBlitImageInfo2 blit_info = {.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2};
                blit_info.srcImage = new_image.image;
                blit_info.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                blit_info.dstImage = new_image.image;
                blit_info.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                blit_info.filter = VK_FILTER_LINEAR;
                blit_info.regionCount = 1;
                blit_info.pRegions = &blit;

                vkCmdBlitImage2(cmd, &blit_info);

                transition_image_mip(cmd, new_image.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mip-1);

                mip_width = next_width;
                mip_height = next_height;
            }

            transition_image_mip(cmd, new_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, new_image.mipmaps - 1);
        });

        destroy_buffer(uploadbuffer);

        return new_image;
    }
    void destroy_image(const AllocatedImage& img) {
        if (img.image_view != VK_NULL_HANDLE) {
            vkDestroyImageView(_vk_device, img.image_view, nullptr);
        }

        if (img.image != VK_NULL_HANDLE && img.allocation != nullptr) {
            vmaDestroyImage(_allocator, img.image, img.allocation);
        }
    }

    class Surface {
    public:
        vector<vector<glm::vec4>> pixels;
        glm::vec4 color = {1.0f, 1.0f, 1.0f, 1.0f};
        AllocatedImage vk_image;
        VkFilter sampler_type = VK_FILTER_LINEAR;

        void clear(int w, int h, glm::vec4 color = {1.0f, 1.0f, 1.0f, 0.0f}) {
            pixels.resize(h);
            for (int i = 0; i < h; i++) {
                pixels[i].resize(w);
                for (int j = 0; j < w; j++) {
                    pixels[i][j] = color;
                }
            }
        }

        void draw(const Surface& other_surf, glm::vec2 pos) {
            int dest_x = static_cast<int>(pos.x);
            int dest_y = static_cast<int>(pos.y);

            if (pixels.empty() || pixels[0].empty() || other_surf.pixels.empty() || other_surf.pixels[0].empty()) return;

            int dest_height = pixels.size();
            int dest_width = pixels[0].size();
            int src_height = other_surf.pixels.size();
            int src_width = other_surf.pixels[0].size();

            for (int src_y = 0; src_y < src_height; src_y++) {
                for (int src_x = 0; src_x < src_width; src_x++) {
                    int target_x = dest_x + src_x;
                    int target_y = dest_y + src_y;

                    if (target_x >= 0 && target_x < dest_width && target_y >= 0 && target_y < dest_height) {
                        glm::vec4 src_pixel = other_surf.pixels[src_y][src_x];
                        glm::vec4 final_pixel = {
                            src_pixel.x * other_surf.color.x,
                            src_pixel.y * other_surf.color.y,
                            src_pixel.z * other_surf.color.z,
                            src_pixel.w * other_surf.color.w
                        };

                        pixels[target_y][target_x] = final_pixel;
                    }
                }
            }
        }

        void draw_text(const stbtt_fontinfo* font, const string& text, glm::vec2 pos, float scale, glm::vec4 text_color = {1.0f, 1.0f, 1.0f, 1.0f}) {
            if (pixels.empty() || pixels[0].empty() || !font) return;

            int dest_width = pixels[0].size();
            int dest_height = pixels.size();

            float x = pos.x;
            float baseline_y = pos.y;

            int ascent, descent, line_gap;
            stbtt_GetFontVMetrics(font, &ascent, &descent, &line_gap);
            float scaled_ascent = ascent * scale;
            float scaled_descent = descent * scale;

            float total_width = 0.0f;
            for (size_t i = 0; i < text.length(); i++) {
                int advance, lsb;
                stbtt_GetCodepointHMetrics(font, text[i], &advance, &lsb);
                total_width += advance * scale;

                if (i < text.length() - 1) {
                    int kern = stbtt_GetCodepointKernAdvance(font, text[i], text[i + 1]);
                    total_width += kern * scale;
                }
            }

            x += total_width;

            for (int i = (int)text.length() - 1; i >= 0; i--) {
                int advance, lsb;
                stbtt_GetCodepointHMetrics(font, text[i], &advance, &lsb);

                x -= advance * scale;

                if (i > 0) {
                    int kern = stbtt_GetCodepointKernAdvance(font, text[i-1], text[i]);
                    x -= kern * scale;
                }

                int glyph_width, glyph_height, glyph_xoff, glyph_yoff;
                unsigned char* glyph_bitmap = stbtt_GetCodepointBitmap(font, 0, scale, text[i], &glyph_width, &glyph_height, &glyph_xoff, &glyph_yoff);

                if (glyph_bitmap) {
                    int start_x = (int)(x + glyph_xoff);
                    int start_y = (int)(baseline_y - scaled_descent - glyph_yoff - glyph_height);

                    for (int gy = 0; gy < glyph_height; gy++) {
                        for (int gx = 0; gx < glyph_width; gx++) {
                            int target_x = start_x + gx;
                            int target_y = start_y + gy;

                            if (target_x >= 0 && target_x < dest_width && target_y >= 0 && target_y < dest_height) {
                                float alpha = glyph_bitmap[gy * glyph_width + gx] / 255.0f;

                                if (alpha > 0.0f) {
                                    glm::vec4 bg_pixel = pixels[target_y][target_x];
                                    glm::vec4 final_pixel = {
                                        bg_pixel.x * (1.0f - alpha) + text_color.x * alpha,
                                        bg_pixel.y * (1.0f - alpha) + text_color.y * alpha,
                                        bg_pixel.z * (1.0f - alpha) + text_color.z * alpha,
                                        bg_pixel.w * (1.0f - alpha) + text_color.w * alpha
                                    };
                                    pixels[target_y][target_x] = final_pixel;
                                }
                            }
                        }
                    }

                    stbtt_FreeBitmap(glyph_bitmap, nullptr);
                }
            }
        }

        void load_from_file(string path) {
            int img_width = 0, img_height = 0, num_channels = 0;
            const int desired_channels = 4;
            stbi_uc* stb_pixels = stbi_load(path.c_str(), &img_width, &img_height, &num_channels, desired_channels);

            if (!stb_pixels) {
                return;
            }

            pixels.resize(img_height);
            for (int i = 0; i < img_height; i++) {
                pixels[i].resize(img_width);
                for (int j = 0; j < img_width; j++) {
                    int idx = (i * img_width + j) * 4;
                    pixels[i][j] = {
                        stb_pixels[idx] / 255.0f,
                        stb_pixels[idx + 1] / 255.0f,
                        stb_pixels[idx + 2] / 255.0f,
                        stb_pixels[idx + 3] / 255.0f
                    };
                }
            }

            stbi_image_free(stb_pixels);
        }

        void resize(int new_width, int new_height) {
            if (pixels.empty() || pixels[0].empty()) return;

            int old_width = pixels[0].size();
            int old_height = pixels.size();

            if (old_width == new_width && old_height == new_height) return;

            vector<uint8_t> input_data(old_width * old_height * 4);
            for (int y = 0; y < old_height; y++) {
                for (int x = 0; x < old_width; x++) {
                    int idx = (y * old_width + x) * 4;
                    input_data[idx] = static_cast<uint8_t>(pixels[y][x].x * 255.0f);
                    input_data[idx + 1] = static_cast<uint8_t>(pixels[y][x].y * 255.0f);
                    input_data[idx + 2] = static_cast<uint8_t>(pixels[y][x].z * 255.0f);
                    input_data[idx + 3] = static_cast<uint8_t>(pixels[y][x].w * 255.0f);
                }
            }

            vector<uint8_t> output_data(new_width * new_height * 4);

            stbir_resize_uint8_linear(
                input_data.data(), old_width, old_height, 0,
                output_data.data(), new_width, new_height, 0,
                STBIR_RGBA
            );

            pixels.resize(new_height);
            for (int y = 0; y < new_height; y++) {
                pixels[y].resize(new_width);
                for (int x = 0; x < new_width; x++) {
                    int idx = (y * new_width + x) * 4;
                    pixels[y][x] = {
                        output_data[idx] / 255.0f,
                        output_data[idx + 1] / 255.0f,
                        output_data[idx + 2] / 255.0f,
                        output_data[idx + 3] / 255.0f
                    };
                }
            }
        }

        void resize_percentage(float width_percent, float height_percent) {
            if (pixels.empty() || pixels[0].empty()) return;

            int old_width = pixels[0].size();
            int old_height = pixels.size();

            int new_width = static_cast<int>(old_width * width_percent);
            int new_height = static_cast<int>(old_height * height_percent);

            if (new_width <= 0) new_width = 1;
            if (new_height <= 0) new_height = 1;

            resize(new_width, new_height);
        }

        void draw_rect(int w, int h, glm::vec2 pos, glm::vec4 color) {
            if (pixels.empty() || pixels[0].empty()) return;

            int dest_x = static_cast<int>(pos.x);
            int dest_y = static_cast<int>(pos.y);
            int src_width = pixels[0].size();
            int src_height = pixels.size();

            for (int y = 0; y < h; y++) {
                for (int x = 0; x < w; x++) {
                    int target_x = dest_x + x;
                    int target_y = dest_y + y;

                    if (target_x >= 0 && target_x < src_width && target_y >= 0 && target_y < src_height) {
                        pixels[target_y][target_x] = color;
                    }
                }
            }
        }

        void refresh() {
            // basically just create / recreate the AllocatedImage

            if (pixels.empty() || pixels[0].empty()) {
                cout << "Nuh uh the pixels are empty what are you refreshing" << endl;
                return;
            }

            int height = pixels.size();
            int width = pixels[0].size();
            uint8_t data[height * width * 4];
            for (int i = 0; i < height; i++) {
                for (int j = 0; j < width; j++) {
                    int idx = (i * width + j) * 4;
                    data[idx] = static_cast<uint8_t>(pixels[i][j].x * 255.0f);
                    data[idx + 1] = static_cast<uint8_t>(pixels[i][j].y * 255.0f);
                    data[idx + 2] = static_cast<uint8_t>(pixels[i][j].z * 255.0f);
                    data[idx + 3] = static_cast<uint8_t>(pixels[i][j].w * 255.0f);
                }
            }

            VkExtent3D extent = { static_cast<uint32_t>(pixels[0].size()), static_cast<uint32_t>(pixels.size()), 1 };
            vk_image = create_image(data, extent, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_SAMPLED_BIT);

            if (!in_deletion_queue) {
                _main_deletion_queue.push_function([&]() {
                    destroy_image(vk_image);
                });
                in_deletion_queue = true;
            }
        }

    private:
        bool in_deletion_queue = false;
    };

    Surface display;

    class PostProcessingStack {
    public:
        AllocatedImage out_image;

        // vignette
        // TODO: do this later

        // gaussian_blur
        AllocatedImage _gb_out_image;
        AllocatedImage _gb_out_image_2;
        VkPipeline _gb_pipeline;
        VkPipelineLayout _gb_pipeline_layout;
        VkDescriptorSetLayout _gbb_descriptor_layout;
        int gaussian_blur_radius = 3;
        float gaussian_blur_sigma = 2.5f;

        AllocatedImage _bb_out_image;
        AllocatedImage _bb_out_image_2;
        VkPipeline _bb_pipeline;
        VkPipelineLayout _bb_pipeline_layout;
        VkDescriptorSetLayout _bb_descriptor_layout;
        int box_blur_radius = 3;

        struct GaussianBlurPushConstants {
            int radius;
            float sigma;
            int is_vertical;
        };

        void _init_vignette() {

        }

        void _init_gaussian_blur() {
            DescriptorLayoutBuilder builder;
            builder.add_binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            _gbb_descriptor_layout = builder.build(_vk_device, VK_SHADER_STAGE_FRAGMENT_BIT);

            VkPushConstantRange blur_push_range{};
            blur_push_range.offset = 0;
            blur_push_range.size = sizeof(GaussianBlurPushConstants);
            blur_push_range.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

            VkPipelineLayoutCreateInfo layout_info = pipeline_layout_create_info();
            layout_info.setLayoutCount = 1;
            layout_info.pSetLayouts = &_gbb_descriptor_layout;
            layout_info.pushConstantRangeCount = 1;
            layout_info.pPushConstantRanges = &blur_push_range;
            vkCreatePipelineLayout(_vk_device, &layout_info, nullptr, &_gb_pipeline_layout);

            VkShaderModule vert_shader, frag_shader;
            if (!load_shader_module("../shaders/fullscreen_triangle.vert.spv", _vk_device, &vert_shader)) {
                cout << "error when loading guassian blur vert (aka fullscreen triangle) shader" << endl;
            }
            if (!load_shader_module("../shaders/gaussian_blur.frag.spv", _vk_device, &frag_shader)) {
                cout << "error when loading gaussian blur frag shader" << endl;
            }

            PipelineBuilder pip_builder;
            pip_builder._pipeline_layout = _gb_pipeline_layout;
            pip_builder.set_shaders(vert_shader, frag_shader);
            pip_builder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
            pip_builder.set_polygon_mode(VK_POLYGON_MODE_FILL);
            pip_builder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
            pip_builder.set_multisampling_none();
            pip_builder.disable_blending();
            pip_builder.disable_depthtest();
            pip_builder.set_color_attachment_format(_draw_image.format);
            _gb_pipeline = pip_builder.build_pipeline(_vk_device);

            vkDestroyShaderModule(_vk_device, vert_shader, nullptr);
            vkDestroyShaderModule(_vk_device, frag_shader, nullptr);

            int w_width, w_height;
            SDL_GetWindowSize(window, &w_width, &w_height);

            VkExtent3D gb_out_extent = {};
            gb_out_extent.width = static_cast<uint32_t>(w_width);
            gb_out_extent.height = static_cast<uint32_t>(w_height);
            gb_out_extent.depth = 1;

            _gb_out_image.format = VK_FORMAT_R8G8B8A8_SRGB;
            _gb_out_image.extent = gb_out_extent;
            _gb_out_image.mipmaps = 1;

            VkImageUsageFlags _gb_out_image_usages{};
            _gb_out_image_usages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
            _gb_out_image_usages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            _gb_out_image_usages |= VK_IMAGE_USAGE_STORAGE_BIT;
            _gb_out_image_usages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            _gb_out_image_usages |= VK_IMAGE_USAGE_SAMPLED_BIT;

            VkImageCreateInfo rimg_info = image_create_info(_gb_out_image.format, _gb_out_image_usages, gb_out_extent);

            VmaAllocationCreateInfo rimg_allocinfo = {};
            rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
            rimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            vmaCreateImage(_allocator, &rimg_info, &rimg_allocinfo, &_gb_out_image.image, &_gb_out_image.allocation, nullptr);
            VkImageViewCreateInfo rview_info = imageview_create_info(_gb_out_image.format, _gb_out_image.image, _gb_out_image.mipmaps, VK_IMAGE_ASPECT_COLOR_BIT);

            VK_CHECK(vkCreateImageView(_vk_device, &rview_info, nullptr, &_gb_out_image.image_view));

            _gb_out_image_2.format = VK_FORMAT_R8G8B8A8_SRGB;
            _gb_out_image_2.extent = gb_out_extent;
            _gb_out_image_2.mipmaps = 1;

            VkImageCreateInfo rimg_info2 = image_create_info(_gb_out_image_2.format, _gb_out_image_usages, gb_out_extent);

            vmaCreateImage(_allocator, &rimg_info2, &rimg_allocinfo, &_gb_out_image_2.image, &_gb_out_image_2.allocation, nullptr);
            VkImageViewCreateInfo rview_info2 = imageview_create_info(_gb_out_image_2.format, _gb_out_image_2.image, _gb_out_image_2.mipmaps, VK_IMAGE_ASPECT_COLOR_BIT);

            VK_CHECK(vkCreateImageView(_vk_device, &rview_info2, nullptr, &_gb_out_image_2.image_view));

            _main_deletion_queue.push_function([&]() {
                destroy_image(_gb_out_image);
                destroy_image(_gb_out_image_2);
                vkDestroyPipelineLayout(_vk_device, _gb_pipeline_layout, nullptr);
                vkDestroyPipeline(_vk_device, _gb_pipeline, nullptr);
                vkDestroyDescriptorSetLayout(_vk_device, _gbb_descriptor_layout, nullptr);
            });
        }

        void gb_blur_image(VkCommandBuffer cmd, AllocatedImage image, int blur_iterations) {
            auto blur_pass = [&](AllocatedImage& src, AllocatedImage& dst, int is_vertical) {
                transition_image(cmd, dst.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

                VkClearValue clear = { .color = { {0.f, 0.f, 0.f, 1.f} } };
                VkRenderingAttachmentInfo color_attach = attachment_info(dst.image_view, &clear, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
                VkExtent2D dst_extent = { dst.extent.width, dst.extent.height };

                VkRenderingInfo render_info = {};
                render_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
                render_info.pNext = nullptr;
                render_info.renderArea = VkRect2D{VkOffset2D{0, 0}, dst_extent};
                render_info.layerCount = 1;
                render_info.colorAttachmentCount = 1;
                render_info.pColorAttachments = &color_attach;
                render_info.pDepthAttachment = nullptr;
                render_info.pStencilAttachment = nullptr;

                vkCmdBeginRendering(cmd, &render_info);

                VkViewport viewport = {};
                viewport.x = 0;
                viewport.y = 0;
                viewport.width = static_cast<float>(dst_extent.width);
                viewport.height = static_cast<float>(dst_extent.height);
                viewport.minDepth = 0.f;
                viewport.maxDepth = 1.f;
                vkCmdSetViewport(cmd, 0, 1, &viewport);

                VkRect2D scissor = {};
                scissor.offset = {0, 0};
                scissor.extent = dst_extent;
                vkCmdSetScissor(cmd, 0, 1, &scissor);

                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _gb_pipeline);

                VkDescriptorSet blur_set = get_current_frame()._frame_descriptors.allocate(_vk_device, _gbb_descriptor_layout, nullptr);
                DescriptorWriter writer;
                writer.write_image(0, src.image_view, _default_sampler_linear, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
                writer.update_set(_vk_device, blur_set);

                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _gb_pipeline_layout, 0, 1, &blur_set, 0, nullptr);

                GaussianBlurPushConstants push_constants{};
                push_constants.radius = gaussian_blur_radius;
                push_constants.sigma = gaussian_blur_sigma;
                push_constants.is_vertical = is_vertical;
                vkCmdPushConstants(cmd, _gb_pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(GaussianBlurPushConstants), &push_constants);

                vkCmdDraw(cmd, 3, 1, 0, 0);

                vkCmdEndRendering(cmd);

                transition_image(cmd, dst.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            };

            if (blur_iterations <= 0) return;

            blur_pass(image, _gb_out_image, 0);
            blur_pass(_gb_out_image, _gb_out_image_2, 1);

            for (int i = 1; i < blur_iterations; i++) {
                blur_pass(_gb_out_image_2, _gb_out_image, 0);
                blur_pass(_gb_out_image, _gb_out_image_2, 1);
            }

            transition_image(cmd, _gb_out_image_2.image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
            transition_image(cmd, image.image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

            VkExtent2D blit_src_extent = { _gb_out_image_2.extent.width, _gb_out_image_2.extent.height };
            VkExtent2D blit_dst_extent = { image.extent.width, image.extent.height };
            copy_image_to_image(cmd, _gb_out_image_2.image, image.image, blit_src_extent, blit_dst_extent);

            transition_image(cmd, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            transition_image(cmd, _gb_out_image_2.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }

        struct BoxBlurPushConstants {
            int radius;
            int is_vertical;
        };

        void _init_box_blur() {
            DescriptorLayoutBuilder builder;
            builder.add_binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            _bb_descriptor_layout = builder.build(_vk_device, VK_SHADER_STAGE_FRAGMENT_BIT);

            VkPushConstantRange blur_push_range{};
            blur_push_range.offset = 0;
            blur_push_range.size = sizeof(BoxBlurPushConstants);
            blur_push_range.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

            VkPipelineLayoutCreateInfo layout_info = pipeline_layout_create_info();
            layout_info.setLayoutCount = 1;
            layout_info.pSetLayouts = &_bb_descriptor_layout;
            layout_info.pushConstantRangeCount = 1;
            layout_info.pPushConstantRanges = &blur_push_range;
            vkCreatePipelineLayout(_vk_device, &layout_info, nullptr, &_bb_pipeline_layout);

            VkShaderModule vert_shader, frag_shader;
            if (!load_shader_module("../shaders/fullscreen_triangle.vert.spv", _vk_device, &vert_shader)) {
                cout << "error when loading box blur vert (aka fullscreen triangle) shader" << endl;
            }
            if (!load_shader_module("../shaders/box_blur.frag.spv", _vk_device, &frag_shader)) {
                cout << "error when loading box blur frag shader" << endl;
            }

            PipelineBuilder pip_builder;
            pip_builder._pipeline_layout = _bb_pipeline_layout;
            pip_builder.set_shaders(vert_shader, frag_shader);
            pip_builder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
            pip_builder.set_polygon_mode(VK_POLYGON_MODE_FILL);
            pip_builder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
            pip_builder.set_multisampling_none();
            pip_builder.disable_blending();
            pip_builder.disable_depthtest();
            pip_builder.set_color_attachment_format(_draw_image.format);
            _bb_pipeline = pip_builder.build_pipeline(_vk_device);

            vkDestroyShaderModule(_vk_device, vert_shader, nullptr);
            vkDestroyShaderModule(_vk_device, frag_shader, nullptr);

            int w_width, w_height;
            SDL_GetWindowSize(window, &w_width, &w_height);

            VkExtent3D bb_out_extent = {};
            bb_out_extent.width = static_cast<uint32_t>(w_width);
            bb_out_extent.height = static_cast<uint32_t>(w_height);
            bb_out_extent.depth = 1;

            _bb_out_image.format = VK_FORMAT_R8G8B8A8_SRGB;
            _bb_out_image.extent = bb_out_extent;
            _bb_out_image.mipmaps = 1;

            VkImageUsageFlags _bb_out_image_usages{};
            _bb_out_image_usages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
            _bb_out_image_usages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            _bb_out_image_usages |= VK_IMAGE_USAGE_STORAGE_BIT;
            _bb_out_image_usages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            _bb_out_image_usages |= VK_IMAGE_USAGE_SAMPLED_BIT;

            VkImageCreateInfo rimg_info = image_create_info(_bb_out_image.format, _bb_out_image_usages, bb_out_extent);

            VmaAllocationCreateInfo rimg_allocinfo = {};
            rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
            rimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            vmaCreateImage(_allocator, &rimg_info, &rimg_allocinfo, &_bb_out_image.image, &_bb_out_image.allocation, nullptr);
            VkImageViewCreateInfo rview_info = imageview_create_info(_bb_out_image.format, _bb_out_image.image, _bb_out_image.mipmaps, VK_IMAGE_ASPECT_COLOR_BIT);

            VK_CHECK(vkCreateImageView(_vk_device, &rview_info, nullptr, &_bb_out_image.image_view));

            _bb_out_image_2.format = VK_FORMAT_R8G8B8A8_SRGB;
            _bb_out_image_2.extent = bb_out_extent;
            _bb_out_image_2.mipmaps = 1;

            VkImageCreateInfo rimg_info2 = image_create_info(_bb_out_image_2.format, _bb_out_image_usages, bb_out_extent);

            vmaCreateImage(_allocator, &rimg_info2, &rimg_allocinfo, &_bb_out_image_2.image, &_bb_out_image_2.allocation, nullptr);
            VkImageViewCreateInfo rview_info2 = imageview_create_info(_bb_out_image_2.format, _bb_out_image_2.image, _bb_out_image_2.mipmaps, VK_IMAGE_ASPECT_COLOR_BIT);

            VK_CHECK(vkCreateImageView(_vk_device, &rview_info2, nullptr, &_bb_out_image_2.image_view));

            _main_deletion_queue.push_function([&]() {
                destroy_image(_bb_out_image);
                destroy_image(_bb_out_image_2);
                vkDestroyPipelineLayout(_vk_device, _bb_pipeline_layout, nullptr);
                vkDestroyPipeline(_vk_device, _bb_pipeline, nullptr);
                vkDestroyDescriptorSetLayout(_vk_device, _bb_descriptor_layout, nullptr);
            });
        }

        void blur_image(VkCommandBuffer cmd, AllocatedImage image, int iterations) {
            auto blur_pass = [&](AllocatedImage& src, AllocatedImage& dst, int is_vertical) {
                transition_image(cmd, dst.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

                VkClearValue clear = { .color = { {0.f, 0.f, 0.f, 1.f} } };
                VkRenderingAttachmentInfo color_attach = attachment_info(dst.image_view, &clear, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
                VkExtent2D dst_extent = { dst.extent.width, dst.extent.height };

                VkRenderingInfo render_info = {};
                render_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
                render_info.pNext = nullptr;
                render_info.renderArea = VkRect2D{VkOffset2D{0, 0}, dst_extent};
                render_info.layerCount = 1;
                render_info.colorAttachmentCount = 1;
                render_info.pColorAttachments = &color_attach;
                render_info.pDepthAttachment = nullptr;
                render_info.pStencilAttachment = nullptr;

                vkCmdBeginRendering(cmd, &render_info);

                VkViewport viewport = {};
                viewport.x = 0;
                viewport.y = 0;
                viewport.width = static_cast<float>(dst_extent.width);
                viewport.height = static_cast<float>(dst_extent.height);
                viewport.minDepth = 0.f;
                viewport.maxDepth = 1.f;
                vkCmdSetViewport(cmd, 0, 1, &viewport);

                VkRect2D scissor = {};
                scissor.offset = {0, 0};
                scissor.extent = dst_extent;
                vkCmdSetScissor(cmd, 0, 1, &scissor);

                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _bb_pipeline);

                VkDescriptorSet blur_set = get_current_frame()._frame_descriptors.allocate(_vk_device, _bb_descriptor_layout, nullptr);
                DescriptorWriter writer;
                writer.write_image(0, src.image_view, _default_sampler_linear, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
                writer.update_set(_vk_device, blur_set);

                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _bb_pipeline_layout, 0, 1, &blur_set, 0, nullptr);

                BoxBlurPushConstants push_constants{};
                push_constants.radius = box_blur_radius;
                push_constants.is_vertical = is_vertical;
                vkCmdPushConstants(cmd, _bb_pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(BoxBlurPushConstants), &push_constants);

                vkCmdDraw(cmd, 3, 1, 0, 0);

                vkCmdEndRendering(cmd);

                transition_image(cmd, dst.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            };

            if (iterations <= 0) return;

            blur_pass(image, _bb_out_image, 0);
            blur_pass(_bb_out_image, _bb_out_image_2, 1);

            for (int i = 1; i < iterations; i++) {
                blur_pass(_bb_out_image_2, _bb_out_image, 0);
                blur_pass(_bb_out_image, _bb_out_image_2, 1);
            }

            transition_image(cmd, _bb_out_image_2.image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
            transition_image(cmd, image.image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

            VkExtent2D blit_src_extent = { _bb_out_image_2.extent.width, _bb_out_image_2.extent.height };
            VkExtent2D blit_dst_extent = { image.extent.width, image.extent.height };
            copy_image_to_image(cmd, _bb_out_image_2.image, image.image, blit_src_extent, blit_dst_extent);

            transition_image(cmd, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            transition_image(cmd, _bb_out_image_2.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }

        void _init_bloom() {

        }

        void _init_tonemapping() {

        }

        void init() {
            int w_width, w_height;
            SDL_GetWindowSize(window, &w_width, &w_height);

            VkExtent3D out_image_extent = {};
            out_image_extent.width = static_cast<uint32_t>(w_width);
            out_image_extent.height = static_cast<uint32_t>(w_height);
            out_image_extent.depth = 1;

            out_image.format = VK_FORMAT_R8G8B8A8_SRGB;
            out_image.extent = out_image_extent;
            out_image.mipmaps = 1;

            VkImageUsageFlags out_image_usages{};
            out_image_usages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
            out_image_usages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            out_image_usages |= VK_IMAGE_USAGE_STORAGE_BIT;
            out_image_usages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            out_image_usages |= VK_IMAGE_USAGE_SAMPLED_BIT;

            VkImageCreateInfo rimg_info = image_create_info(out_image.format, out_image_usages, out_image_extent);

            VmaAllocationCreateInfo rimg_allocinfo = {};
            rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
            rimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            vmaCreateImage(_allocator, &rimg_info, &rimg_allocinfo, &out_image.image, &out_image.allocation, nullptr);
            VkImageViewCreateInfo rview_info = imageview_create_info(out_image.format, out_image.image, out_image.mipmaps, VK_IMAGE_ASPECT_COLOR_BIT);

            VK_CHECK(vkCreateImageView(_vk_device, &rview_info, nullptr, &out_image.image_view));

            _main_deletion_queue.push_function([&]() {
                destroy_image(out_image);
            });

            _init_vignette();
            _init_gaussian_blur();
            _init_box_blur();
            _init_bloom();
            _init_tonemapping();
        }
    };

    PostProcessingStack main_post_processing_stack;

    struct Material {
        AllocatedImage albedo_map;
        AllocatedImage normal_map;
        AllocatedImage rougness_map;
        AllocatedImage metallic_map;
        AllocatedImage emissive_map;
        AllocatedImage ao_map;

        float scalar_tint;
        float roughness;
        float metallic;
    };

    GPUMeshBuffers upload_mesh(span<uint32_t> indices, span<Vertex> vertices) {
        const size_t vb_size = vertices.size() * sizeof(Vertex);
        const size_t ib_size = indices.size() * sizeof(uint32_t);

        GPUMeshBuffers new_surface;

        new_surface.vertex_buffer = create_buffer(vb_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
        VkBufferDeviceAddressInfo device_address_info{.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = new_surface.vertex_buffer.buffer};
        new_surface.vertex_buffer_address = vkGetBufferDeviceAddress(_vk_device, &device_address_info);

        new_surface.index_buffer = create_buffer(ib_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

        AllocatedBuffer staging = create_buffer(vb_size+ib_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

        void* data = staging.allocation->GetMappedData();

        memcpy(data, vertices.data(), vb_size);
        memcpy((char*)data + vb_size, indices.data(), ib_size);

        immediate_submit([&](VkCommandBuffer cmd) {
            VkBufferCopy vertexCopy{ 0 };
            vertexCopy.dstOffset = 0;
            vertexCopy.srcOffset = 0;
            vertexCopy.size = vb_size;

            vkCmdCopyBuffer(cmd, staging.buffer, new_surface.vertex_buffer.buffer, 1, &vertexCopy);

            VkBufferCopy indexCopy{ 0 };
            indexCopy.dstOffset = 0;
            indexCopy.srcOffset = vb_size;
            indexCopy.size = ib_size;

            vkCmdCopyBuffer(cmd, staging.buffer, new_surface.index_buffer.buffer, 1, &indexCopy);
        });

        destroy_buffer(staging);

        return new_surface;
    }

    void init_skybox() {
        vector<Vertex> skybox_vertices_as_proper_struct_because_im_lazy_to_hardcode_them;
        for (int i = 0; i<(static_cast<int>(skybox_vertices.size())-2); i=i+3) {
            Vertex temp_vert;
            temp_vert.normal = {1.f, 1.f, 1.f};
            temp_vert.position.x = skybox_vertices[i];
            temp_vert.position.y = skybox_vertices[i+1];
            temp_vert.position.z = skybox_vertices[i+2];
            temp_vert.uv_x = 1.f;
            temp_vert.uv_y = 1.f;

            skybox_vertices_as_proper_struct_because_im_lazy_to_hardcode_them.push_back(temp_vert);
        }

        skybox.mesh_buffers = upload_mesh(skybox_indices, skybox_vertices_as_proper_struct_because_im_lazy_to_hardcode_them);

        _main_deletion_queue.push_function([&]() {
            destroy_buffer(skybox.mesh_buffers.vertex_buffer);
            destroy_buffer(skybox.mesh_buffers.index_buffer);
        });

        int w_width, w_height;
        SDL_GetWindowSize(window, &w_width, &w_height);

        VkExtent3D sb_draw_image_extent = {};
        sb_draw_image_extent.width = static_cast<uint32_t>(w_width);
        sb_draw_image_extent.height = static_cast<uint32_t>(w_height);
        sb_draw_image_extent.depth = 1;

        _skybox_draw_image.format = VK_FORMAT_R8G8B8A8_SRGB;
        _skybox_draw_image.extent = sb_draw_image_extent;
        _skybox_draw_image.mipmaps = 1;

        VkImageUsageFlags draw_image_usages{};
        draw_image_usages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        draw_image_usages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        draw_image_usages |= VK_IMAGE_USAGE_STORAGE_BIT;
        draw_image_usages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        draw_image_usages |= VK_IMAGE_USAGE_SAMPLED_BIT;

        VkImageCreateInfo rimg_info = image_create_info(_skybox_draw_image.format, draw_image_usages, sb_draw_image_extent);

        VmaAllocationCreateInfo rimg_allocinfo = {};
        rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        rimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        vmaCreateImage(_allocator, &rimg_info, &rimg_allocinfo, &_skybox_draw_image.image, &_skybox_draw_image.allocation, nullptr);
        VkImageViewCreateInfo rview_info = imageview_create_info(_skybox_draw_image.format, _skybox_draw_image.image, _skybox_draw_image.mipmaps, VK_IMAGE_ASPECT_COLOR_BIT);

        VK_CHECK(vkCreateImageView(_vk_device, &rview_info, nullptr, &_skybox_draw_image.image_view));

        _main_deletion_queue.push_function([=]() {
            vkDestroyImageView(_vk_device, _skybox_draw_image.image_view, nullptr);
            vmaDestroyImage(_allocator, _skybox_draw_image.image, _skybox_draw_image.allocation);
        });

        // msaa for the skybox
        _skybox_draw_image_msaa.format = VK_FORMAT_R8G8B8A8_SRGB;
        _skybox_draw_image_msaa.extent = sb_draw_image_extent;
        _skybox_draw_image_msaa.mipmaps = 1;
        draw_image_usages = {};
        draw_image_usages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        draw_image_usages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        rimg_info.usage = draw_image_usages;
        rimg_info.samples = msaa_sample_count;
        rimg_info.mipLevels = 1;

        vmaCreateImage(_allocator, &rimg_info, &rimg_allocinfo, &_skybox_draw_image_msaa.image, &_skybox_draw_image_msaa.allocation, nullptr);
        VkImageViewCreateInfo rmview_info = imageview_create_info(_skybox_draw_image_msaa.format, _skybox_draw_image_msaa.image, _skybox_draw_image_msaa.mipmaps, VK_IMAGE_ASPECT_COLOR_BIT);
        rmview_info.subresourceRange.levelCount = 1;

        VK_CHECK(vkCreateImageView(_vk_device, &rmview_info, nullptr, &_skybox_draw_image_msaa.image_view));

        _main_deletion_queue.push_function([=]() {
            vkDestroyImageView(_vk_device, _skybox_draw_image_msaa.image_view, nullptr);
            vmaDestroyImage(_allocator, _skybox_draw_image_msaa.image, _skybox_draw_image_msaa.allocation);
        });

        DescriptorLayoutBuilder builder;
        builder.add_binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        skybox.desc_layout = builder.build(_vk_device, VK_SHADER_STAGE_FRAGMENT_BIT);

        VkPushConstantRange buffer_range{};
        buffer_range.offset = 0;
        buffer_range.size = sizeof(GPUDrawPushConstants);
        buffer_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkPipelineLayoutCreateInfo layout_info = pipeline_layout_create_info();
        layout_info.pPushConstantRanges = &buffer_range;
        layout_info.pushConstantRangeCount = 1;
        layout_info.pSetLayouts = &skybox.desc_layout;
        layout_info.setLayoutCount = 1;
        vkCreatePipelineLayout(_vk_device, &layout_info, nullptr, &skybox.piplayout);

        VkShaderModule vert_shader, frag_shader;
        if (!load_shader_module("../shaders/skybox.vert.spv", _vk_device, &vert_shader)) {
            cout << "error when loading skybox vert shader" << endl;
        }
        if (!load_shader_module("../shaders/skybox.frag.spv", _vk_device, &frag_shader)) {
            cout << "error when loading skybox frag shader" << endl;
        }

        PipelineBuilder pip_builder;
        pip_builder._pipeline_layout = skybox.piplayout;
        pip_builder.set_shaders(vert_shader, frag_shader);
        pip_builder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        pip_builder.set_polygon_mode(VK_POLYGON_MODE_FILL);
        // if you can't see anything it might just be the backface culling so remember that
        pip_builder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
        pip_builder.enable_multisampling(msaa_sample_count);
        pip_builder.disable_blending();
        pip_builder.disable_depthtest();
        pip_builder.set_color_attachment_format(_skybox_draw_image.format);
        pip_builder.set_depth_format(_depth_image.format);
        skybox.pip = pip_builder.build_pipeline(_vk_device);

        vkDestroyShaderModule(_vk_device, vert_shader, nullptr);
        vkDestroyShaderModule(_vk_device, frag_shader, nullptr);

        _main_deletion_queue.push_function([&]() {
            vkDestroyPipelineLayout(_vk_device, skybox.piplayout, nullptr);
            vkDestroyPipeline(_vk_device, skybox.pip, nullptr);
            vkDestroyDescriptorSetLayout(_vk_device, skybox.desc_layout, nullptr);
        });
    }

    void init_composite() {
        DescriptorLayoutBuilder builder;
        builder.add_binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); // scene
        builder.add_binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); // skybox
        builder.add_binding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); // 2d
        _composite_descriptor_layout = builder.build(_vk_device, VK_SHADER_STAGE_FRAGMENT_BIT);

        VkPipelineLayoutCreateInfo layout_info = pipeline_layout_create_info();
        layout_info.setLayoutCount = 1;
        layout_info.pSetLayouts = &_composite_descriptor_layout;
        vkCreatePipelineLayout(_vk_device, &layout_info, nullptr, &_composite_pipeline_layout);

        VkShaderModule vert_shader, frag_shader;
        if (!load_shader_module("../shaders/fullscreen_triangle.vert.spv", _vk_device, &vert_shader)) {
            cout << "error when loading composite vert shader" << endl;
        }
        if (!load_shader_module("../shaders/composite.frag.spv", _vk_device, &frag_shader)) {
            cout << "error when loading composite frag shader" << endl;
        }

        PipelineBuilder pip_builder;
        pip_builder._pipeline_layout = _composite_pipeline_layout;
        pip_builder.set_shaders(vert_shader, frag_shader);
        pip_builder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        pip_builder.set_polygon_mode(VK_POLYGON_MODE_FILL);
        pip_builder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
        pip_builder.set_multisampling_none();
        pip_builder.disable_blending();
        pip_builder.disable_depthtest();
        pip_builder.set_color_attachment_format(_draw_image.format);
        _composite_pipeline = pip_builder.build_pipeline(_vk_device);

        vkDestroyShaderModule(_vk_device, vert_shader, nullptr);
        vkDestroyShaderModule(_vk_device, frag_shader, nullptr);

        _main_deletion_queue.push_function([&]() {
            vkDestroyPipelineLayout(_vk_device, _composite_pipeline_layout, nullptr);
            vkDestroyPipeline(_vk_device, _composite_pipeline, nullptr);
            vkDestroyDescriptorSetLayout(_vk_device, _composite_descriptor_layout, nullptr);
        });

        int w_width, w_height;
        SDL_GetWindowSize(window, &w_width, &w_height);

        VkExtent3D composite_image_extent = {};
        composite_image_extent.width = static_cast<uint32_t>(w_width);
        composite_image_extent.height = static_cast<uint32_t>(w_height);
        composite_image_extent.depth = 1;

        _composite_image.format = VK_FORMAT_R8G8B8A8_SRGB;
        _composite_image.extent = composite_image_extent;
        _composite_image.mipmaps = 1;

        VkImageUsageFlags composite_image_usages{};
        composite_image_usages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        composite_image_usages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        composite_image_usages |= VK_IMAGE_USAGE_STORAGE_BIT;
        composite_image_usages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        composite_image_usages |= VK_IMAGE_USAGE_SAMPLED_BIT;

        VkImageCreateInfo rimg_info = image_create_info(_composite_image.format, composite_image_usages, composite_image_extent);

        VmaAllocationCreateInfo rimg_allocinfo = {};
        rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        rimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        vmaCreateImage(_allocator, &rimg_info, &rimg_allocinfo, &_composite_image.image, &_composite_image.allocation, nullptr);
        VkImageViewCreateInfo rview_info = imageview_create_info(_composite_image.format, _composite_image.image, _composite_image.mipmaps, VK_IMAGE_ASPECT_COLOR_BIT);

        VK_CHECK(vkCreateImageView(_vk_device, &rview_info, nullptr, &_composite_image.image_view));

        _main_deletion_queue.push_function([&]() {
            destroy_image(_composite_image);
        });
    }

    void init_2d() {
        int w_width, w_height;
        SDL_GetWindowSize(window, &w_width, &w_height);

        display.clear(w_width, w_height, {0.f, 0.f, 0.f, 0.f});
        display.refresh();
    }

    void init_imgui() {
        VkDescriptorPoolSize pool_sizes[] = { { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 } };

        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1000;
        pool_info.poolSizeCount = (uint32_t)size(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;

        VkDescriptorPool imguiPool;
        VK_CHECK(vkCreateDescriptorPool(_vk_device, &pool_info, nullptr, &imguiPool));

        ImGui::CreateContext();

        ImGui_ImplSDL3_InitForVulkan(window);

        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance = _vk_instance;
        init_info.PhysicalDevice = _chosen_GPU;
        init_info.Device = _vk_device;
        init_info.Queue = _graphics_queue;
        init_info.DescriptorPool = imguiPool;
        init_info.MinImageCount = 3;
        init_info.ImageCount = 3;
        init_info.UseDynamicRendering = true;

        init_info.PipelineInfoMain.PipelineRenderingCreateInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
        init_info.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
        init_info.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &_swapchain_image_format;


        init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

        ImGui_ImplVulkan_Init(&init_info);

        // this should be here, but I think init replaces it now: ImGui_ImplVulkan_CreateFontsTexture();
        // if text isn't working then just uncomment it and find an alternative I guess

        _main_deletion_queue.push_function([=]() {
            ImGui_ImplVulkan_Shutdown();
            vkDestroyDescriptorPool(_vk_device, imguiPool, nullptr);
        });
    }

    void draw_imgui(VkCommandBuffer cmd, VkImageView targetImageView)
    {
        VkRenderingAttachmentInfo colorAttachment = attachment_info(targetImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        VkRenderingInfo renderInfo{};
        renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderInfo.pNext = nullptr;
        renderInfo.renderArea = VkRect2D{ VkOffset2D{ 0, 0 }, _swapchain_extent };
        renderInfo.layerCount = 1;
        renderInfo.colorAttachmentCount = 1;
        renderInfo.pColorAttachments = &colorAttachment;
        renderInfo.pDepthAttachment = nullptr;
        renderInfo.pStencilAttachment = nullptr;

        vkCmdBeginRendering(cmd, &renderInfo);

        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

        vkCmdEndRendering(cmd);
    }

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

        // memory allocation
        VmaAllocatorCreateInfo allocator_info = {};
        allocator_info.physicalDevice = _chosen_GPU;
        allocator_info.device = _vk_device;
        allocator_info.instance = _vk_instance;
        allocator_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        vmaCreateAllocator(&allocator_info, &_allocator);
        _main_deletion_queue.push_function([&]() {vmaDestroyAllocator(_allocator);});
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

        VkExtent3D draw_image_extent = {};
        draw_image_extent.width = static_cast<uint32_t>(w_width);
        draw_image_extent.height = static_cast<uint32_t>(w_height);
        draw_image_extent.depth = 1;

        _draw_image.format = VK_FORMAT_R8G8B8A8_SRGB;
        _draw_image.extent = draw_image_extent;
        _draw_image.mipmaps = 1;

        VkImageUsageFlags draw_image_usages{};
        draw_image_usages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        draw_image_usages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        draw_image_usages |= VK_IMAGE_USAGE_STORAGE_BIT;
        draw_image_usages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        draw_image_usages |= VK_IMAGE_USAGE_SAMPLED_BIT;

        VkImageCreateInfo rimg_info = image_create_info(_draw_image.format, draw_image_usages, draw_image_extent);

        VmaAllocationCreateInfo rimg_allocinfo = {};
        rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        rimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        vmaCreateImage(_allocator, &rimg_info, &rimg_allocinfo, &_draw_image.image, &_draw_image.allocation, nullptr);
        VkImageViewCreateInfo rview_info = imageview_create_info(_draw_image.format, _draw_image.image, _draw_image.mipmaps, VK_IMAGE_ASPECT_COLOR_BIT);

        VK_CHECK(vkCreateImageView(_vk_device, &rview_info, nullptr, &_draw_image.image_view));

        _depth_image.format = VK_FORMAT_D32_SFLOAT;
        _depth_image.extent = draw_image_extent;
        _depth_image.mipmaps = 1;
        VkImageUsageFlags depthImageUsages{};
        depthImageUsages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

        VkImageCreateInfo dimg_info = image_create_info(_depth_image.format, depthImageUsages, draw_image_extent);

        vmaCreateImage(_allocator, &dimg_info, &rimg_allocinfo, &_depth_image.image, &_depth_image.allocation, nullptr);

        VkImageViewCreateInfo dview_info = imageview_create_info(_depth_image.format, _depth_image.image, _depth_image.mipmaps, VK_IMAGE_ASPECT_DEPTH_BIT);

        VK_CHECK(vkCreateImageView(_vk_device, &dview_info, nullptr, &_depth_image.image_view));

        _main_deletion_queue.push_function([=]() {
            vkDestroyImageView(_vk_device, _draw_image.image_view, nullptr);
            vmaDestroyImage(_allocator, _draw_image.image, _draw_image.allocation);

            vkDestroyImageView(_vk_device, _depth_image.image_view, nullptr);
            vmaDestroyImage(_allocator, _depth_image.image, _depth_image.allocation);
        });
    }

    void init_msaa() {
        int w_width, w_height;
        SDL_GetWindowSize(window, &w_width, &w_height);

        VkExtent3D draw_image_extent = {};
        draw_image_extent.width = static_cast<uint32_t>(w_width);
        draw_image_extent.height = static_cast<uint32_t>(w_height);
        draw_image_extent.depth = 1;

        _draw_image_msaa.format = VK_FORMAT_R8G8B8A8_SRGB;
        _draw_image_msaa.extent = draw_image_extent;
        _draw_image_msaa.mipmaps = 1;

        VkImageUsageFlags draw_image_usages{};
        draw_image_usages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        draw_image_usages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

        VkImageCreateInfo rimg_info = image_create_info(_draw_image_msaa.format, draw_image_usages, draw_image_extent);
        rimg_info.samples = msaa_sample_count;
        rimg_info.mipLevels = 1;

        VmaAllocationCreateInfo rimg_allocinfo = {};
        rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        rimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        vmaCreateImage(_allocator, &rimg_info, &rimg_allocinfo, &_draw_image_msaa.image, &_draw_image_msaa.allocation, nullptr);
        VkImageViewCreateInfo rview_info = imageview_create_info(_draw_image_msaa.format, _draw_image_msaa.image, _draw_image_msaa.mipmaps, VK_IMAGE_ASPECT_COLOR_BIT);
        rview_info.subresourceRange.levelCount = 1;

        VK_CHECK(vkCreateImageView(_vk_device, &rview_info, nullptr, &_draw_image_msaa.image_view));

        _depth_image_msaa.format = VK_FORMAT_D32_SFLOAT;
        _depth_image_msaa.extent = draw_image_extent;
        _depth_image_msaa.mipmaps = 1;
        VkImageUsageFlags depthImageUsages{};
        depthImageUsages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

        VkImageCreateInfo dimg_info = image_create_info(_depth_image_msaa.format, depthImageUsages, draw_image_extent);
        dimg_info.samples = msaa_sample_count;
        dimg_info.mipLevels = 1;

        vmaCreateImage(_allocator, &dimg_info, &rimg_allocinfo, &_depth_image_msaa.image, &_depth_image_msaa.allocation, nullptr);

        VkImageViewCreateInfo dview_info = imageview_create_info(_depth_image_msaa.format, _depth_image_msaa.image, _depth_image_msaa.mipmaps, VK_IMAGE_ASPECT_DEPTH_BIT);
        dview_info.subresourceRange.levelCount = 1;

        VK_CHECK(vkCreateImageView(_vk_device, &dview_info, nullptr, &_depth_image_msaa.image_view));

        _main_deletion_queue.push_function([=]() {
            vkDestroyImageView(_vk_device, _draw_image_msaa.image_view, nullptr);
            vmaDestroyImage(_allocator, _draw_image_msaa.image, _draw_image_msaa.allocation);

            vkDestroyImageView(_vk_device, _depth_image_msaa.image_view, nullptr);
            vmaDestroyImage(_allocator, _depth_image_msaa.image, _depth_image_msaa.allocation);
        });
    }

    void init_commands() {
        VkCommandPoolCreateInfo command_pool_info = init_command_pool_create_info(_graphics_queue_family, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

        for (int i = 0; i < FRAME_OVERLAP; i++) {
            VK_CHECK(vkCreateCommandPool(_vk_device, &command_pool_info, nullptr, &_frames[i]._commandPool));

            VkCommandBufferAllocateInfo cmd_alloc_info = init_command_buffer_allocate_info(_frames[i]._commandPool, 1);

            VK_CHECK(vkAllocateCommandBuffers(_vk_device, &cmd_alloc_info, &_frames[i]._mainCommandBuffer));
        }

        // imgui command pool
        VK_CHECK(vkCreateCommandPool(_vk_device, &command_pool_info, nullptr, &_imm_command_pool));
        VkCommandBufferAllocateInfo cmd_alloc_info = init_command_buffer_allocate_info(_imm_command_pool, 1);
        VK_CHECK(vkAllocateCommandBuffers(_vk_device, &cmd_alloc_info, &_imm_command_buffer));

        _main_deletion_queue.push_function([=]() {
            vkDestroyCommandPool(_vk_device, _imm_command_pool, nullptr);
        });
    }

    void init_sync_structures() {
        VkFenceCreateInfo fence_create_info = create_fence_info(VK_FENCE_CREATE_SIGNALED_BIT);
        VkSemaphoreCreateInfo semaphore_create_info = create_semaphore_info(0);

        for (int i = 0; i < FRAME_OVERLAP; i++) {
            VK_CHECK(vkCreateFence(_vk_device, &fence_create_info, nullptr, &_frames[i]._render_fence));

            VK_CHECK(vkCreateSemaphore(_vk_device, &semaphore_create_info, nullptr, &_frames[i]._swapchain_semaphore));
            VK_CHECK(vkCreateSemaphore(_vk_device, &semaphore_create_info, nullptr, &_frames[i]._render_semaphore));
        }

        // imgui
        VK_CHECK(vkCreateFence(_vk_device, &fence_create_info, nullptr, &_imm_fence));
        _main_deletion_queue.push_function([=]() {
            vkDestroyFence(_vk_device, _imm_fence, nullptr);
        });
    }

    void init_default_data() {
        uint32_t white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
        _white_image = create_image((void*)&white, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

        uint32_t grey = glm::packUnorm4x8(glm::vec4(0.66f, 0.66f, 0.66f, 1));
        _gray_image = create_image((void*)&grey, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

        uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 1.f));
        _black_image = create_image((void*)&black, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

        uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
        std::array<uint32_t, 16 *16 > pixels;
        for (int x = 0; x < 16; x++) {
            for (int y = 0; y < 16; y++) {
                pixels[y*16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
            }
        }
        _error_checkerboard_image = create_image(pixels.data(), VkExtent3D{16, 16, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

        VkSamplerCreateInfo sampl = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};

        sampl.magFilter = VK_FILTER_NEAREST;
        sampl.minFilter = VK_FILTER_NEAREST;
        sampl.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        sampl.maxLod = VK_LOD_CLAMP_NONE;
        vkCreateSampler(_vk_device, &sampl, nullptr, &_default_sampler_nearest);

        sampl.magFilter = VK_FILTER_LINEAR;
        sampl.minFilter = VK_FILTER_LINEAR;
        sampl.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        sampl.maxLod = VK_LOD_CLAMP_NONE;
        vkCreateSampler(_vk_device, &sampl, nullptr, &_default_sampler_linear);

        _main_deletion_queue.push_function([&](){
            vkDestroySampler(_vk_device, _default_sampler_linear, nullptr);
            vkDestroySampler(_vk_device, _default_sampler_nearest, nullptr);

            destroy_image(_white_image);
            destroy_image(_gray_image);
            destroy_image(_black_image);
            destroy_image(_error_checkerboard_image);
        });
    }

    void init_mesh_pipeline() {
        VkShaderModule triangleFragShader;
        if (!load_shader_module("../shaders/mesh_frag.frag.spv", _vk_device, &triangleFragShader)) {
            fmt::println("error when building the mesh fragment shader");
        }
        else {
            fmt::println("mesh fragment shader succesfully loaded");
        }

        VkShaderModule triangleVertexShader;
        if (!load_shader_module("../shaders/mesh_vertex.vert.spv", _vk_device, &triangleVertexShader)) {
            fmt::println("error when building the mesh vertex shader module");
        }
        else {
            fmt::println("mesh vertex shader succesfully loaded");
        }

        VkPushConstantRange bufferRange{};
        bufferRange.offset = 0;
        bufferRange.size = sizeof(GPUDrawPushConstants);
        bufferRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkPipelineLayoutCreateInfo pipeline_layout_info = pipeline_layout_create_info();
        pipeline_layout_info.pPushConstantRanges = &bufferRange;
        pipeline_layout_info.pushConstantRangeCount = 1;
        pipeline_layout_info.pSetLayouts = &_single_image_descriptor_layout;
        pipeline_layout_info.setLayoutCount = 1;

        VK_CHECK(vkCreatePipelineLayout(_vk_device, &pipeline_layout_info, nullptr, &_mesh_pipeline_layout));

        PipelineBuilder pipelineBuilder;

        pipelineBuilder._pipeline_layout = _mesh_pipeline_layout;
        pipelineBuilder.set_shaders(triangleVertexShader, triangleFragShader);
        pipelineBuilder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        pipelineBuilder.set_polygon_mode(VK_POLYGON_MODE_FILL);
        pipelineBuilder.set_cull_mode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
        pipelineBuilder.enable_multisampling(msaa_sample_count);
        pipelineBuilder.enable_blending_alphablend();

        pipelineBuilder.enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

        pipelineBuilder.set_color_attachment_format(_draw_image.format);
        pipelineBuilder.set_depth_format(_depth_image.format);

        _mesh_pipeline = pipelineBuilder.build_pipeline(_vk_device);

        vkDestroyShaderModule(_vk_device, triangleFragShader, nullptr);
        vkDestroyShaderModule(_vk_device, triangleVertexShader, nullptr);

        _main_deletion_queue.push_function([&]() {
            vkDestroyPipelineLayout(_vk_device, _mesh_pipeline_layout, nullptr);
            vkDestroyPipeline(_vk_device, _mesh_pipeline, nullptr);
        });
    }

    void draw_mesh(shared_ptr<MeshAsset> mesh, AllocatedImage texture = _error_checkerboard_image, glm::vec3 position = {0, 0, 0}, glm::vec3 scale = {1, 1, 1}, glm::quat rotation = {1, 0, 0, 0}) {
        render_queue.push_back(RenderQueueMesh{
            .mesh = mesh,
            .image = texture,
            .position = position,
            .scale = scale,
            .rotation = rotation
        });
    }

    void draw_mesh(RenderQueueMesh render_queue_mesh) {
        render_queue.push_back(render_queue_mesh);
    }

    void draw_skybox_pass(VkCommandBuffer cmd) {
        VkClearValue clear = { .color = { {clear_color.r, clear_color.g, clear_color.b, clear_color.a} } };
        VkRenderingAttachmentInfo color_attachment = attachment_info(_skybox_draw_image_msaa.image_view, &clear, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        // this fella right here might cause an issue but just like might, like probably not but if something goes wrong with the depth I'm gonna point my finger right here and call past me very stupid and lazy for reusing this
        VkRenderingAttachmentInfo depth_attachment = depth_attachment_info(_depth_image_msaa.image_view, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
        VkRenderingInfo rendering_info = {};
        rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        rendering_info.pNext = nullptr;
        rendering_info.renderArea = VkRect2D{VkOffset2D{0, 0}, _draw_extent};
        rendering_info.layerCount = 1;
        rendering_info.colorAttachmentCount = 1;
        rendering_info.pColorAttachments = &color_attachment;
        rendering_info.pDepthAttachment = &depth_attachment;
        rendering_info.pStencilAttachment = nullptr;

        vkCmdBeginRendering(cmd, &rendering_info);
        VkViewport viewport = {};
        viewport.x = 0;
        viewport.y = 0;
        viewport.width = static_cast<float>(_draw_extent.width);
        viewport.height = static_cast<float>(_draw_extent.height);
        viewport.minDepth = 0.f;
        viewport.maxDepth = 1.f;
        vkCmdSetViewport(cmd, 0, 1, &viewport);

        VkRect2D scissor = {};
        scissor.offset = {0, 0};
        scissor.extent = _draw_extent;
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, skybox.pip);

        GPUDrawPushConstants push_constants;
        glm::mat4 projection = glm::perspective(glm::radians(fov), static_cast<float>(_draw_extent.width) / static_cast<float>(_draw_extent.height), 10000.f, 0.1f);
        projection[1][1] *= -1;
        glm::mat4 view = glm::lookAt(camera.position, camera.position + camera.direction, glm::vec3{ 0.f, 1.f, 0.f });
        push_constants.world_matrix = projection*glm::mat4(glm::mat3(view));

        VkDescriptorSet imageSet = get_current_frame()._frame_descriptors.allocate(_vk_device, skybox.desc_layout);
        {
            DescriptorWriter writer;
            writer.write_image(0, skybox.cubemap.image_view, skybox.cubemap.sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

            writer.update_set(_vk_device, imageSet);
        }

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, skybox.piplayout, 0, 1, &imageSet, 0, nullptr);

        push_constants.vertex_buffer = skybox.mesh_buffers.vertex_buffer_address;

        vkCmdPushConstants(cmd, skybox.piplayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUDrawPushConstants), &push_constants);
        vkCmdBindIndexBuffer(cmd, skybox.mesh_buffers.index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdDrawIndexed(cmd, 36, 1, 0, 0, 0);

        vkCmdEndRendering(cmd);
    }

    void draw_geometry(VkCommandBuffer cmd) {
        VkClearValue clear = { .color = { {clear_color.r, clear_color.g, clear_color.b, 0.f} } };
        VkRenderingAttachmentInfo color_attachment = attachment_info(_draw_image_msaa.image_view, &clear, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        VkRenderingAttachmentInfo depth_attachment = depth_attachment_info(_depth_image_msaa.image_view, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
        VkRenderingInfo render_info = rendering_info(_draw_extent, &color_attachment, &depth_attachment);

        AllocatedBuffer gpu_scene_data_buffer = create_buffer(sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

        get_current_frame()._deletion_queue.push_function([=]() {
            destroy_buffer(gpu_scene_data_buffer);
        });

        GPUSceneData* scene_uniform_data = static_cast<GPUSceneData *>(gpu_scene_data_buffer.allocation->GetMappedData());
        *scene_uniform_data = scene_data;

        VkDescriptorSet global_descriptor = get_current_frame()._frame_descriptors.allocate(_vk_device, _gpu_scene_data_descriptor_layout);

        DescriptorWriter writer;
        writer.write_buffer(0, gpu_scene_data_buffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        writer.update_set(_vk_device, global_descriptor);

        vkCmdBeginRendering(cmd, &render_info);

        VkViewport viewport = {};
        viewport.x = 0;
        viewport.y = 0;
        viewport.width = (float)_draw_extent.width;
        viewport.height = (float)_draw_extent.height;
        viewport.minDepth = 0.f;
        viewport.maxDepth = 1.f;
        vkCmdSetViewport(cmd, 0, 1, &viewport);

        VkRect2D scissor = {};
        scissor.offset = {0, 0};
        scissor.extent = _draw_extent;
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _mesh_pipeline);

        GPUDrawPushConstants push_constants;
        glm::mat4 projection = glm::perspective(glm::radians(fov), static_cast<float>(_draw_extent.width) / static_cast<float>(_draw_extent.height), 10000.f, 0.1f);
        projection[1][1] *= -1;

        glm::mat4 view = glm::lookAt(camera.position, camera.position + camera.direction, glm::vec3{ 0.f, 1.f, 0.f });

        extract_frustum_planes(projection*view, fc_left_plane, fc_right_plane, fc_bottom_plane, fc_top_plane, fc_near_plane, fc_far_plane);

        for (RenderQueueMesh m : render_queue) {
            //  frustum culling
            glm::mat4 model = glm::translate(glm::mat4(1.f), m.position) * (glm::mat4_cast(m.rotation) * glm::scale(glm::mat4(1.f), m.scale));
            glm::vec3 min;
            glm::vec3 max;
            calculate_world_AABB(min, max, m);
            if (is_AABB_inside_frustum(min, max, fc_left_plane, fc_right_plane, fc_bottom_plane, fc_top_plane, fc_near_plane, fc_top_plane)) {
                push_constants.world_matrix = projection * view * model;

                VkDescriptorSet imageSet = get_current_frame()._frame_descriptors.allocate(_vk_device, _single_image_descriptor_layout);
                {
                    DescriptorWriter writer;
                    writer.write_image(0, m.image.image_view, _default_sampler_linear, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

                    writer.update_set(_vk_device, imageSet);
                }

                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _mesh_pipeline_layout, 0, 1, &imageSet, 0, nullptr);

                push_constants.vertex_buffer = m.mesh->mesh_buffers.vertex_buffer_address;

                vkCmdPushConstants(cmd, _mesh_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUDrawPushConstants), &push_constants);
                vkCmdBindIndexBuffer(cmd, m.mesh->mesh_buffers.index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);

                vkCmdDrawIndexed(cmd, m.mesh->surfaces[0].count, 1, m.mesh->surfaces[0].start_index, 0, 0);
            }
        }

        vkCmdEndRendering(cmd);
    }

    void draw_composite_pass(VkCommandBuffer cmd) {
        // I'm pretty sure I don't need to transition these images to SHADER_READ_ONLY_OPTIMAL cause it should already be that by now
        VkClearValue clear = { .color = { {0.f, 0.f, 0.f, 1.f} } };
        VkRenderingAttachmentInfo color_attach = attachment_info(_composite_image.image_view, &clear, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        VkRenderingInfo render_info = {};
        render_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        render_info.pNext = nullptr;
        render_info.renderArea = VkRect2D{VkOffset2D{0, 0}, _draw_extent};
        render_info.layerCount = 1;
        render_info.colorAttachmentCount = 1;
        render_info.pColorAttachments = &color_attach;
        render_info.pDepthAttachment = nullptr; // this may cause a problem idk
        render_info.pStencilAttachment = nullptr;

        vkCmdBeginRendering(cmd, &render_info);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _composite_pipeline);

        VkDescriptorSet composite_set = get_current_frame()._frame_descriptors.allocate(_vk_device, _composite_descriptor_layout, nullptr);
        DescriptorWriter writer;
        writer.write_image(0, _draw_image.image_view, _default_sampler_linear, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        writer.write_image(1, _skybox_draw_image.image_view, _default_sampler_linear, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        writer.write_image(2, display.vk_image.image_view, (display.sampler_type = VK_FILTER_LINEAR) ? _default_sampler_linear : _default_sampler_nearest, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        writer.update_set(_vk_device, composite_set);

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _composite_pipeline_layout, 0, 1, &composite_set, 0, nullptr);

        vkCmdDraw(cmd, 3, 1, 0, 0);

        vkCmdEndRendering(cmd);
    }

    void init_pipelines() {
        init_mesh_pipeline();
    }

    void init_descriptors() {
        vector<DescriptorAllocator::PoolSizeRatio> sizes = {
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1}
        };

        global_descriptor_allocator.init_pool(_vk_device, 20, sizes);

        {
            DescriptorLayoutBuilder builder;
            builder.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
            _draw_image_descriptor_layout = builder.build(_vk_device, VK_SHADER_STAGE_COMPUTE_BIT);
        }

        {
            DescriptorLayoutBuilder builder;
            builder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            _gpu_scene_data_descriptor_layout = builder.build(_vk_device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
        }

        {
            DescriptorLayoutBuilder builder;
            builder.add_binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            _single_image_descriptor_layout = builder.build(_vk_device, VK_SHADER_STAGE_FRAGMENT_BIT);
        }

        _draw_image_descriptors = global_descriptor_allocator.allocate(_vk_device, _draw_image_descriptor_layout);
        DescriptorWriter writer;
        writer.write_image(0, _draw_image.image_view, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

        writer.update_set(_vk_device, _draw_image_descriptors);

        _main_deletion_queue.push_function([&]() {
           global_descriptor_allocator.destroy_pool(_vk_device);

            vkDestroyDescriptorSetLayout(_vk_device, _draw_image_descriptor_layout, nullptr);
            vkDestroyDescriptorSetLayout(_vk_device, _gpu_scene_data_descriptor_layout, nullptr);
            vkDestroyDescriptorSetLayout(_vk_device, _single_image_descriptor_layout, nullptr);
        });

        for (int i = 0; i < FRAME_OVERLAP; i++) {
            std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> frame_sizes = {
                { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3 },
                { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 },
                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
                { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 },
            };

            _frames[i]._frame_descriptors = DescriptorAllocatorGrowable{};
            _frames[i]._frame_descriptors.init(_vk_device, 1000, frame_sizes);
        }
    }

    std::optional<AllocatedImage> load_image(std::filesystem::path path) {
        int width, height, channels;
        stbi_uc* data = stbi_load(path.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
        if (!data) {
            fmt::println("failed to load image at path: {}", path.string());
            return {};
        }

        VkExtent3D extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
        AllocatedImage image = create_image(data, extent, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_SAMPLED_BIT);

        stbi_image_free(data);
        return image;
    }

    CubeMap load_cubemap(filesystem::path path) {
        int src_width, src_height, src_channels;
        stbi_uc* src_data = stbi_load(path.string().c_str(), &src_width, &src_height, &src_channels, STBI_rgb_alpha);
        if (!src_data) {
            cout << "failed to load cubemap: " << path.string() << endl;
            return {};
        }

        int face_size = src_width/4;
        size_t face_data_size = face_size * face_size * 4;
        vector<uint8_t> face_pixels(6*face_data_size);

        auto sample_equirect = [&](glm::vec3 dir) -> glm::u8vec4 {
            dir = glm::normalize(dir);
            float phi = atan2(dir.z, dir.x);
            float theta = asin(glm::clamp(dir.y, -1.f, 1.f));
            float u = phi / (2.f * glm::pi<float>()) + 0.5f;
            float v = 0.5f - theta / glm::pi<float>();

            float fx = u * src_width - 0.5f;
            float fy = v * src_height - 0.5f;

            int x0 = static_cast<int>(floor(fx));
            int y0 = static_cast<int>(floor(fy));
            int x1 = x0 + 1;
            int y1 = y0 + 1;
            float tx = fx - x0;
            float ty = fy - y0;

            x0 = ((x0 % src_width) + src_width) % src_width;
            x1 = ((x1 % src_width) + src_width) % src_width;
            y0 = glm::clamp(y0, 0, src_height - 1);
            y1 = glm::clamp(y1, 0, src_height - 1);

            auto fetch = [&](int x, int y) -> glm::vec4 {
                int idx = (y * src_width + x) * 4;
                return glm::vec4(src_data[idx], src_data[idx+1], src_data[idx+2], src_data[idx+3]);
            };

            glm::vec4 c = glm::mix(
                glm::mix(fetch(x0, y0), fetch(x1, y0), tx),
                glm::mix(fetch(x0, y1), fetch(x1, y1), tx),
                ty
            );

            return glm::u8vec4(glm::clamp(c, 0.f, 255.f));
        };

        auto face_direction = [](int face, float u, float v) -> glm::vec3 {
            switch (face) {
                case 0: return {  1.f, -v, -u };
                case 1: return { -1.f, -v,  u };
                case 2: return {  u,   1.f,  v };
                case 3: return {  u,  -1.f, -v };
                case 4: return {  u,  -v,   1.f };
                case 5: return { -u,  -v,  -1.f };
                default: return { 0.f, 0.f, 1.f };
            }
        };

        for (int face = 0; face < 6; face++) {
            for (int py = 0; py < face_size; py++) {
                for (int px = 0; px < face_size; px++) {
                    float u = ((float)px + 0.5f) / face_size * 2.f - 1.f;
                    float v = ((float)py + 0.5f) / face_size * 2.f - 1.f;
                    glm::u8vec4 col = sample_equirect(face_direction(face, u, v));
                    size_t dst = ((size_t)face * face_size * face_size + py * face_size + px) * 4;
                    face_pixels[dst + 0] = col.r;
                    face_pixels[dst + 1] = col.g;
                    face_pixels[dst + 2] = col.b;
                    face_pixels[dst + 3] = col.a;
                }
            }
        }

        stbi_image_free(src_data);

        VkExtent3D face_extent = {static_cast<uint32_t>(face_size), static_cast<uint32_t>(face_size), 1};

        AllocatedBuffer staging = create_buffer(6*face_data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
        memcpy(staging.info.pMappedData, face_pixels.data(), 6*face_data_size);

        AllocatedImage new_image;
        new_image.format = VK_FORMAT_R8G8B8A8_SRGB;
        new_image.extent = face_extent;
        new_image.mipmaps = 1;

        VkImageCreateInfo img_info = image_create_info(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, face_extent);
        img_info.arrayLayers = 6;
        img_info.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

        VmaAllocationCreateInfo alloc_info = {};
        alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        alloc_info.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        VK_CHECK(vmaCreateImage(_allocator, &img_info, &alloc_info, &new_image.image, &new_image.allocation, nullptr));

        immediate_submit([&](VkCommandBuffer cmd) {
            transition_image(cmd, new_image.image, VK_IMAGE_LAYOUT_UNDEFINED,  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            for (int face = 0; face < 6; face++) {
                VkBufferImageCopy region = {};
                region.bufferOffset = face*face_data_size;
                region.bufferRowLength = 0;
                region.bufferImageHeight = 0;
                region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                region.imageSubresource.mipLevel = 0;
                region.imageSubresource.baseArrayLayer = face;
                region.imageSubresource.layerCount = 1;
                region.imageExtent = face_extent;
                vkCmdCopyBufferToImage(cmd, staging.buffer, new_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
            }

            transition_image(cmd, new_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        });

        destroy_buffer(staging);

        VkImageViewCreateInfo view_info = cubemap_imageview_create_info(VK_FORMAT_R8G8B8A8_SRGB, new_image.image);
        VK_CHECK(vkCreateImageView(_vk_device, &view_info, nullptr, &new_image.image_view));

        VkSamplerCreateInfo sampler_info = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        sampler_info.magFilter = VK_FILTER_LINEAR;
        sampler_info.minFilter = VK_FILTER_LINEAR;
        sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler_info.maxLod = VK_LOD_CLAMP_NONE;

        CubeMap result;
        result.image = new_image;
        result.image_view = new_image.image_view;
        VK_CHECK(vkCreateSampler(_vk_device, &sampler_info, nullptr, &result.sampler));

        _main_deletion_queue.push_function([&]() {
            destroy_image(new_image);
            vkDestroySampler(_vk_device, result.sampler, nullptr);
        });

        return result;
    }

    bool load_skybox(filesystem::path path) {
        if (skybox.cubemap.sampler != VK_NULL_HANDLE) {
            vkDestroySampler(_vk_device, skybox.cubemap.sampler, nullptr);
        }
        if (skybox.cubemap.image.image_view != VK_NULL_HANDLE) {
            destroy_image(skybox.cubemap.image);
        }

        skybox.cubemap = load_cubemap(path);
        if (skybox.cubemap.image.image != VK_NULL_HANDLE) {
            return true;
        }
        return false;
    }

    optional<vector<shared_ptr<MeshAsset>>> load_gltf_meshes(filesystem::path path)
    {
        fmt::println("loading GLTF: {}", path.string());

        tinygltf::TinyGLTF loader;
        tinygltf::Model model;
        string err, warn;

        bool ok = (path.extension() == ".glb") ? loader.LoadBinaryFromFile(&model, &err, &warn, path.string()) : loader.LoadASCIIFromFile (&model, &err, &warn, path.string());

        if (!warn.empty()) fmt::println("GLTF warning: {}", warn);
        if (!err.empty())  fmt::println("GLTF error: {}",   err);
        if (!ok) {
            fmt::println("failed to load GLTF: {}", path.string());
            return {};
        }

        auto data_ptr = [&](const tinygltf::Accessor& acc) -> const uint8_t* {
            const auto& bv  = model.bufferViews[acc.bufferView];
            const auto& buf = model.buffers[bv.buffer];
            return buf.data.data() + bv.byteOffset + acc.byteOffset;
        };

        auto stride_of = [&](const tinygltf::Accessor& acc) -> size_t {
            const auto& bv = model.bufferViews[acc.bufferView];
            if (bv.byteStride != 0) return bv.byteStride;
            return tinygltf::GetComponentSizeInBytes(acc.componentType) * tinygltf::GetNumComponentsInType(acc.type);
        };

        vector<shared_ptr<MeshAsset>> meshes;

        vector<uint32_t> indices;
        vector<Vertex> vertices;

        for (const tinygltf::Mesh& mesh : model.meshes) {
            MeshAsset new_mesh;
            new_mesh.name = mesh.name;
            indices.clear();
            vertices.clear();

            for (const tinygltf::Primitive& prim : mesh.primitives) {
                GeoSurface new_surface;
                new_surface.start_index = static_cast<uint32_t>(indices.size());
                const size_t initial_vtx = vertices.size();

                // INDEX BUFFER
                {
                    const tinygltf::Accessor& acc = model.accessors[prim.indices];
                    new_surface.count = static_cast<uint32_t>(acc.count);
                    indices.reserve(indices.size() + acc.count);
                    const uint8_t* idx_data = data_ptr(acc);

                    switch (acc.componentType) {
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                            for (size_t i = 0; i < acc.count; i++) indices.push_back(idx_data[i] + (uint32_t)initial_vtx);
                            break;
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                            for (size_t i = 0; i < acc.count; i++) indices.push_back(reinterpret_cast<const uint16_t*>(idx_data)[i] + (uint32_t)initial_vtx);
                            break;
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                            for (size_t i = 0; i < acc.count; i++) indices.push_back(reinterpret_cast<const uint32_t*>(idx_data)[i] + (uint32_t)initial_vtx);
                            break;
                        default:
                            fmt::println("GLTF: unsupported index component type {}", acc.componentType);
                            return {};
                    }
                }

                // POSITION
                {
                    auto it = prim.attributes.find("POSITION");
                    if (it == prim.attributes.end()) {
                        fmt::println("GLTF: primitive missing POSITION attribute");
                        return {};
                    }
                    const tinygltf::Accessor& acc = model.accessors[it->second];
                    const size_t s = stride_of(acc);
                    const uint8_t* ptr = data_ptr(acc);
                    vertices.resize(initial_vtx + acc.count);

                    for (size_t i = 0; i < acc.count; i++) {
                        Vertex vtx{};
                        vtx.position = *reinterpret_cast<const glm::vec3*>(ptr + i * s);
                        vtx.normal = {1, 0, 0};
                        vtx.uv_x = 0;
                        vtx.uv_y = 0;
                        vertices[initial_vtx + i] = vtx;
                    }
                }

                // NORMAL
                {
                    auto it = prim.attributes.find("NORMAL");
                    if (it != prim.attributes.end()) {
                        const tinygltf::Accessor& acc = model.accessors[it->second];
                        const size_t s = stride_of(acc);
                        const uint8_t* ptr = data_ptr(acc);
                        for (size_t i = 0; i < acc.count; i++) vertices[initial_vtx + i].normal = *reinterpret_cast<const glm::vec3*>(ptr + i * s);
                    }
                }

                // TEXCOORD_0
                {
                    auto it = prim.attributes.find("TEXCOORD_0");
                    if (it != prim.attributes.end()) {
                        const tinygltf::Accessor& acc = model.accessors[it->second];
                        const size_t s = stride_of(acc);
                        const uint8_t* ptr = data_ptr(acc);
                        for (size_t i = 0; i < acc.count; i++) {
                            const glm::vec2 uv = *reinterpret_cast<const glm::vec2*>(ptr + i * s);
                            vertices[initial_vtx + i].uv_x = uv.x;
                            vertices[initial_vtx + i].uv_y = uv.y;
                        }
                    }
                }

                new_mesh.surfaces.push_back(new_surface);
            }

            calculate_AABB(new_mesh.AABB_min, new_mesh.AABB_max, vertices);

            new_mesh.mesh_buffers = upload_mesh(indices, vertices);
            meshes.emplace_back(make_shared<MeshAsset>(move(new_mesh)));
        }

        return meshes;
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
        init_descriptors();
        init_pipelines();
        init_default_data();
        init_skybox();
        init_imgui();
        init_composite();
        init_2d();
        init_msaa();
        main_post_processing_stack.init();
    }

    void draw() {
        VK_CHECK(vkWaitForFences(_vk_device, 1, &get_current_frame()._render_fence, true, 1000000000));
        get_current_frame()._deletion_queue.flush();
        get_current_frame()._frame_descriptors.clear_pools(_vk_device);
        VK_CHECK(vkResetFences(_vk_device, 1, &get_current_frame()._render_fence));

        uint32_t swapchain_image_index;
        VK_CHECK(vkAcquireNextImageKHR(_vk_device, _swapchain, 1000000000, get_current_frame()._swapchain_semaphore, nullptr, &swapchain_image_index));
        VkCommandBuffer cmd = get_current_frame()._mainCommandBuffer;
        VK_CHECK(vkResetCommandBuffer(cmd, 0));

        _draw_extent.width = _draw_image.extent.width;
        _draw_extent.height = _draw_image.extent.height;

        VkCommandBufferBeginInfo cmd_begin_info = create_command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        VK_CHECK(vkBeginCommandBuffer(cmd, &cmd_begin_info));

        transition_image(cmd, _draw_image_msaa.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        transition_image(cmd, _skybox_draw_image_msaa.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        transition_image(cmd, _depth_image_msaa.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

        draw_skybox_pass(cmd);
        draw_geometry(cmd);

        // msaa
        transition_image(cmd, _draw_image_msaa.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        transition_image(cmd, _skybox_draw_image_msaa.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        transition_image(cmd, _draw_image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        transition_image(cmd, _skybox_draw_image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        VkImageResolve resolve_region = msaa_resolve();
        resolve_region.extent = {_draw_extent.width, _draw_extent.height, 1};

        vkCmdResolveImage(cmd, _draw_image_msaa.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, _draw_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &resolve_region);
        vkCmdResolveImage(cmd, _skybox_draw_image_msaa.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, _skybox_draw_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &resolve_region);

        transition_image(cmd, _draw_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        transition_image(cmd, _skybox_draw_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        transition_image(cmd, _composite_image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        draw_composite_pass(cmd);

        render_queue.clear();

        // blit to swapchain
        transition_image(cmd, _composite_image.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        transition_image(cmd, _swapchain_images[swapchain_image_index], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        copy_image_to_image(cmd, _composite_image.image, _swapchain_images[swapchain_image_index], _draw_extent, _swapchain_extent);
        transition_image(cmd, _swapchain_images[swapchain_image_index], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        draw_imgui(cmd, _swapchain_image_views[swapchain_image_index]);

        transition_image(cmd, _swapchain_images[swapchain_image_index], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

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
        vkDeviceWaitIdle(_vk_device);

        for (int i = 0; i < FRAME_OVERLAP; i++) {
            _frames[i]._deletion_queue.flush();
            _frames[i]._frame_descriptors.destroy_pools(_vk_device);
        }

        destroy_swapchain();

        for (int i = 0; i < FRAME_OVERLAP; i++) {
            vkDestroyCommandPool(_vk_device, _frames[i]._commandPool, nullptr);

            vkDestroyFence(_vk_device, _frames[i]._render_fence, nullptr);
            vkDestroySemaphore(_vk_device, _frames[i]._render_semaphore, nullptr);
            vkDestroySemaphore(_vk_device, _frames[i]._swapchain_semaphore, nullptr);
        }

        _main_deletion_queue.flush();

        vkDestroySurfaceKHR(_vk_instance, _vk_surface, nullptr);
        vkDestroyDevice(_vk_device, nullptr);

        vkb::destroy_debug_utils_messenger(_vk_instance, _debug_messenger);
        vkDestroyInstance(_vk_instance, nullptr);

        SDL_DestroyWindow(window);
        SDL_Quit();
    }
}

#endif