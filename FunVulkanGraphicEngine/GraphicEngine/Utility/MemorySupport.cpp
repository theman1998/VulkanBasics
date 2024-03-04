#include "GraphicEngine/Utility/MemorySupport.hpp"
#include <exception>
#include <stdexcept>
#include "GraphicEngine/Utility/DeviceSupport.hpp"


namespace GE::Util
{









	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue)
	{
		VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

		VkBufferCopy copyRegion{};
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		endSingleTimeCommands(commandBuffer, device, commandPool, graphicsQueue);
	}
	VkCommandBuffer beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool)
	{
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;// VK_COMMAND_POOL_CREATE_TRANSIENT_BIT  if we want to create a command pool for this
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPool;
		allocInfo.commandBufferCount = 1;
		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;// Good practice to tell driver the intent usage
		vkBeginCommandBuffer(commandBuffer, &beginInfo);
		return commandBuffer;
	}
	void endSingleTimeCommands(VkCommandBuffer commandBuffer, VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue)
	{
		vkEndCommandBuffer(commandBuffer);// Contains only copy command, need to stop recording

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		// There are 2 things we could do to wait for the transmission to complete
		// 1) Add a fence and wait for it to be free.
		// 2) wait tell the queue become idle.
		vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(graphicsQueue);

		vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
	}

	VkImageView CreateImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels)
	{
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;
		viewInfo.subresourceRange.aspectMask = aspectFlags;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = mipLevels;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		VkImageView imageView;
		if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture image view!");
		}
		return imageView;
	}

	std::string createImage(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
	{
		// Extent defines the dimensinal the image is
		// 1D images store an array of data or gradient
		// 2D images usefull for texures
		// 3D store voxels volumes
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1; // 1d as we our image uses array
		imageInfo.mipLevels = mipLevels;
		imageInfo.arrayLayers = 1;
		imageInfo.format = format; // Should be the same as the texels as with buffer
		imageInfo.tiling = tiling; // How the pixels are ordered. Use linear if going into buffer manually is desired. not in our case. The shader will be doing it.
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Not usable by GPU.
		imageInfo.usage = usage;// The image is copy is going directly to its desination
		imageInfo.samples = numSamples; //Used with Multi Sampling
		imageInfo.flags = 0; // Optional. Used with "Sparse Images"
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; //Only going to one queue family

		if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {return "failed to create image!";}

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(device, image, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = Util::findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties); // ERROR MAYBE?
		// Image allocation is same as buffer allocation
		if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) { return "failed to allocate image memory!"; }

		vkBindImageMemory(device, image, imageMemory, 0);
		return "";
	}
}