#include "GraphicEngine/GraphicsSwapchain.hpp"
#include "GraphicEngine/GraphicsQueue.hpp"

#include "GraphicEngine/Utility/DeviceSupport.hpp"
#include "GraphicEngine/Utility/MemorySupport.hpp"


namespace {

	// formats can contain rgba / svg,  bit pixel size, defines the color space
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		for (const auto& availableFormat : availableFormats) {
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return availableFormat;
			}
		}

		std::cout << "did not find VK_FORMAT_B8G8R8A8_SRGB && VK_COLOR_SPACE_SRGB_NONLINEAR_KHR so we are going with the first index found in the list" << std::endl;
		return availableFormats[0];
	}



	// Most important setting in the swap chain. it represents the actual condition for showing imags to the screen.
	// 4 possible modes in vulkan: VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR, VK_PRESENT_MODE_FIFO_RELAXED_KHR, VK_PRESENT_MODE_MAILBOX_KHR.
	// Find descriptions here: https://vulkan-tutorial.com/Drawing_a_triangle/Presentation/Swap_chain
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
	{
		// we desire the the mailboxing if it is available. best framerate without tearing
		for (const auto& availablePresentMode : availablePresentModes) {
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return availablePresentMode;
			}
		}

		// All swapchain devices will have this capability
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	// The extent is the resolution of swap chain images. Most of the time it should equal the resolution of the window in pixels.
	// range of possible resolutions is defined by VkSurfaceCapabilitiesKHR .
	// There are 2 units when measuring size. Pixels and screen coordinates. Sometimes coordinates do not contain the same value scaling.
	VkExtent2D chooseSwapExtent(GLFWwindow* window, const VkSurfaceCapabilitiesKHR& capabilities)
	{
		if (capabilities.currentExtent.width != 0xFFFFFFFF) {//std::numeric_limits<uint32_t>::max()) {
			return capabilities.currentExtent;
		}
		else {
			int width, height;
			glfwGetFramebufferSize(window, &width, &height);

			VkExtent2D actualExtent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};

			// if width < less return less or width > max return max
			actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

			return actualExtent;
		}
	}

	VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) {
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;
		viewInfo.subresourceRange.aspectMask = aspectFlags;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = mipLevels;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		VkImageView imageView;
		if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture image view!");
		}

		return imageView;
	}
}

namespace GE
{

	SwapchainHandle::SwapchainHandle() : internals(), currentError(), window(nullptr), device(nullptr), physicalDevice(nullptr),surface(nullptr){};
	SwapchainHandle::~SwapchainHandle() = default;
	void SwapchainHandle::Free()
	{
		if (device == nullptr)return;

		FreeJustExtras();

		if (internals.renderPass != nullptr)vkDestroyRenderPass(device, internals.renderPass, nullptr);
		internals.renderPass = nullptr;

		if (internals.swapChain != nullptr)
		{
			for (auto imageView : internals.swapchainImageViews) {
				vkDestroyImageView(device, imageView, nullptr);
			}
			internals.swapchainImageViews.clear();
			vkDestroySwapchainKHR(device, internals.swapChain, nullptr);
		}
		internals.swapChain = nullptr;

	}
	void SwapchainHandle::FreeJustExtras()
	{

		SwapchainBufferInternals& extras = bufferInternals;
		if (extras.colorImageView != nullptr)vkDestroyImageView(device, extras.colorImageView, nullptr);
		if (extras.colorImage != nullptr)vkDestroyImage(device, extras.colorImage, nullptr);
		if (extras.colorImageMemory != nullptr)vkFreeMemory(device, extras.colorImageMemory, nullptr);
		if (extras.depthImageView != nullptr)vkDestroyImageView(device, extras.depthImageView, nullptr);
		if (extras.depthImage != nullptr)vkDestroyImage(device, extras.depthImage, nullptr);
		if (extras.depthImageMemory != nullptr)vkFreeMemory(device, extras.depthImageMemory, nullptr);

		for (auto framebuffer : extras.swapchainFramebuffers) {
			vkDestroyFramebuffer(device, framebuffer, nullptr);
		}
		extras.swapchainFramebuffers.clear();
		bufferInternals.colorImage = nullptr;
		bufferInternals.colorImageMemory = nullptr;
		bufferInternals.colorImageView = nullptr;
		bufferInternals.depthImage = nullptr;
		bufferInternals.depthImageMemory = nullptr;
		bufferInternals.depthImageView = nullptr;

		//for (auto& [_, extras] : internals.extrasList)
		//{
		//	if (extras.colorImageView != nullptr)vkDestroyImageView(device, extras.colorImageView, nullptr);
		//	if (extras.colorImage != nullptr)vkDestroyImage(device, extras.colorImage, nullptr);
		//	if (extras.colorImageMemory != nullptr)vkFreeMemory(device, extras.colorImageMemory, nullptr);
		//	if (extras.depthImageView != nullptr)vkDestroyImageView(device, extras.depthImageView, nullptr);
		//	if (extras.depthImage != nullptr)vkDestroyImage(device, extras.depthImage, nullptr);
		//	if (extras.depthImageMemory != nullptr)vkFreeMemory(device, extras.depthImageMemory, nullptr);

		//	for (auto framebuffer : extras.swapchainFramebuffers) {
		//		vkDestroyFramebuffer(device, framebuffer, nullptr);
		//	}
		//}
		//attachRenderPasses.clear();
		//internals.extrasList.clear();




	}

