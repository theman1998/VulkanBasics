#include "graphic/VertexHelpers.h"
#include <exception>
#include <stdexcept>


namespace V
{


    VkVertexInputBindingDescription Vertex::getBindingDescription()
{
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 3> Vertex::getAttributeDescriptions()
{
    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0; // Look into our sharder.vert file. 
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT; // first input is 2d
    attributeDescriptions[0].offset = offsetof(Vertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; // second input is 3d
    attributeDescriptions[1].offset = offsetof(Vertex, color);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

    return attributeDescriptions;
}




// helper function to create a shader object.
VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data()); // It takes a 4bytes as the arg

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }

    return shaderModule;
}


// creates the vertex stage
VkPipelineShaderStageCreateInfo createShaderStageInfoVertex(VkShaderModule shaderModule)
{
    // Shader stage creation within the graphics pipeline
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO; //used in every vk struct
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT; //enum value represents all the types of stages
    vertShaderStageInfo.module = shaderModule; // created from our helper function
    vertShaderStageInfo.pName = "main"; //used for label when working with multiple of shaders
    vertShaderStageInfo.pSpecializationInfo = nullptr; // not required but can be used for shader constants and will reduce if statements because compile will make optimization

    return vertShaderStageInfo;
}

// Creates the fragment stage
// Currently static, but end goal is to make this dynamic so we can load more than one image
VkPipelineShaderStageCreateInfo createShaderStageInfoFragment(VkShaderModule shaderModule)
{
    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = shaderModule;
    fragShaderStageInfo.pName = "main";

    return fragShaderStageInfo;
}



uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    // Searching for the first bit that matches in input type filter
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}


// This struct describes the geometry that will be drawn from the vertices and if primitive restart should be enabled.
// Read more on this from https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions under "Input assembly" section
VkPipelineInputAssemblyStateCreateInfo createBasicAssemblyInfo()
{
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
    return inputAssembly;
}

// Create The 3 VK structures that is responisble for the view state of the graphics. This includes the viewport, scissors, and viewport state.
// Structure returns a shared pointer because the viewportState requires a reference to the viewport that is generated from this stack and
// it will get wipe out once it leaves this function. Hence we need to put it on the heap. We do not want to worry about that so RAII for the win
std::shared_ptr<ViewportContainer> createBasicViewportState(VkExtent2D* swapChainExtent)
{
    std::shared_ptr<ViewportContainer> c(new ViewportContainer);
    //Viewport and scissors
    // Viewport describes the region of the framebuffer that the output will rendered to. Will almost always be (0,0) to (width,height)
    // Need to use the swapchain image width and height, not the windows.
    // Scissor rectangles define the region pixels will actually be stored. any pixel outside the area will be discarded by the rasterizer. act as a filter
    VkViewport &viewport = c->viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapChainExtent->width;
    viewport.height = (float)swapChainExtent->height;
    viewport.minDepth = 0.0f; // don't need to mess with these values
    viewport.maxDepth = 1.0f; // don't need to mess with these values
    VkRect2D &scissor = c->scissor;
    scissor.offset = { 0, 0 };
    scissor.extent = *swapChainExtent;

    // It's possible to have multiple scissors active at one time, if the gpu has it as a feature
    VkPipelineViewportStateCreateInfo &viewportState = c->viewportState;
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    return c;
}


// Rasterizer
// Takes geometry that is shaped by the vertices from the vertex shader and turns it into graments to be colored by the fragment shader
// It also performs depth testing, face culling, and the scissor test.
// It can be configured to output entire polygons or edges(wireframe rendering).
VkPipelineRasterizationStateCreateInfo createBasicRasterizeInfo()
{
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE; // if true, will not discard fragemnts outside the planes but rather clamp them ( not sure what clamp means)
    rasterizer.rasterizerDiscardEnable = VK_FALSE; // if true, disable geometry. basically disables output from the framebuffer.
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL; // determines how fragments are generated. Anymode that isn't this one requires enabling a GPU feature
    rasterizer.lineWidth = 1.0f; // Thickness of lines in terms of fragments. Higher then one requires enabling wideLines in the GPU features
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT; // determines the type of face culling to use. 
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE; // vertex order for faces
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f; // Optional
    rasterizer.depthBiasClamp = 0.0f; // Optional
    rasterizer.depthBiasSlopeFactor = 0.0f; // Optional
    return rasterizer;
}


// one way to perform anti-aliasing when combining the fragment shader and the rasterizer fragment polygon output.
// Typical work will occur along edges. GPU requires the feature if turned on
VkPipelineMultisampleStateCreateInfo createBasicMultisamplingInfo()
{
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f; // Optional
    multisampling.pSampleMask = nullptr; // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE; // Optional

    return multisampling;
}

// Color blending
// After fragment shader returns the color, it needs to be combine with the color that is already in the framebuffer.
// This is known as color blending. 2 ways
// 1) Mixed the old and new value to produce a final color
// 2) combine the old and new value using a bitwise operation
// More research should be done in color blending. Different implemenation can have impact on the color
std::shared_ptr < ColorBlendContainer > createBasicColorBlendState()
{
    ColorBlendContainer * container= new ColorBlendContainer;
    VkPipelineColorBlendAttachmentState &colorBlendAttachment = container->attachment;
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

    VkPipelineColorBlendStateCreateInfo & colorBlending = container->info;
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional

    return std::shared_ptr < ColorBlendContainer >(container);
}

// pipeline layout
// represented in the global space
// Used to updated shaders without having to recreate them. Commonly used to pass the transformation matrix to the vertex shader, or to create textyure samplers in the fragment shader
// Don't need to use them but required to fill it out.
VkPipelineLayoutCreateInfo createBasicPipelineLayoutInfo()
{
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0; // Optional
    pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
    pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional
    return pipelineLayoutInfo;
}




}