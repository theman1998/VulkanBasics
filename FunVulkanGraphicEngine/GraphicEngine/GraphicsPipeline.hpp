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
		bool setDevice(VkDevice device);
		void setCommandBuffers(CommandBuffers& buffers);
		
		
		bool setShaders(const std::vector<ShaderLoadInfo>&);
		std::vector<ShaderLoadInfo> getShaders()const;
		bool initPipeline(VkPhysicalDevice physicalDevice, VkFormat swapChainImageFormat, VkExtent2D swapchainExtent, VkSampleCountFlagBits msaaSamples, VkRenderPass renderPass);


		uint64_t pipelineId{ 0 };

	private:
		GraphicPipelineInternals internals;

		/// @brief Used within pipeline to load in the shader programs
		std::vector<ShaderLoadInfo> shaderLoadInfoList;

		VkDevice device;
		ErrorMessage currentError;

		VkRenderPass renderPass{nullptr};
		CommandBuffers commandBuffers{nullptr,nullptr};
	};


}