	std::string_view SwapchainHandle::getError()const
	{
		return currentError;
	}
	const SwapchainInternals& SwapchainHandle::Internals() const
	{
		return internals;
	}
	const SwapchainBufferInternals& SwapchainHandle::InternalsBuffers() const
	{
		return bufferInternals;
	}

	bool SwapchainHandle::initSwapchain(GLFWwindow* window, VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSampleCountFlagBits msaaSamples)
	{
		if (device == nullptr) {
			currentError = "VkDevice is null";
			return false;
		} if (window == nullptr) {
			currentError = "window is null";
			return false;
		}
		if (physicalDevice == nullptr) {
			currentError = "physicalDevice is null";
			return false;
		}
		if (surface == nullptr) {
			currentError = "surface is null";
			return false;
		}
		this->window = window;
		this->device = device;
		this->physicalDevice = physicalDevice;
		this->surface = surface;
		this->msaaSamples = msaaSamples;
		return createSwapchain();
	}

	bool SwapchainHandle::createSwapchain()
	{
		VkSurfaceCapabilitiesKHR _capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;

		// All of the support querying functions takes the physicaldevice and surfacekhr as the first 2 args
		// query for the capabilities
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &_capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);

		if (formatCount != 0) {
			formats.resize(formatCount); //reallocate the vector for the space needed for all the formats structs
			vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);

