#pragma once

#include "GraphicEngine/ConstDefines.hpp"
#include "GraphicEngine/GraphicsPipeline.hpp"
#include "GraphicEngine/GraphicsDevice.hpp"


namespace GE
{

class GraphicsPipelineE2E
{
public:
	GraphicsPipelineE2E();

	void setDevices(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSampleCountFlagBits msaa);
	void appendPipeline(GraphicPipeline*);

	
	void initPipelines(VkFormat swapChainImageFormat, VkExtent2D swapchainExtent);



	std::vector<GraphicPipeline*>& Pipelines();
	std::vector<std::pair<VkRenderPass, VkSampleCountFlagBits>> grabRenderPasses();

private:
	VkDevice device;
	VkPhysicalDevice physicalDevice;
	VkSampleCountFlagBits msaa;


	GraphicsCommandPool commandPool;


	std::vector<GraphicPipeline*> pipelines;

};

}