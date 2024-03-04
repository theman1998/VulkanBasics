#pragma once

#include "GraphicEngine/ConstDefines.hpp"
#include "GraphicEngine/GraphicsVertex.hpp"


namespace GE
{
	enum class ShaderType { Vertex, Fragment, Undefined };
	struct ShaderLoadInfo 
	{
		std::string name;
		std::string fileName;
		ShaderType type;

	};

	struct GraphicPipelineInternals
	{
		///@brief Used to draw indirectly
		VkCommandPool commandPool{ nullptr };
		///@brief Used for allocations buffer
		std::vector < VkCommandBuffer > commandBuffers{};
		///@brief Used for render passing
		VkRenderPass renderPass{ nullptr };
		///@brief Keeps reference to the Uniform buffers object
		VkDescriptorSetLayout descriptorSetLayout{ nullptr };
		///@brief required for the graphic pipeline
		VkPipelineLayout pipelineLayout{ nullptr };
		///@brief The final piece
		VkPipeline graphicsPipeline{ nullptr };
	};

	class GraphicPipeline
	{
	public:
		GraphicPipeline(VkDevice device = nullptr);
		~GraphicPipeline();
		void Free();
		const ErrorMessage& getError() const;

		const GraphicPipelineInternals & Internals() const;
		GraphicPipelineInternals& Internals();

		// Use the following in sequential order to set up pipeline
		bool initDevice(VkDevice device);
		bool initCommandInfo(VkPhysicalDevice, VkSurfaceKHR);
		bool setShaders(const std::vector<ShaderLoadInfo>&);
		// Following broken down in two parts, so later we can more easily add configurations to the routines.
		bool initRenderPass(VkPhysicalDevice, VkFormat swapChainImageFormat, VkSampleCountFlagBits msaaSamples);
		bool initPipeline(VkExtent2D swapchainExtent, VkSampleCountFlagBits msaaSamples);


		bool updateViewPort(uint32_t currentFrame, VkExtent2D swapchainExtent);

		/// @brief Begins the command buffer and render pass to allow for draw commands.
		bool startPipelinePass(uint32_t currentFrame, VkExtent2D swapchainExtent, VkFramebuffer swapchainFrameBuffer, const VkClearColorValue& backgroundColor);
		/// @brief Binds and Commands draw commands on desired buffers
		bool bindAndDrawIndexVertices(uint32_t currentFrame, VkBuffer indexBuffer, uint32_t indicesSize, VkBuffer verticesBuffer, VkDescriptorSet descriptorSet);
		/// @brief finalize the rending and culling pass.
		bool completePipelinePass(uint32_t currentFrame);

	private:
		GraphicPipelineInternals internals;
		/// @brief Used within pipeline to load in the shader programs
		std::vector<ShaderLoadInfo> shaderLoadInfoList;

		VkDevice device;

		ErrorMessage currentError;

	};


}