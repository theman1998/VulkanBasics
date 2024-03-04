#pragma once

#include "GraphicEngine/ConstDefines.hpp"

#include <vector>
#include <array>
#include <memory>

namespace GE
{
	struct SwapchainInternals
	{
		// swapchain controls the outputting graphics
		VkSwapchainKHR swapChain{nullptr};
		// once swapchain is setup we can control the images through this member
		std::vector<VkImage> swapchainImages{};
		//need this in future chapters
		VkFormat swapchainImageFormat{};
		//need this in future chapters
		VkExtent2D swapchainExtent{};
		// Images that wil get render the the pipeline
		std::vector<VkImageView> swapchainImageViews{};


		// Uses same calls as the texture. Allows the rasterizer to do a depth check
		VkImage colorImage; // msaa requires to be processes of screen. So we need a new buffer to store the multisample pixels
		VkDeviceMemory colorImageMemory;
		VkImageView colorImageView;
		VkImage depthImage;
		VkDeviceMemory depthImageMemory;
		VkImageView depthImageView;

		// TODO: Does this object belong here?
		// Holds the framebuffers for the rendering pass.
		std::vector<VkFramebuffer> swapchainFramebuffers;
		VkFramebufferCreateInfo framebufferInfo;
	};

	class SwapchainHandle {
	public:
		SwapchainHandle();
		~SwapchainHandle();
		void Free();
		std::string_view getError()const;
		const SwapchainInternals& Internals() const;

		bool initSwapchain(GLFWwindow* window, VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
		bool initExtras(VkRenderPass renderPass, VkSampleCountFlagBits msaaSamples);

		bool recreateSwapchain();

	protected:
		bool createSwapchain();
		bool createExtras();

	private:

		SwapchainInternals internals;

		std::string currentError;



		// Extranstic Variables
		GLFWwindow* window;
		VkDevice device;
		VkPhysicalDevice physicalDevice;
		VkSurfaceKHR surface;
		VkRenderPass renderPass;
		VkSampleCountFlagBits msaaSamples;
	};



}