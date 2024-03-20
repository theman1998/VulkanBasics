#pragma once


#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>
#include <array>

namespace GE
{
	constexpr int MAX_FRAMES_IN_FLIGHT = 2;
	using ErrorMessage = std::string;


	using CommandBuffers = std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT>;

}