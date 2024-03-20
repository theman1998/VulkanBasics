#pragma once

#include "GraphicEngine/GraphicsObjectController.hpp"
#include "GraphicEngine/ConstDefines.hpp"

namespace GE
{
	struct ThingManagerPIMPL{
		GraphicsObjectController * controller;
		VkDevice device;
		VkPhysicalDevice physicalDevice;
		VkQueue queue;
		VkCommandPool commandPool;
		VkDescriptorSetLayout descriptorSetLayout;
	};
}