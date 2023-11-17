#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <sstream>

namespace V
{

	// indices = index
	struct QueueFamilyIndices
	{
		uint32_t graphicsFamily{ 0 };
		bool graphicsFamilySet{ false }; // a queue familty could be 0. set true once a vlaue is set

		uint32_t presentFamily{ 0 };
		bool presentFamilySet{ false }; // a queue familty could be 0. set true once a vlaue is set

		bool isComplete() { return presentFamilySet && graphicsFamilySet; }

		std::string toString();
	};

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR& surface);


}