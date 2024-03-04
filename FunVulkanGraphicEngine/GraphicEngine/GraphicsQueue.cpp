#include "GraphicEngine/GraphicsQueue.hpp"

#include <vector>


namespace GE::Util
{
	bool QueueFamilyIndices::isComplete() const
	{
		return graphicsFamily.has_value() && presentFamily.has_value();
	}

	std::string QueueFamilyIndices::toString() const 
	{
		std::stringstream ss;
		ss << *this;
		return ss.str();
	}


	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR& surface)
	{
		QueueFamilyIndices indices;
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		// VkQueueFamilyProperties contains details on queue family. Includes operations that support different queues.
		// Finding a queue family that supports VK_QQUEUES_GRAPHICS_BIT
		int i = 0;
		for (const auto& queueFamily : queueFamilies) {
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				indices.graphicsFamily = i;
			}

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

			if (presentSupport) {
				indices.presentFamily = i;
			}

			if (indices.isComplete()) {
				break;
			}

			i++;
		}

		return indices;
	}


}
namespace std
{
	std::ostream& operator<<(std::ostream& os, const GE::Util::QueueFamilyIndices& obj)
	{
		os << "QueueFamilyIndices{graphicsFamily=" << (obj.graphicsFamily.has_value() ? std::to_string(*obj.graphicsFamily) : "NA")
			<< ",presentFamily=" << (obj.presentFamily.has_value() ? std::to_string(*obj.presentFamily) : "NA") << "}";
		return os;
	}
}