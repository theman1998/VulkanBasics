#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <sstream>

#include <cstdint> // Necessary for uint32_t
#include <limits> // Necessary for std::numeric_limits
#include <algorithm> // Necessary for std::clamp


namespace V {



	// Three types of properties, that we need to check before swapchain
	// * Basic surface capabilities(min / max number of images in swap chain, min / max width and height of images)
	// * Surface formats(pixel format, color space)
	// * Available presentation modes

	// If the conditions were met for swap chain support, then there are additional modes that we must query and verifiy.
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

	//Surface format
	// formats can contain rgba / svg,  bit pixel size, defines the color space
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

	// Most important setting in the swap chain. it represents the actual condition for showing imags to the screen.
	// 4 possible modes in vulkan: VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR, VK_PRESENT_MODE_FIFO_RELAXED_KHR, VK_PRESENT_MODE_MAILBOX_KHR.
	// Find descriptions here: https://vulkan-tutorial.com/Drawing_a_triangle/Presentation/Swap_chain
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

	// The extent is the resolution of swap chain images. Most of the time it should equal the resolution of the window in pixels.
	// range of possible resolutions is defined by VkSurfaceCapabilitiesKHR .
	// There are 2 units when measuring size. Pixels and screen coordinates. Sometimes coordinates do not contain the same value scaling.
	VkExtent2D chooseSwapExtent(GLFWwindow* window, const VkSurfaceCapabilitiesKHR& capabilities);










}