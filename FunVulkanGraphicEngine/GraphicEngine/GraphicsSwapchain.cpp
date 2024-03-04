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

	SwapchainHandle::SwapchainHandle() : internals(), currentError(), window(nullptr), device(nullptr), physicalDevice(nullptr),surface(nullptr),renderPass(nullptr), msaaSamples(VK_SAMPLE_COUNT_1_BIT){};
	SwapchainHandle::~SwapchainHandle() = default;
	void SwapchainHandle::Free()
	{
		if (device == nullptr)return;
		
		if (internals.colorImageView != nullptr)vkDestroyImageView(device, internals.colorImageView, nullptr);
		if (internals.colorImage != nullptr)vkDestroyImage(device, internals.colorImage, nullptr);
		if (internals.colorImageMemory != nullptr)vkFreeMemory(device, internals.colorImageMemory, nullptr);
		if (internals.depthImageView != nullptr)vkDestroyImageView(device, internals.depthImageView, nullptr);
		if (internals.depthImage != nullptr)vkDestroyImage(device, internals.depthImage, nullptr);
		if (internals.depthImageMemory != nullptr)vkFreeMemory(device, internals.depthImageMemory, nullptr);

		for (auto framebuffer : internals.swapchainFramebuffers) {
			vkDestroyFramebuffer(device, framebuffer, nullptr);
		}
		if (internals.swapChain != nullptr)
		{
			for (auto imageView : internals.swapchainImageViews) {
				vkDestroyImageView(device, imageView, nullptr);
			}
			vkDestroySwapchainKHR(device, internals.swapChain, nullptr);
		}
	}

	std::string_view SwapchainHandle::getError()const
	{
		return currentError;
	}
	const SwapchainInternals& SwapchainHandle::Internals() const
	{
		return internals;
	}

	bool SwapchainHandle::initSwapchain(GLFWwindow* window, VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
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

		return true;
	}
	bool SwapchainHandle::initExtras(VkRenderPass renderPass, VkSampleCountFlagBits msaaSamples)
	{
		if (renderPass == nullptr) { currentError = "renderPass is null"; return false; }
		this->renderPass = renderPass;
		this->msaaSamples = msaaSamples;
		return createExtras();
	}
	bool SwapchainHandle::createExtras()
	{
		VkFormat colorFormat = internals.swapchainImageFormat;
		if (auto errorMessage =
			Util::createImage(device, physicalDevice, internals.swapchainExtent.width, internals.swapchainExtent.height, 1, msaaSamples, colorFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, internals.colorImage, internals.colorImageMemory);
			!errorMessage.empty()) {
			currentError = "Color Image: " + errorMessage;
			return false;
		}

		try {
			internals.colorImageView = createImageView(device, internals.colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
		}
		catch (const std::runtime_error& e)
		{
			currentError = "Color Image: " + std::string(e.what());
			return false;
		}



		VkFormat depthFormat = Util::findDepthFormat(physicalDevice);
		if (auto errorMessage = Util::createImage(device, physicalDevice, internals.swapchainExtent.width, internals.swapchainExtent.height, 1, msaaSamples, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, internals.depthImage, internals.depthImageMemory);
			!errorMessage.empty()) {
			currentError = "Depth Image: " + errorMessage;
			return false;
		}
		try {
			internals.depthImageView = createImageView(device, internals.depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
		}
		catch (const std::runtime_error& e)
		{
			currentError = "Color Image: " + std::string(e.what());
			return false;
		}

		internals.swapchainFramebuffers.resize(internals.swapchainImageViews.size()); // The framebuffers will be from the swap chain the we prevously set up

		for (size_t i = 0; i < internals.swapchainImageViews.size(); i++) {
			std::array<VkImageView, 3> attachments = {
				internals.colorImageView,
				internals.depthImageView,//Semaphores allows the swap chain to use only 1 instance of this view
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

			if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &internals.swapchainFramebuffers[i]) != VK_SUCCESS) {
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
		if (!createSwapchain())return false;

		std::cout << "w,h = " << width << "," << height << ";  " << internals.swapchainExtent.width << ", " << internals.swapchainExtent.height << std::endl;
		return createExtras();
	}

}