		if (presentModeCount != 0) {
			presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data());
		}

		VkSurfaceFormatKHR surfaceFormat = ::chooseSwapSurfaceFormat(formats);
		VkPresentModeKHR presentMode = ::chooseSwapPresentMode(presentModes);
		VkExtent2D extent = ::chooseSwapExtent(window, _capabilities);
		// recommended to request at least 1 over the min image count, for rendering reasons
		uint32_t imageCount = _capabilities.minImageCount + 1;
		// If out image count goes ovre maximum then we need to set it to max
		if (_capabilities.maxImageCount > 0 && imageCount > _capabilities.maxImageCount) {
			imageCount = _capabilities.maxImageCount;
		}

		// Yes now we fill in large struct just like the other devices
		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1; // layers each image contain. use 1 unless you are 3d modleing
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // how images are loaded, we are rendering directly


		// If the phyiscal and logical devices differ we will need to speccify it, if not they both can share the same queue.
		Util::QueueFamilyIndices indices = Util::findQueueFamilies(physicalDevice, const_cast<VkSurfaceKHR&>(surface));
		uint32_t queueFamilyIndices[] = { *indices.graphicsFamily, *indices.presentFamily };
		if (indices.graphicsFamily != indices.presentFamily) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; //have to be exclusive as concurrent required 2 distinct queues
			createInfo.queueFamilyIndexCount = 0; // Optional
			createInfo.pQueueFamilyIndices = nullptr; // Optional
		}

		createInfo.preTransform = _capabilities.currentTransform; // How to rotate images if the desire, but current will leave it as it is
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // If alpha should be used with blending with other windows, Currently being ignored
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE; // Pixels can be blocked by another window.
		createInfo.oldSwapchain = VK_NULL_HANDLE; // Topic for another discussion, but from what I read, if we decide to change swap change, we will need this.
		//createInfo.oldSwapchain = swapChain;

		if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &internals.swapChain) != VK_SUCCESS) { return "failed to create swap chain!"; }

		// Like the other get functions found in vk, first to get the size, then second to get the structure
		vkGetSwapchainImagesKHR(device, internals.swapChain, &imageCount, nullptr);
		internals.swapchainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(device, internals.swapChain, &imageCount, internals.swapchainImages.data());

		internals.swapchainImageFormat = surfaceFormat.format;
		internals.swapchainExtent = extent;

		internals.swapchainImageViews.resize(internals.swapchainImages.size());

		for (size_t i = 0; i < internals.swapchainImages.size(); i++) {
			internals.swapchainImageViews[i] = Util::CreateImageView(device, internals.swapchainImages[i], internals.swapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
		}




		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = internals.swapchainImageFormat; // should match the format of the swap chain
		colorAttachment.samples = msaaSamples;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // what to do with attachment before rendering
		//if (static int doMakeDontCare = 0; doMakeDontCare++ >= 1) { colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD; }
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
		colorAttachmentResolve.format = internals.swapchainImageFormat;
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
		std::array<VkAttachmentDescription, 2> attachment2D = { depthAttachment, colorAttachmentResolve };


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



		VkFormat& colorFormat = internals.swapchainImageFormat;
		VkRenderPass& renderPass = internals.renderPass;
		SwapchainBufferInternals& extra = bufferInternals;

		//internals.extrasList[renderPass] = SwapchainInternals::Extras();
		//SwapchainInternals::Extras& extra = internals.extrasList[renderPass];

		if (auto errorMessage =
			Util::createImage(device, physicalDevice, internals.swapchainExtent.width, internals.swapchainExtent.height, 1, msaaSamples, colorFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, extra.colorImage, extra.colorImageMemory);
			!errorMessage.empty()) {
			currentError = "Color Image: " + errorMessage;
			return false;
		}

		try {
			extra.colorImageView = createImageView(device, extra.colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
		}
		catch (const std::runtime_error& e)
		{
			currentError = "Color Image: " + std::string(e.what());
			return false;
		}



		VkFormat depthFormat = Util::findDepthFormat(physicalDevice);
		if (auto errorMessage = Util::createImage(device, physicalDevice, internals.swapchainExtent.width, internals.swapchainExtent.height, 1, msaaSamples, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, extra.depthImage, extra.depthImageMemory);
			!errorMessage.empty()) {
			currentError = "Depth Image: " + errorMessage;
			return false;
		}
		try {
			extra.depthImageView = createImageView(device, extra.depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
		}
		catch (const std::runtime_error& e)
		{
			currentError = "Color Image: " + std::string(e.what());
			return false;
		}

		extra.swapchainFramebuffers.resize(internals.swapchainImageViews.size()); // The framebuffers will be from the swap chain the we prevously set up

		for (size_t i = 0; i < internals.swapchainImageViews.size(); i++) {
			std::array<VkImageView, 3> attachments = {
				extra.colorImageView,
				extra.depthImageView,//Semaphores allows the swap chain to use only 1 instance of this view
				internals.swapchainImageViews[i]
			};

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = internals.swapchainExtent.width;
			framebufferInfo.height = internals.swapchainExtent.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &extra.swapchainFramebuffers[i]) != VK_SUCCESS) {
				currentError = "failed to create framebuffer!";
				return false;
			}
		}

		return true;
	}

	bool SwapchainHandle::recreateSwapchain()
	{
		if (window == nullptr || device == nullptr)
		{
			currentError = "Must initialize the swapchain before recreating it.";
			return false;
		}

		int width = 0, height = 0;
		glfwGetFramebufferSize(window, &width, &height);
		while (width == 0 || height == 0) {
			glfwGetFramebufferSize(window, &width, &height);
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(device); // wait for shared resources to be freed

		Free();
		return createSwapchain();
		//if (!createSwapchain())return false;
		//return createSwapchainExtras();
		
		
	}




	void SwapchainHandle::setCommandBuffer(CommandBuffers& commandBuffers)
	{
		this->commandBuffers = commandBuffers;
	}

	bool SwapchainHandle::beginRenderPass(uint32_t currentFrame, uint32_t imageIndex, const VkClearColorValue& backgroundColor)
	{
	
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0; // Optional
		beginInfo.pInheritanceInfo = nullptr; // Optional. Used for secondary command buffers
		// If the command buffer was already recorded once, then vkBeginCommandBuffer will implicity reset it. 
		if (vkBeginCommandBuffer(commandBuffers[currentFrame], &beginInfo) != VK_SUCCESS) {
			currentError = "failed to begin recording command buffer!";
			return false;
		}
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = internals.renderPass;
		renderPassInfo.framebuffer = bufferInternals.swapchainFramebuffers[imageIndex]; // create a framebuffer for each swap chain image where it is specified as a color attachment
		renderPassInfo.renderArea.offset = { 0, 0 }; // Size for render area. Starting position
		renderPassInfo.renderArea.extent = internals.swapchainExtent; // size
		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = backgroundColor;// Black with 100% opacity
		clearValues[1].depthStencil = { 1.0f, 0 }; // 1 is far plane, 0 is near plane
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();
		// Render pass can now begin. All function that record commands can be recongnized by their vkCmd
		vkCmdBeginRenderPass(commandBuffers[currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		return true;
	}
	bool SwapchainHandle::endRenderPass(uint32_t currentFrame)
	{
		vkCmdEndRenderPass(commandBuffers[currentFrame]);
		if (vkEndCommandBuffer(commandBuffers[currentFrame]) != VK_SUCCESS) {
			currentError = "failed to record command buffer!";
			return false;
		}
		return true;
	}
	void SwapchainHandle::bindPipeline(uint32_t currentFrame, VkPipeline pipeline)
	{
		vkCmdBindPipeline(commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	}

	void SwapchainHandle::drawVertices(uint32_t currentFrame, VkPipelineLayout pipelineLayout, VkBuffer indexBuffer, uint32_t indicesSize, VkBuffer verticesBuffer, VkDescriptorSet descriptorSet)
	{
		vkCmdBindIndexBuffer(commandBuffers[currentFrame], indexBuffer, 0, VK_INDEX_TYPE_UINT32);


		// This is what is what uploading the input to the shader of the program
		VkBuffer vertexBuffers[] = { verticesBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffers[currentFrame], 0, 1, vertexBuffers, offsets);

		// Binding our descriptor sets to the frame. Specifing that its for the graphics over the compute pipeline. 
		vkCmdBindDescriptorSets(commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

		vkCmdDrawIndexed(commandBuffers[currentFrame], indicesSize, 1, 0, 0, 0);
	}

}

