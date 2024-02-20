#include "graphic/VQueues.h"

namespace V
{
	std::string QueueFamilyIndices::toString() {
			std::stringstream ss;
			ss << "QueueFamilyIndices: graphicsFamily=" << graphicsFamily << ", active=" << (graphicsFamilySet ? "true" : "false");
			ss << "; presentFamily=" << presentFamily << ", active=" << (presentFamilySet ? "true" : "false");

			return ss.str();
		}

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)
	{
		QueueFamilyIndices indices;
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		// VkQueueFamilyProperties contains details on queue family. Includes operations that support different queues.
		// Finding a queue familty that supports VK_QQUEUES_GRAPHICS_BIT
		int i = 0;
		for (const auto& queueFamily : queueFamilies) {
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				indices.graphicsFamily = i;
				indices.graphicsFamilySet = true;
				break;
			}

			i++;
		}


		return indices;
	}

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR& surface)
	{
		QueueFamilyIndices indices;
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		// VkQueueFamilyProperties contains details on queue family. Includes operations that support different queues.
		// Finding a queue familty that supports VK_QQUEUES_GRAPHICS_BIT
		int i = 0;
		for (const auto& queueFamily : queueFamilies) {
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {

				indices.graphicsFamily = i;
				indices.graphicsFamilySet = true;
				VkBool32 presentSupport = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
				if (!presentSupport) {
					std::cout << "No queue support for the surface" << std::endl;
					continue;
				}
				indices.presentFamily = i;
				indices.presentFamilySet = true;
				break;
			}

			i++;
		}

		return indices;
	}


}