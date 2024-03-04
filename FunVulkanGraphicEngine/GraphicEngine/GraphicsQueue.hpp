#pragma once
#include "GraphicEngine/ConstDefines.hpp"

#include <optional>
#include <string>
#include <ostream>
#include <sstream>


namespace GE
{

	struct Queues
	{
		// Contains the swapchain which holds the visuals for the surface window
		VkQueue presentQueue;
		// used to interact with the queues made from the logical devices (vkDevie). Command buffers for the pipeline.
		VkQueue graphicsQueue;
	};

	namespace Util
	{
		struct QueueFamilyIndices
		{
			std::optional<uint32_t> graphicsFamily;
			std::optional<uint32_t> presentFamily;

			bool isComplete() const;

			std::string toString() const;
		};
		QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR& surface);
	}

}



namespace std {
	std::ostream& operator<<(std::ostream& os, const GE::Util::QueueFamilyIndices& obj);
}