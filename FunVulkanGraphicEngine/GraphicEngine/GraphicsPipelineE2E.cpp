#include "GraphicEngine/GraphicsPipelineE2E.hpp"

#include <iostream>

namespace GE
{

GraphicsPipelineE2E::GraphicsPipelineE2E() : device(nullptr), physicalDevice(nullptr), msaa(VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT)
{}

void GraphicsPipelineE2E::appendPipeline(GraphicPipeline* pipeline)
{
	assert(pipeline->getShaders().empty() == false, "Pipeline needs to have all meta info attach before appending");
	pipelines.push_back(pipeline);
}

void GraphicsPipelineE2E::setDevices(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSampleCountFlagBits msaa)
{
	this->device = device;
	this->physicalDevice = physicalDevice;
	this->msaa = msaa;

	commandPool.setDevice(device);
	if (!commandPool.init(physicalDevice, surface))
	{
		std::cout << "ERROR: " << commandPool.currentError << std::endl;
	}
}



void GraphicsPipelineE2E::initPipelines(VkFormat swapChainImageFormat, VkExtent2D swapchainExtent)
{
	//std::vector<GraphicPipeline*> pipelinesToRemove;
	//for (GraphicPipeline* pipeline : pipelines)
	//{
	//	if (!pipeline->initPipeline(physicalDevice, swapChainImageFormat, swapchainExtent, msaa))
	//	{
	//		assert(false, "pipeline has failed");
	//		pipelinesToRemove.push_back(pipeline);

	//	}
	//}
	//for (GraphicPipeline* pipeline : pipelinesToRemove)
	//{
	//	auto newEnd = std::remove(pipelines.begin(), pipelines.end(), pipeline);
	//}
	//if (pipelinesToRemove.empty())
	//{
	//	throw std::runtime_error("There are no valid pipelines when initializing GraphicsPipelineE2E");
	//}

}


std::vector<GraphicPipeline*>& GraphicsPipelineE2E::Pipelines()
{
	return pipelines;
}

std::vector<std::pair<VkRenderPass, VkSampleCountFlagBits>> GraphicsPipelineE2E::grabRenderPasses()
{
	std::vector<std::pair<VkRenderPass, VkSampleCountFlagBits>> res;

	//for (auto* pipeline : pipelines)
	//{
	//	res.emplace_back(pipeline->Internals().renderPass, msaa);
	//}

	return res;
}

}