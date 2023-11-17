#include "graphic/VSwapChain.h"


namespace V {

	// Used to verify swap chain support
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
		SwapChainSupportDetails details;

		// All of the support querying functions takes the physicaldevice and surfacekhr as the first 2 args
		// query for the capabilities
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

		if (formatCount != 0) {
			details.formats.resize(formatCount); //reallocate the vector for the space needed for all the formats structs
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

		if (presentModeCount != 0) {
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
		}



		return details;
	}

	//Surface format
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
		if (capabilities.currentExtent.width != 0xFFFFFFFF){//std::numeric_limits<uint32_t>::max()) {
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










}