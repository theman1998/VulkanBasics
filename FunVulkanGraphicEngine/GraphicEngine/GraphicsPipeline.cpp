#include "GraphicEngine/GraphicsPipeline.hpp"
#include "GraphicEngine/GraphicsQueue.hpp"
#include "GraphicEngine/Utility/DeviceSupport.hpp"

#include <filesystem>

namespace
{
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


	VkPipelineShaderStageCreateInfo createPipelineShaderInfo(VkDevice vkDevice, std::string_view name, GE::ShaderType type, VkShaderModule module)
	{
		if (type == GE::ShaderType::Undefined) { throw std::runtime_error("ShaderType is undefine when creating shader info."); }

		VkPipelineShaderStageCreateInfo shaderStageInfo{};
		shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO; //used in every vk struct

		switch (type)
		{
		case GE::ShaderType::Vertex:
		{
			shaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT; //enum value represents all the types of stages
		}break;
		case GE::ShaderType::Fragment:
		{
			shaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		}break;
		}
		shaderStageInfo.module = module; // created from our helper function
		shaderStageInfo.pName = name.data(); //used for label when working with multiple of shaders
		shaderStageInfo.pSpecializationInfo = nullptr; // not required but can be used for shader constants and will reduce if statements because compile will make optimization

		return shaderStageInfo;
	}

	
}


namespace GE
{


	GraphicPipeline::GraphicPipeline(VkDevice device) : internals(), device(device), shaderLoadInfoList()
	{}
	GraphicPipeline::~GraphicPipeline() = default;
	void GraphicPipeline::Free()
	{
		if (device == nullptr)return;

		if (internals.graphicsPipeline != nullptr)vkDestroyPipeline(device, internals.graphicsPipeline, nullptr);
		if (internals.pipelineLayout != nullptr)vkDestroyPipelineLayout(device, internals.pipelineLayout, nullptr);
		if (internals.descriptorSetLayout != nullptr)vkDestroyDescriptorSetLayout(device, internals.descriptorSetLayout, nullptr);
		if (internals.renderPass != nullptr)vkDestroyRenderPass(device, internals.renderPass, nullptr);
		if (internals.commandPool != nullptr) { vkDestroyCommandPool(device, internals.commandPool, nullptr); }

	}
	const ErrorMessage& GraphicPipeline::getError() const { return currentError; }

	const GraphicPipelineInternals& GraphicPipeline::Internals() const { return internals; }
	GraphicPipelineInternals& GraphicPipeline::Internals() { return internals; }

