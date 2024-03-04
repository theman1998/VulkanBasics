#pragma once

#include "GraphicEngine/Validation.hpp"

#include <cstdint> // Necessary for uint32_t
#include <limits> // Necessary for std::numeric_limits
#include <algorithm> // Necessary for std::clamp
#include <vector>
#include <string>


namespace GE::Util
{

	const std::vector<const char*> deviceExtensions = {
	    VK_KHR_SWAPCHAIN_EXTENSION_NAME // used to enable the swap chain
	};


	bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface );
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);


	std::vector<VkExtensionProperties> getExtensionPropertiesList(); //Gets the vector list of all usable extension properties

	bool checkUsedInstancesAreValid(const char** glfwExtensions, uint32_t count); // Checks our grab properies and compares it to the all extensionPropertiesList

	std::vector<const char*> getRequiredExtensions();


	std::vector<char> readFile(const std::string& filename);





	// Three types of properties, that we need to check before swapchain
	// * Basic surface capabilities(min / max number of images in swap chain, min / max width and height of images)
	// * Surface formats(pixel format, color space)
	// * Available presentation modes

	// If the conditions were met for swap chain support, then there are additional modes that we must query and verify.
	// 3 types to determine
	// * Surface format (color depth)
	// * Presentation mode (conditions for "swapping" images to the screen)
	// * Swap extent (resolution of images in swap chain)

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	// Used to verify swap chain support
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);




	// For Render Pass


	VkFormat findSupportedFormat(VkPhysicalDevice physicalDevice, std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	VkFormat findDepthFormat(VkPhysicalDevice physicalDevice);

	uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);


}