#include "predictor_spv.h"
#include "tiff_vulkan.h"
#include "tiffiop.h"
#include "vulkan_shader_spv.h"

#ifdef HAVE_VULKAN
#include <string.h>
#include <vulkan/vulkan.h>

#define PACK(r, g, b)                                                          \
    ((uint32_t)(r) | ((uint32_t)(g) << 8) | ((uint32_t)(b) << 16) | 0xff000000U)

int tiff_use_vulkan = 0;
static VkInstance tiff_vk_instance = VK_NULL_HANDLE;
static VkDevice tiff_vk_device = VK_NULL_HANDLE;
static VkQueue tiff_vk_queue = VK_NULL_HANDLE;
static VkPhysicalDevice tiff_vk_phys = VK_NULL_HANDLE;
static uint32_t tiff_vk_queue_family = 0;
static VkCommandPool tiff_vk_pool = VK_NULL_HANDLE;
static VkPipeline tiff_vk_pipeline = VK_NULL_HANDLE;
static VkPipeline tiff_vk_pred_pipeline = VK_NULL_HANDLE;
static VkPipelineLayout tiff_vk_pipeline_layout = VK_NULL_HANDLE;
static VkDescriptorSetLayout tiff_vk_dsl = VK_NULL_HANDLE;
static VkDescriptorPool tiff_vk_desc_pool = VK_NULL_HANDLE;
static VkShaderModule tiff_vk_shader = VK_NULL_HANDLE;
static VkShaderModule tiff_vk_pred_shader = VK_NULL_HANDLE;

static TIFFYCbCrToRGB *get_ycbcr_table(void)
{
    static TIFFYCbCrToRGB table;
    static int initialized = 0;
    if (!initialized)
    {
        float luma[3] = {0.2126f, 0.7152f, 0.0722f};
        float refbw[6] = {0.0f, 255.0f, 128.0f, 255.0f, 128.0f, 255.0f};
        TIFFYCbCrToRGBInit(&table, luma, refbw);
        initialized = 1;
    }
    return &table;
}

static uint32_t find_mem_type(uint32_t type_bits, VkMemoryPropertyFlags props)
{
    VkPhysicalDeviceMemoryProperties mem_props;
    vkGetPhysicalDeviceMemoryProperties(tiff_vk_phys, &mem_props);
    for (uint32_t i = 0; i < mem_props.memoryTypeCount; i++)
    {
        if ((type_bits & (1u << i)) &&
            (mem_props.memoryTypes[i].propertyFlags & props) == props)
            return i;
    }
    return 0;
}

static int create_buffer(VkDeviceSize size, VkBufferUsageFlags usage,
                         VkBuffer *buf, VkDeviceMemory *mem)
{
    VkBufferCreateInfo bi = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bi.size = size;
    bi.usage = usage;
    bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(tiff_vk_device, &bi, NULL, buf) != VK_SUCCESS)
        return 0;

    VkMemoryRequirements req;
    vkGetBufferMemoryRequirements(tiff_vk_device, *buf, &req);
    VkMemoryAllocateInfo ai = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    ai.allocationSize = req.size;
    ai.memoryTypeIndex = find_mem_type(
        req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (vkAllocateMemory(tiff_vk_device, &ai, NULL, mem) != VK_SUCCESS)
    {
        vkDestroyBuffer(tiff_vk_device, *buf, NULL);
        return 0;
    }
    vkBindBufferMemory(tiff_vk_device, *buf, *mem, 0);
    return 1;
}

int TIFFUseVulkan(void) { return tiff_use_vulkan; }

void TIFFSetUseVulkan(int enable) { tiff_use_vulkan = enable; }