	bool GraphicPipeline::initDevice(VkDevice device)
	{
		this->device = device;
		return true;
	}
	bool GraphicPipeline::initCommandInfo(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
	{
		if (device == nullptr)
		{
			currentError = "Device is null while creating command info";
			return false;
		}
		else if (physicalDevice == nullptr)
		{
			currentError = "physicalDevice is null while creating command info";
			return false;
		}
		else if (surface == nullptr)
		{
			currentError = "surface is null while creating command info";
			return false;
		}

		Util::QueueFamilyIndices indices = Util::findQueueFamilies(physicalDevice, const_cast<VkSurfaceKHR&>(surface));

		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Allow the buffer to be rerecoreded individually. Other commands can be found here
		// https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Command_buffers
		poolInfo.queueFamilyIndex = *indices.graphicsFamily;

		if (vkCreateCommandPool(device, &poolInfo, nullptr, &internals.commandPool) != VK_SUCCESS)
		{
			currentError = "failed to create command pool!";
			return false;
		}


		internals.commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = internals.commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // specifies if it's primary or secondary command buffers
		allocInfo.commandBufferCount = (uint32_t)internals.commandBuffers.size();

		if (vkAllocateCommandBuffers(device, &allocInfo, internals.commandBuffers.data()) != VK_SUCCESS)
		{
			currentError = "failed to allocate command buffers!";
			return false;
		}

		return true;
	}

	bool GraphicPipeline::setShaders(const std::vector<ShaderLoadInfo>& shaders)
	{
		for (auto& shader : shaders)
		{
			if (!std::filesystem::exists(shader.fileName))
			{
				currentError = "Shader file " + shader.fileName + " doesn't exist.";
				return false;
			}
		}
		shaderLoadInfoList = shaders;
		return true;
	}

	bool GraphicPipeline::initRenderPass(VkPhysicalDevice physicalDevice, VkFormat swapChainImageFormat, VkSampleCountFlagBits msaaSamples)
	{

		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = swapChainImageFormat; // should match the format of the swap chain
		colorAttachment.samples = msaaSamples;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // what to do with attachment before rendering
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // what to do with attachment after rendering. want to see the image so we store it
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // used in the stencil buffer. we aren't using it currently
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		// more discussion on this in the textering chapter
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // specifies which layout the image wil have have before rendering
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // specifies the transition to when render pass finishes, Multi-Sample Pixel can't be represented directly, so this flag helps

		// Subpasses and attechment references
		// Used to reduce memory bandwidth and increase performance. Imagine filters on a snapchat pic
		// Will only use 1 subpass for the triangle
		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription depthAttachment{};
		depthAttachment.format = Util::findDepthFormat(physicalDevice);
		depthAttachment.samples = msaaSamples;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		VkAttachmentReference depthAttachmentRef{};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


		VkAttachmentDescription colorAttachmentResolve{};
		colorAttachmentResolve.format = swapChainImageFormat;
		colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		VkAttachmentReference colorAttachmentResolveRef{};
		colorAttachmentResolveRef.attachment = 2;
		colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;
		subpass.pResolveAttachments = &colorAttachmentResolveRef;

		std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };


		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;



		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &internals.renderPass) != VK_SUCCESS) {
			currentError = "failed to create render pass!";
			return false;
		}



		VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1; // Can be represented as an array. allowing for skeleton movement

		// The type of stages that will be used in this stage. Can OR operation each bit
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		// Used with image sampling descriptors
		uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

		VkDescriptorSetLayoutBinding samplerLayoutBinding{};
		samplerLayoutBinding.binding = 1;
		samplerLayoutBinding.descriptorCount = 1;
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBinding.pImmutableSamplers = nullptr;
		// Typically done in the frag shader. But if dynamically deform a grid of vertices by a heightmap, than we should use the vertex shader
		samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();


		// Telling vulkan which shader we will be using
		if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &internals.descriptorSetLayout) != VK_SUCCESS) {
			currentError = "failed to create descriptor set layout!";
			return false;
		}



		return true;
	}

	bool GraphicPipeline::initPipeline(VkExtent2D swapchainExtent, VkSampleCountFlagBits msaaSamples)
	{
		std::vector<VkShaderModule> shaderModulesObjs;// used for cleanup
		std::vector<VkPipelineShaderStageCreateInfo> shaderStagesObjs;

		for (auto& shaderStage : shaderLoadInfoList)
		{
			std::vector<char> shaderCode = Util::readFile(shaderStage.fileName);
			VkShaderModule shaderModule = createShaderModule(device, shaderCode);
			shaderModulesObjs.push_back(shaderModule);
			shaderStagesObjs.push_back(createPipelineShaderInfo(device, shaderStage.name, shaderStage.type, shaderModule));
		}


		VkVertexInputBindingDescription bindingDescription = Vertex::getBindingDescription();
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = Vertex::getAttributeDescriptions();


		// This struct describes the format that will be pass to the vertex shader in 2 ways
		// * bindings: space between data. per-vertex or per-instance
		// * atrribute descriptions: type of attriibutes passed to the vertex shader
		// Currently here for filler. Tutorial will go over this in the "vertex buffers" chapter
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription; // meta info for vertex buffering
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());;
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data(); // Meta info for Vertex buffering

		// This struct describes the geometry that will be drawn from the vertices and if primitive restart should be enabled.
		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)swapchainExtent.width;
		viewport.height = (float)swapchainExtent.height;
		viewport.minDepth = 0.0f; // don't need to mess with these values
		viewport.maxDepth = 1.0f; // don't need to mess with these values
		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = swapchainExtent;
		//It's possible to have multiple scissors active at one time, if the gpu has it as a feature
		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		// Takes geometry that is shaped by the vertices from the vertex shader and turns it into graments to be colored by the fragment shader
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
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; // due to the flip we are doing within the y position in the matrixs

		// one way to perform anti-aliasing when combining the fragment shader and the rasterizer fragment polygon output.
		// Typical work will occur along edges. GPU requires the feature if turned on
		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.minSampleShading = 1.0f; // Optional
		multisampling.pSampleMask = nullptr; // Optional
		multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
		multisampling.alphaToOneEnable = VK_FALSE; // Optional
		multisampling.rasterizationSamples = msaaSamples;

		// Depth and stencil testing
		// struct is VkPipelineDepthStencilStateCreateInfo
		VkPipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_TRUE;// If depth fragment passes, should it be written to buffer
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS; // Lower depth  = closer
		depthStencil.depthBoundsTestEnable = VK_FALSE; // If true, only allows frag that fall in the range
		depthStencil.minDepthBounds = 0.0f; // Optional
		depthStencil.maxDepthBounds = 1.0f; // Optional
		depthStencil.stencilTestEnable = VK_FALSE;
		depthStencil.front = {}; // Optional
		depthStencil.back = {}; // Optional


		// Color blending
		// After fragment shader returns the color, it needs to be combine with the color that is already in the framebuffer.
		// This is known as color blending. 2 ways
		// 1) Mixed the old and new value to produce a final color
		// 2) combine the old and new value using a bitwise operation
		// More research should be done in color blending. Different implemenation can have impact on the color
		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional
		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f; // Optional
		colorBlending.blendConstants[1] = 0.0f; // Optional
		colorBlending.blendConstants[2] = 0.0f; // Optional
		colorBlending.blendConstants[3] = 0.0f; // Optional


		// Dynamic state
		// limited amount of the states created above can't be changes without recreating the pipeline
		// Not sure if this is required as it is used to change some features during run time
		std::vector<VkDynamicState> dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_LINE_WIDTH
		};
		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();


		// represented in the global space
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0; // Optional
		pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
		pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
		pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &internals.descriptorSetLayout;
		if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &internals.pipelineLayout) != VK_SUCCESS) {
			currentError = "failed to create pipeline layout!";
			return false;
		}



		// Finaly. We can create the Graphic Pipeline !!!!!
		VkGraphicsPipelineCreateInfo pipelineInfo_{};
		pipelineInfo_.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo_.stageCount = (uint32_t)shaderStagesObjs.size();
		pipelineInfo_.pStages = shaderStagesObjs.data();
		pipelineInfo_.pVertexInputState = &vertexInputInfo;
		pipelineInfo_.pInputAssemblyState = &inputAssembly;
		pipelineInfo_.pViewportState = &viewportState;
		pipelineInfo_.pRasterizationState = &rasterizer;
		pipelineInfo_.pMultisampleState = &multisampling;
		pipelineInfo_.pColorBlendState = &colorBlending;
		pipelineInfo_.pDynamicState = nullptr; // Optional
		pipelineInfo_.layout = internals.pipelineLayout; // fixed function stage
		pipelineInfo_.renderPass = internals.renderPass;
		pipelineInfo_.subpass = 0; // The index of the subpass
		// The following 2 will derive from an existing pipeline. save time.
		pipelineInfo_.basePipelineHandle = VK_NULL_HANDLE; // Optional
		pipelineInfo_.basePipelineIndex = -1; // Optional
		pipelineInfo_.pDepthStencilState = &depthStencil;


		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo_, nullptr, &internals.graphicsPipeline) != VK_SUCCESS) {
			currentError = "failed to create graphics pipeline!";
			return false;
		}


		for (auto mod : shaderModulesObjs)
		{
			vkDestroyShaderModule(device, mod, nullptr);
		}
		return true;
	}

	bool GraphicPipeline::updateViewPort(uint32_t currentFrame, VkExtent2D swapchainExtent)
	{


		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)swapchainExtent.width;
		viewport.height = (float)swapchainExtent.height;
		viewport.minDepth = 0.0f; // don't need to mess with these values
		viewport.maxDepth = 1.0f; // don't need to mess with these values


		//vkCmdSetViewport(internals.commandBuffers[currentFrame], 0, 1, &viewport);


		return true;
	}

	bool GraphicPipeline::startPipelinePass(uint32_t currentFrame, VkExtent2D swapchainExtent, VkFramebuffer swapchainFrameBuffer, const VkClearColorValue& backgroundColor)
	{
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0; // Optional
		beginInfo.pInheritanceInfo = nullptr; // Optional. Used for secondary command buffers

		// If the command buffer was already recorded once, then vkBeginCommandBuffer will implicity reset it. 
		if (vkBeginCommandBuffer(internals.commandBuffers[currentFrame], &beginInfo) != VK_SUCCESS) {
			currentError = "failed to begin recording command buffer!";
			return false;
		}
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = internals.renderPass;
		renderPassInfo.framebuffer = swapchainFrameBuffer; // create a framebuffer for each swap chain image where it is specified as a color attachment

		renderPassInfo.renderArea.offset = { 0, 0 }; // Size for render area. Starting position
		renderPassInfo.renderArea.extent = swapchainExtent; // size


		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = backgroundColor;// Black with 100% opacity
		clearValues[1].depthStencil = { 1.0f, 0 }; // 1 is far plane, 0 is near plane

		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());;
		renderPassInfo.pClearValues = clearValues.data();

		// Render pass can now begin. All function that record commands can be recongnized by their vkCmd
		vkCmdBeginRenderPass(internals.commandBuffers[currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		return true;
	}

	bool GraphicPipeline::bindAndDrawIndexVertices(uint32_t currentFrame, VkBuffer indexBuffer, uint32_t indicesSize, VkBuffer verticesBuffer, VkDescriptorSet descriptorSet)
	{
		VkCommandBuffer commandBuffer = internals.commandBuffers[currentFrame];
		// 2nd arg: pipline object is either graphics or compute pipeline. 
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, internals.graphicsPipeline);
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		// This is what is what uploading the input to the shader of the program
		VkBuffer vertexBuffers[] = { verticesBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

		// Binding our descriptor sets to the frame. Specifing that its for the graphics over the compute pipeline. 
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, internals.pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

		vkCmdDrawIndexed(commandBuffer, indicesSize, 1, 0, 0, 0);

		return true;
	}

	bool GraphicPipeline::completePipelinePass(uint32_t currentFrame)
	{
		vkCmdEndRenderPass(internals.commandBuffers[currentFrame]);
		if (vkEndCommandBuffer(internals.commandBuffers[currentFrame]) != VK_SUCCESS) {
			currentError = "failed to record command buffer!";
			return false;
		}
		return true;
	}

}