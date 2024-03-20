#pragma once

#include "GraphicEngine/ConstDefines.hpp"

#include <vector>
#include <array>
#include <memory>

#include <unordered_map>

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

		///@brief Only one is needed for many pipelines
		VkRenderPass renderPass{ nullptr };


		//struct Extras {
		//	// Uses same calls as the texture. Allows the rasterizer to do a depth check
		//	VkImage colorImage; // msaa requires to be processes of screen. So we need a new buffer to store the multisample pixels
		//	VkDeviceMemory colorImageMemory;
		//	VkImageView colorImageView;
		//	VkImage depthImage;
		//	VkDeviceMemory depthImageMemory;
		//	VkImageView depthImageView;

		//	// TODO: Does this object belong here?
		//	// Holds the framebuffers for the rendering pass.
		//	std::vector<VkFramebuffer> swapchainFramebuffers;
		//	VkFramebufferCreateInfo framebufferInfo;
		//};
		//std::unordered_map<VkRenderPass, Extras> extrasList;





	};

	struct SwapchainBufferInternals
	{
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
		void FreeJustExtras();

		std::string_view getError()const;
		const SwapchainInternals& Internals() const;
		const SwapchainBufferInternals& InternalsBuffers() const;

		bool initSwapchain(GLFWwindow* window, VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSampleCountFlagBits msaaSamples);
		bool recreateSwapchain();
		
		void setCommandBuffer(CommandBuffers& commandBuffers);





		/// ------------------------------------------------------
		/// Drawling Commands. AKA Graphic pipeline instructions
		/// ------------------------------------------------------
		/// @Notifies the GPU to sets up the render pass 
		bool beginRenderPass(uint32_t currentFrame, uint32_t imageIndex, const VkClearColorValue& backgroundColor);
		/// Attaching the pipeline to handle the shaders stages
		void bindPipeline(uint32_t currentFrame, VkPipeline pipeline);
		/// begin raterizing and rending the data
		void drawVertices(uint32_t currentFrame, VkPipelineLayout pipelineLayout, VkBuffer indexBuffer, uint32_t indicesSize, VkBuffer verticesBuffer, VkDescriptorSet descriptorSet);
		/// @Complete and Render out the computed information
		bool endRenderPass(uint32_t currentFrame);
		/// ------------------------------------------------------

	protected:
		bool createSwapchain();

	private:

		SwapchainInternals internals;
		SwapchainBufferInternals bufferInternals;

		std::string currentError;

		// Extranstic Variables
		GLFWwindow* window;
		VkDevice device;
		VkPhysicalDevice physicalDevice;
		VkSurfaceKHR surface;
		VkSampleCountFlagBits msaaSamples;
		CommandBuffers commandBuffers{ nullptr,nullptr };
	};



}