void TIFFInitVulkan(void)
{
    if (tiff_use_vulkan)
        return;

    VkApplicationInfo app = {.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                             .pApplicationName = "libtiff",
                             .apiVersion = VK_API_VERSION_1_0};
    VkInstanceCreateInfo inst_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app};
    if (vkCreateInstance(&inst_info, NULL, &tiff_vk_instance) != VK_SUCCESS)
        return;

    uint32_t dev_count = 0;
    if (vkEnumeratePhysicalDevices(tiff_vk_instance, &dev_count, NULL) !=
            VK_SUCCESS ||
        dev_count == 0)
    {
        vkDestroyInstance(tiff_vk_instance, NULL);
        tiff_vk_instance = VK_NULL_HANDLE;
        return;
    }
    VkPhysicalDevice devices[1];
    vkEnumeratePhysicalDevices(tiff_vk_instance, &dev_count, devices);
    tiff_vk_phys = devices[0];

    uint32_t qf_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(tiff_vk_phys, &qf_count, NULL);
    VkQueueFamilyProperties props[qf_count];
    vkGetPhysicalDeviceQueueFamilyProperties(tiff_vk_phys, &qf_count, props);
    for (uint32_t i = 0; i < qf_count; i++)
    {
        if (props[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
        {
            tiff_vk_queue_family = i;
            break;
        }
    }

    float queue_prio = 1.0f;
    VkDeviceQueueCreateInfo qinfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = tiff_vk_queue_family,
        .queueCount = 1,
        .pQueuePriorities = &queue_prio};
    VkDeviceCreateInfo dinfo = {.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                                .queueCreateInfoCount = 1,
                                .pQueueCreateInfos = &qinfo};
    if (vkCreateDevice(tiff_vk_phys, &dinfo, NULL, &tiff_vk_device) !=
        VK_SUCCESS)
    {
        vkDestroyInstance(tiff_vk_instance, NULL);
        tiff_vk_instance = VK_NULL_HANDLE;
        return;
    }
    vkGetDeviceQueue(tiff_vk_device, tiff_vk_queue_family, 0, &tiff_vk_queue);

    VkCommandPoolCreateInfo pool_info = {
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pool_info.queueFamilyIndex = tiff_vk_queue_family;
    vkCreateCommandPool(tiff_vk_device, &pool_info, NULL, &tiff_vk_pool);

    VkDescriptorSetLayoutBinding bindings[2] = {
        {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT,
         NULL},
        {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT,
         NULL}};
    VkDescriptorSetLayoutCreateInfo dsl_info = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    dsl_info.bindingCount = 2;
    dsl_info.pBindings = bindings;
    vkCreateDescriptorSetLayout(tiff_vk_device, &dsl_info, NULL, &tiff_vk_dsl);

    VkPushConstantRange pcr = {VK_SHADER_STAGE_COMPUTE_BIT, 0,
                               sizeof(uint32_t)};
    VkPipelineLayoutCreateInfo pli = {
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pli.setLayoutCount = 1;
    pli.pSetLayouts = &tiff_vk_dsl;
    pli.pushConstantRangeCount = 1;
    pli.pPushConstantRanges = &pcr;
    vkCreatePipelineLayout(tiff_vk_device, &pli, NULL,
                           &tiff_vk_pipeline_layout);

    VkShaderModuleCreateInfo smci = {
        VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    smci.codeSize = ycbcr_to_rgba_spv_len;
    smci.pCode = (const uint32_t *)ycbcr_to_rgba_spv;
    vkCreateShaderModule(tiff_vk_device, &smci, NULL, &tiff_vk_shader);

    smci.codeSize = predictor_spv_len;
    smci.pCode = (const uint32_t *)predictor_spv;
    vkCreateShaderModule(tiff_vk_device, &smci, NULL, &tiff_vk_pred_shader);

    VkPipelineShaderStageCreateInfo stage = {
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stage.module = tiff_vk_shader;
    stage.pName = "main";
    VkComputePipelineCreateInfo cpi = {
        VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
    cpi.stage = stage;
    cpi.layout = tiff_vk_pipeline_layout;
    vkCreateComputePipelines(tiff_vk_device, VK_NULL_HANDLE, 1, &cpi, NULL,
                             &tiff_vk_pipeline);

    stage.module = tiff_vk_pred_shader;
    cpi.stage = stage;
    vkCreateComputePipelines(tiff_vk_device, VK_NULL_HANDLE, 1, &cpi, NULL,
                             &tiff_vk_pred_pipeline);

    VkDescriptorPoolSize ps = {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2};
    VkDescriptorPoolCreateInfo dpi = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    dpi.maxSets = 1;
    dpi.poolSizeCount = 1;
    dpi.pPoolSizes = &ps;
    vkCreateDescriptorPool(tiff_vk_device, &dpi, NULL, &tiff_vk_desc_pool);

    tiff_use_vulkan = 1;
}

void TIFFCleanupVulkan(void)
{
    if (tiff_vk_device)
    {
        vkDeviceWaitIdle(tiff_vk_device);
        if (tiff_vk_pipeline != VK_NULL_HANDLE)
            vkDestroyPipeline(tiff_vk_device, tiff_vk_pipeline, NULL);
        if (tiff_vk_pred_pipeline != VK_NULL_HANDLE)
            vkDestroyPipeline(tiff_vk_device, tiff_vk_pred_pipeline, NULL);
        if (tiff_vk_shader != VK_NULL_HANDLE)
            vkDestroyShaderModule(tiff_vk_device, tiff_vk_shader, NULL);
        if (tiff_vk_pred_shader != VK_NULL_HANDLE)
            vkDestroyShaderModule(tiff_vk_device, tiff_vk_pred_shader, NULL);
        if (tiff_vk_pipeline_layout != VK_NULL_HANDLE)
            vkDestroyPipelineLayout(tiff_vk_device, tiff_vk_pipeline_layout,
                                    NULL);
        if (tiff_vk_dsl != VK_NULL_HANDLE)
            vkDestroyDescriptorSetLayout(tiff_vk_device, tiff_vk_dsl, NULL);
        if (tiff_vk_desc_pool != VK_NULL_HANDLE)
            vkDestroyDescriptorPool(tiff_vk_device, tiff_vk_desc_pool, NULL);
        if (tiff_vk_pool != VK_NULL_HANDLE)
            vkDestroyCommandPool(tiff_vk_device, tiff_vk_pool, NULL);
        vkDestroyDevice(tiff_vk_device, NULL);
        tiff_vk_device = VK_NULL_HANDLE;
    }
    if (tiff_vk_instance)
    {
        vkDestroyInstance(tiff_vk_instance, NULL);
        tiff_vk_instance = VK_NULL_HANDLE;
    }
    tiff_use_vulkan = 0;
}

void TIFFYCbCr8ToRGBAVulkan(const unsigned char *src, uint32_t *dst,
                            uint32_t pixel_count)
{
    if (!tiff_use_vulkan || pixel_count == 0)
    {
        uint32_t r, g, b;
        for (uint32_t i = 0; i < pixel_count; i++)
        {
            TIFFYCbCrtoRGB(get_ycbcr_table(), src[0], src[1], src[2], &r, &g,
                           &b);
            dst[i] = PACK(r, g, b);
            src += 3;
        }
        return;
    }
    VkDeviceSize in_size = pixel_count * 3;
    VkDeviceSize out_size = pixel_count * 4;
    VkBuffer buf_in, buf_out;
    VkDeviceMemory mem_in, mem_out;
    if (!create_buffer(in_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, &buf_in,
                       &mem_in) ||
        !create_buffer(out_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, &buf_out,
                       &mem_out))
    {
        TIFFErrorExtR(NULL, "tiff_vulkan", "buffer allocation failed");
        return;
    }
    void *map;
    vkMapMemory(tiff_vk_device, mem_in, 0, in_size, 0, &map);
    memcpy(map, src, (size_t)in_size);
    vkUnmapMemory(tiff_vk_device, mem_in);

    VkDescriptorSetAllocateInfo dsai = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    dsai.descriptorPool = tiff_vk_desc_pool;
    dsai.descriptorSetCount = 1;
    dsai.pSetLayouts = &tiff_vk_dsl;
    VkDescriptorSet set;
    vkAllocateDescriptorSets(tiff_vk_device, &dsai, &set);

    VkDescriptorBufferInfo in_info = {buf_in, 0, in_size};
    VkDescriptorBufferInfo out_info = {buf_out, 0, out_size};
    VkWriteDescriptorSet writes[2] = {
        {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, NULL, set, 0, 0, 1,
         VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, NULL, &in_info, NULL},
        {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, NULL, set, 1, 0, 1,
         VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, NULL, &out_info, NULL}};
    vkUpdateDescriptorSets(tiff_vk_device, 2, writes, 0, NULL);

    VkCommandBufferAllocateInfo cbai = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    cbai.commandPool = tiff_vk_pool;
    cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbai.commandBufferCount = 1;
    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(tiff_vk_device, &cbai, &cmd);

    VkCommandBufferBeginInfo begin = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    vkBeginCommandBuffer(cmd, &begin);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, tiff_vk_pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
                            tiff_vk_pipeline_layout, 0, 1, &set, 0, NULL);
    uint32_t groups = (pixel_count + 255) / 256;
    vkCmdDispatch(cmd, groups, 1, 1);
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submit = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    vkQueueSubmit(tiff_vk_queue, 1, &submit, VK_NULL_HANDLE);
    vkQueueWaitIdle(tiff_vk_queue);

    vkMapMemory(tiff_vk_device, mem_out, 0, out_size, 0, &map);
    memcpy(dst, map, (size_t)out_size);
    vkUnmapMemory(tiff_vk_device, mem_out);

    vkFreeCommandBuffers(tiff_vk_device, tiff_vk_pool, 1, &cmd);
    vkDestroyBuffer(tiff_vk_device, buf_in, NULL);
    vkDestroyBuffer(tiff_vk_device, buf_out, NULL);
    vkFreeMemory(tiff_vk_device, mem_in, NULL);
    vkFreeMemory(tiff_vk_device, mem_out, NULL);
}

void TIFFPredictorDiff16Vulkan(const uint16_t *src, uint16_t *dst,
                               uint32_t count, uint32_t stride)
{
    if (!tiff_use_vulkan || count == 0)
    {
        memcpy(dst, src, count * sizeof(uint16_t));
        for (uint32_t i = stride; i < count; i++)
            dst[i] = src[i] - src[i - stride];
        return;
    }

    VkDeviceSize buf_size = count * sizeof(uint16_t);
    VkBuffer buf_in, buf_out;
    VkDeviceMemory mem_in, mem_out;
    if (!create_buffer(buf_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, &buf_in,
                       &mem_in) ||
        !create_buffer(buf_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, &buf_out,
                       &mem_out))
    {
        TIFFErrorExtR(NULL, "tiff_vulkan", "buffer allocation failed");
        return;
    }
    void *map;
    vkMapMemory(tiff_vk_device, mem_in, 0, buf_size, 0, &map);
    memcpy(map, src, (size_t)buf_size);
    vkUnmapMemory(tiff_vk_device, mem_in);

    VkDescriptorSetAllocateInfo dsai = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    dsai.descriptorPool = tiff_vk_desc_pool;
    dsai.descriptorSetCount = 1;
    dsai.pSetLayouts = &tiff_vk_dsl;
    VkDescriptorSet set;
    vkAllocateDescriptorSets(tiff_vk_device, &dsai, &set);

    VkDescriptorBufferInfo in_info = {buf_in, 0, buf_size};
    VkDescriptorBufferInfo out_info = {buf_out, 0, buf_size};
    VkWriteDescriptorSet writes[2] = {
        {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, NULL, set, 0, 0, 1,
         VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, NULL, &in_info, NULL},
        {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, NULL, set, 1, 0, 1,
         VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, NULL, &out_info, NULL}};
    vkUpdateDescriptorSets(tiff_vk_device, 2, writes, 0, NULL);

    VkCommandBufferAllocateInfo cbai = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    cbai.commandPool = tiff_vk_pool;
    cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbai.commandBufferCount = 1;
    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(tiff_vk_device, &cbai, &cmd);

    VkCommandBufferBeginInfo begin = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    vkBeginCommandBuffer(cmd, &begin);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
                      tiff_vk_pred_pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
                            tiff_vk_pipeline_layout, 0, 1, &set, 0, NULL);
    vkCmdPushConstants(cmd, tiff_vk_pipeline_layout,
                       VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(uint32_t),
                       &stride);
    uint32_t groups = (count + 255) / 256;
    vkCmdDispatch(cmd, groups, 1, 1);
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submit = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    vkQueueSubmit(tiff_vk_queue, 1, &submit, VK_NULL_HANDLE);
    vkQueueWaitIdle(tiff_vk_queue);

    vkMapMemory(tiff_vk_device, mem_out, 0, buf_size, 0, &map);
    memcpy(dst, map, (size_t)buf_size);
    vkUnmapMemory(tiff_vk_device, mem_out);

    vkFreeCommandBuffers(tiff_vk_device, tiff_vk_pool, 1, &cmd);
    vkDestroyBuffer(tiff_vk_device, buf_in, NULL);
    vkDestroyBuffer(tiff_vk_device, buf_out, NULL);
    vkFreeMemory(tiff_vk_device, mem_in, NULL);
    vkFreeMemory(tiff_vk_device, mem_out, NULL);
}

#else

void TIFFInitVulkan(void) {}
void TIFFCleanupVulkan(void) {}
void TIFFYCbCr8ToRGBAVulkan(const unsigned char *src, uint32_t *dst,
                            uint32_t pixel_count)
{
    (void)src;
    (void)dst;
    (void)pixel_count;
}

void TIFFPredictorDiff16Vulkan(const uint16_t *src, uint16_t *dst,
                               uint32_t count, uint32_t stride)
{
    memcpy(dst, src, count * sizeof(uint16_t));
    for (uint32_t i = stride; i < count; i++)
        dst[i] = src[i] - src[i - stride];
}

#endif
