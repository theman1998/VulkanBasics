#include "GraphicEngine/GraphicsVertex.hpp"
#include "GraphicEngine/Utility/DeviceSupport.hpp"
#include "GraphicEngine/Utility/MemorySupport.hpp"

#include <cmath>
#include <algorithm>

// Only 1 file should define the implemenation
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>


namespace {

	std::string createBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
	{
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) { return "failed to create buffer!"; }

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = GE::Util::findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) { return "failed to allocate buffer memory!"; }

		vkBindBufferMemory(device, buffer, bufferMemory, 0);

		return "";
	}


	std::string transitionImageLayout(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels)
	{
		// Could copy image data into bkCmdCopyBufferToImage. but we need to have image in the right layout
		VkCommandBuffer commandBuffer = GE::Util::beginSingleTimeCommands(device, commandPool);

		// Barries are used to synchronize access to resouces, like images. 
		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout; // Can use VK_IMAGE_LAYOUT_UNDEFINED if we don't care about existing contents of image
		barrier.newLayout = newLayout;

		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; //used if we are transforing ownership of queues
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		barrier.image = image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = mipLevels;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		// More information on the aforementioned table
		// https://registry.khronos.org/vulkan/specs/1.3-extensions/html/chap7.html#VkPipelineStageFlagBits
		// First stage within the transition
		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = 0; // Don't need to wait on anything
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; // Going to copy image data over to shader

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;//Not real graphic pipeline stage. Its a pseudo stage where the transfer is happings.
		}
		// Second stage of transision
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else { return "unsupported layout transition!"; }

		// 2nd param - defines pipeline stage the operation occurs before the barrier
		// 3rd param - pipeline stage in which to wait for barrier
		// 4th - 0 or VK_DEPENDENCY_BY_REGION_BIT. Allow to read part that were written.
		// 5th - reference to memory barriers
		// 6th - buffer memory barriers
		// 7th - image memory barriers
		vkCmdPipelineBarrier(
			commandBuffer,
			sourceStage, destinationStage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);

		GE::Util::endSingleTimeCommands(commandBuffer, device, commandPool, graphicsQueue);

		return "";
	}



	std::string copyBufferToImage(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
	{
		VkCommandBuffer commandBuffer = GE::Util::beginSingleTimeCommands(device, commandPool);

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;

		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = {
			width,
			height,
			1
		};

		vkCmdCopyBufferToImage(
			commandBuffer,
			buffer,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,// Need to match up with the operation that was defined when allocating the image
			1,
			&region
		);

		GE::Util::endSingleTimeCommands(commandBuffer, device, commandPool, graphicsQueue);
		return "";
	}


	std::string generateMipmaps(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue, VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels)
	{
		// Check if image format supports linear blitting
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);

		// 2 other options.
		// 1) Create a feature that allows for mipmapping on certain images
		// 2) Precompile images which will be used in replace with the blitting
		if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) { return "texture image format does not support linear blitting!"; }

		VkCommandBuffer commandBuffer = GE::Util::beginSingleTimeCommands(device, commandPool);

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = image;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.levelCount = 1;

		int32_t mipWidth = texWidth;
		int32_t mipHeight = texHeight;

		for (uint32_t i = 1; i < mipLevels; i++) {
			barrier.subresourceRange.baseMipLevel = i - 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

			VkImageBlit blit{};
			blit.srcOffsets[0] = { 0, 0, 0 };
			blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
			blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.srcSubresource.mipLevel = i - 1;
			blit.srcSubresource.baseArrayLayer = 0;
			blit.srcSubresource.layerCount = 1;
			blit.dstOffsets[0] = { 0, 0, 0 };
			blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
			blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.mipLevel = i;
			blit.dstSubresource.baseArrayLayer = 0;
			blit.dstSubresource.layerCount = 1;

			vkCmdBlitImage(commandBuffer,
				image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &blit,
				VK_FILTER_LINEAR);

			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

			if (mipWidth > 1) mipWidth /= 2;
			if (mipHeight > 1) mipHeight /= 2;
		}


		barrier.subresourceRange.baseMipLevel = mipLevels - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		GE::Util::endSingleTimeCommands(commandBuffer, device, commandPool, graphicsQueue);

		return "";
	}



}

namespace GE
{
	VkVertexInputBindingDescription Vertex::getBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescription;
	}

	bool Vertex::operator==(const Vertex& other) const {
		return pos == other.pos && color == other.color && texCoord == other.texCoord;
	}

	std::array<VkVertexInputAttributeDescription, 3> Vertex::getAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0; // Look into our sharder.vert file. 
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; // second input is 3d
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

		return attributeDescriptions;
	}


	VerticesHandle::VerticesHandle() = default;
	VerticesHandle::~VerticesHandle() = default;
	void VerticesHandle::Free() {
		if (device == nullptr)return;

		if (internals.indexBuffer != nullptr)vkDestroyBuffer(device, internals.indexBuffer, nullptr);
		if (internals.indexBufferMemory != nullptr)vkFreeMemory(device, internals.indexBufferMemory, nullptr);
		if (internals.vertexBuffer != nullptr)vkDestroyBuffer(device, internals.vertexBuffer, nullptr);
		if (internals.vertexBufferMemory != nullptr)vkFreeMemory(device, internals.vertexBufferMemory, nullptr);
	}
	std::string_view VerticesHandle::getError() const { return currentError; }
	const VerticesInternal& VerticesHandle::Internals()const { return internals; }
	bool VerticesHandle::init(std::string_view filePath, VkDevice device, VkPhysicalDevice physicalDevice, VkQueue graphicsQueue, VkCommandPool commandPool, VkDescriptorSetLayout descriptorSetLayout)
	{
		if (device == nullptr) {
			currentError = "Must insert a valid device";
			return false;
		}
		if (commandPool == nullptr) {
			currentError = "Must insert a valid command pool";
			return false;
		}
		if (physicalDevice == nullptr)
		{
			currentError = "Must insert a valid physical device";
			return false;
		}
		if (descriptorSetLayout == nullptr)
		{
			currentError = "Must insert a valid descriptorSetLayout";
			return false;
		}
		this->device = device;

		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filePath.data())) { currentError = (warn + err); return false; }

		std::unordered_map<Vertex, uint32_t> uniqueVertices{};
		std::vector<uint32_t> indices;
		std::vector<Vertex> vertices;

		for (const auto& shape : shapes) {
			for (const auto& index : shape.mesh.indices) {
				Vertex vertex{};

				vertex.pos = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]
				};

				vertex.texCoord = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
				};

				vertex.color = { 1.0f, 1.0f, 1.0f };

				if (uniqueVertices.count(vertex) == 0) {
					uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
					vertices.push_back(vertex);
				}

				indices.push_back(uniqueVertices[vertex]);
			}
		}

		return init(vertices,indices,device,physicalDevice,graphicsQueue,commandPool,descriptorSetLayout);
	}

	bool VerticesHandle::init(std::vector<Vertex> vertices, std::vector<uint32_t> indices, VkDevice device, VkPhysicalDevice physicalDevice, VkQueue graphicsQueue, VkCommandPool commandPool, VkDescriptorSetLayout descriptorSetLayout)
	{
		if (device == nullptr) {
			currentError = "Must insert a valid device";
			return false;
		}
		if (commandPool == nullptr) {
			currentError = "Must insert a valid command pool";
			return false;
		}
		if (physicalDevice == nullptr)
		{
			currentError = "Must insert a valid physical device";
			return false;
		}
		if (descriptorSetLayout == nullptr)
		{
			currentError = "Must insert a valid descriptorSetLayout";
			return false;
		}
		this->device = device;


		internals.vertices = vertices;
		internals.indices = indices;

		{
			VkDeviceSize bufferSize = sizeof(internals.vertices[0]) * internals.vertices.size();

			VkBuffer stagingBuffer;
			VkDeviceMemory stagingBufferMemory;
			if (auto errorMessage = createBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
				!errorMessage.empty()
				) {
				currentError = "vertices buffer 1: " + errorMessage;
				return false;
			}

			void* data;
			vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
			memcpy(data, internals.vertices.data(), (size_t)bufferSize);
			vkUnmapMemory(device, stagingBufferMemory);

			// Reason for having this extra buffer, is so that we can load our vertex in more performant memory
			if (auto errorMessage = createBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, internals.vertexBuffer, internals.vertexBufferMemory);
				!errorMessage.empty()) {
				currentError = "Vertices buffer 2: " + errorMessage;
				return false;
			}
			Util::copyBuffer(stagingBuffer, internals.vertexBuffer, bufferSize, device, commandPool, graphicsQueue);
			vkDestroyBuffer(device, stagingBuffer, nullptr);
			vkFreeMemory(device, stagingBufferMemory, nullptr);
		}

		{
			VkDeviceSize bufferSize = sizeof(internals.indices[0]) * internals.indices.size();

			VkBuffer stagingBuffer;
			VkDeviceMemory stagingBufferMemory;
			if (auto errorMessage = createBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
				!errorMessage.empty()
				) {
				currentError = "indices buffer 1: " + errorMessage;
				return false;
			}

			void* data;
			vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
			memcpy(data, internals.indices.data(), (size_t)bufferSize);
			vkUnmapMemory(device, stagingBufferMemory);

			// Reason for having this extra buffer, is so that we can load our vertex in more performant memory
			if (auto errorMessage = createBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, internals.indexBuffer, internals.indexBufferMemory);
				!errorMessage.empty()) {
				currentError = "vertices buffer 2: " + errorMessage;
				return false;
			}
			Util::copyBuffer(stagingBuffer, internals.indexBuffer, bufferSize, device, commandPool, graphicsQueue);
			vkDestroyBuffer(device, stagingBuffer, nullptr);
			vkFreeMemory(device, stagingBufferMemory, nullptr);
		}
		return true;
	}



	GraphicsTextureHandle::GraphicsTextureHandle() :internals(), currentError(), device(nullptr){}
	GraphicsTextureHandle::~GraphicsTextureHandle() {}
	void GraphicsTextureHandle::Free() {
		if (device == nullptr)return;

		TextureInternal& textureInfo = internals.texture;
		if (textureInfo.textureSampler != nullptr)vkDestroySampler(device, textureInfo.textureSampler, nullptr);
		if (textureInfo.textureImageView != nullptr)vkDestroyImageView(device, textureInfo.textureImageView, nullptr);
		if (textureInfo.textureImage != nullptr)vkDestroyImage(device, textureInfo.textureImage, nullptr);
		if (textureInfo.textureImageMemory != nullptr)vkFreeMemory(device, textureInfo.textureImageMemory, nullptr);

		UBOInternal& uniformBuffer = internals.ubo;

		for (auto obj : uniformBuffer.uniformBuffers)vkDestroyBuffer(device, obj, nullptr);
		for (auto obj : uniformBuffer.uniformBuffersMemory)vkFreeMemory(device, obj, nullptr);
		vkDestroyDescriptorPool(device, uniformBuffer.descriptorPool, nullptr);

	}
	std::string_view GraphicsTextureHandle::getError() const { return currentError; }
	const UniformTextureInternals& GraphicsTextureHandle::Internals()const { return internals; }
	bool GraphicsTextureHandle::init(std::string_view filePath, VkDevice device, VkPhysicalDevice physicalDevice, VkQueue graphicsQueue, VkCommandPool commandPool, VkDescriptorSetLayout descriptorSetLayout)
	{
		TextureInternal& textureInfo = internals.texture;

		
		TextureMetaData fileData;
		fileData.pixelSize = STBI_rgb_alpha;
		// Will be using command buffer to store our image object. The images can be found in shader/texures
		int texChannels;
		// Force the load to create alpha, so it consistant with all other texures
		fileData.imageData = stbi_load(filePath.data(), &fileData.pictureWidth, &fileData.pictureHeight, &texChannels, fileData.pixelSize);
		textureInfo.textureFile = std::string(filePath);
		if (!fileData.imageData) { currentError = "failed to load texture image!"; return false; }
		bool state = init(fileData,device,physicalDevice,graphicsQueue,commandPool,descriptorSetLayout);
		stbi_image_free(fileData.imageData);
		return state;
	}


	bool GraphicsTextureHandle::init(const TextureMetaData& textureData, VkDevice device, VkPhysicalDevice physicalDevice, VkQueue graphicsQueue, VkCommandPool commandPool, VkDescriptorSetLayout descriptorSetLayout)
	{
		if (device == nullptr) {
			currentError = "Must insert a valid device";
			return false;
		}
		if (commandPool == nullptr) {
			currentError = "Must insert a valid command pool";
			return false;
		}
		if (physicalDevice == nullptr)
		{
			currentError = "Must insert a valid physical device";
			return false;
		}
		if (descriptorSetLayout == nullptr)
		{
			currentError = "Must insert a valid descriptorSetLayout";
			return false;
		}
		this->device = device;
		TextureInternal& textureInfo = internals.texture;
		// Copying image data to our vk buffer
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		textureInfo.mipLevels = textureData.mipLevel;
		if (textureInfo.mipLevels == 0) { textureInfo.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max<int>(textureData.pictureWidth, textureData.pictureHeight)))) + 1; }
		
		VkDeviceSize imageSize = textureData.pictureWidth * textureData.pictureHeight * textureData.pixelSize;

		createBuffer(device, physicalDevice, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data); // Going to allocate memory to persistant memory
		memcpy(data, textureData.imageData, static_cast<size_t>(imageSize));
		vkUnmapMemory(device, stagingBufferMemory);


		std::string errorMessage;
		errorMessage = Util::createImage(device, physicalDevice, textureData.pictureWidth, textureData.pictureHeight, textureInfo.mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureInfo.textureImage, textureInfo.textureImageMemory);
		if (!errorMessage.empty()) { currentError = "createImage:" + errorMessage; return false; }

		// VK_IMAGE_LAYOUT_UNDEFINED used before that how we initilized the image. Don't care about contents tell we perform copy operation
		errorMessage = transitionImageLayout(device, commandPool, graphicsQueue, textureInfo.textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, textureInfo.mipLevels);
		if (!errorMessage.empty()) { currentError = "transitionImageLayout:" + errorMessage; return false; }

		errorMessage = copyBufferToImage(device, commandPool, graphicsQueue, stagingBuffer, textureInfo.textureImage, static_cast<uint32_t>(textureData.pictureWidth), static_cast<uint32_t>(textureData.pictureHeight));// Moving the data down the pipeline
		if (!errorMessage.empty()) { currentError = "copyBufferToImage:" + errorMessage; return false; }

		errorMessage = generateMipmaps(device, physicalDevice, commandPool, graphicsQueue, textureInfo.textureImage, VK_FORMAT_R8G8B8A8_SRGB, textureData.pictureWidth, textureData.pictureHeight, textureInfo.mipLevels);
		if (!errorMessage.empty()) { currentError = "generateMipmaps:" + errorMessage; return false; }

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);


		textureInfo.textureImageView = Util::CreateImageView(device, textureInfo.textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, textureInfo.mipLevels);


		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		// specify how to interpolate texels that are magnified or minified
		// Magnified helps with the oversampling problem
		// Minufucation helps with the undersampling
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		/// axes are called U,V,W instead of X,Y,Z
		// This field only relevant when masking outside of the image
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		// NEED TO Read More About this
		// anisotropic filtering should be used most of the time. Unless performance is an issue
		// Lower value is lower quality. Should get maxium value from our device
		// If device doesn't contain the capability, than we should make this false
		samplerInfo.anisotropyEnable = VK_TRUE;
		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(physicalDevice, &properties);
		samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
		// Returns the color if clamp to border addressing mode is active
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		// If true, than values will be in range of [0,texWidth]
		// If false, than values will range in [9,1]
		// Most application are in normalized coordinates. Reason can be textures of different resolutions.
		samplerInfo.unnormalizedCoordinates = VK_FALSE;

		// Enables filtering on shadow maps.
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

		// READ More about this. mipmapping
		// Another filter that can be applied. 
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = static_cast<float>(textureInfo.mipLevels);

		if (vkCreateSampler(device, &samplerInfo, nullptr, &textureInfo.textureSampler) != VK_SUCCESS) { currentError = "failed to create texture sampler!"; return false; }



		UBOInternal& uniformBuffer = internals.ubo;
		VkDeviceSize bufferSize = sizeof(UniformBufferObject);

		uniformBuffer.uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		uniformBuffer.uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
		uniformBuffer.uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);//memory void here, so using vulkan memory mappping, we can make it dynamic

		// Technique "Persistent Mapping" which maps the buffer to specific pointer during the application lifetime
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			auto errorMessage = createBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffer.uniformBuffers[i], uniformBuffer.uniformBuffersMemory[i]);
			if (!errorMessage.empty())
			{
				currentError = "UniformBuffer: " + errorMessage;
				return false;
			}

			vkMapMemory(device, uniformBuffer.uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffer.uniformBuffersMapped[i]);
		}

		// These descriptors are created every frame
		std::array<VkDescriptorPoolSize, 2> poolSizes{};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);// No reason to allow create more than whats needed
		//VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT is an option to create the descriptors every frame

		if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &uniformBuffer.descriptorPool) != VK_SUCCESS) { currentError = "failed to create descriptor pool!"; return false; }


		//// Creating only 1 descriptor set for each descrptor pool
		std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = uniformBuffer.descriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		allocInfo.pSetLayouts = layouts.data();
		internals.descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
		if (vkAllocateDescriptorSets(device, &allocInfo, internals.descriptorSets.data()) != VK_SUCCESS) { currentError = "failed to allocate descriptor sets!"; return false; }
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			// Defining out uniform buffer object with the descriptor
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = uniformBuffer.uniformBuffers[i];
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);
			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = textureInfo.textureImageView;
			imageInfo.sampler = textureInfo.textureSampler;
			std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;// define configuration
			descriptorWrites[0].dstSet = internals.descriptorSets[i];// address to descriptor
			descriptorWrites[0].dstBinding = 0;// Define first index of array. They can be arrays
			descriptorWrites[0].dstArrayElement = 0;// Not using an array so 0 will make it not treat is as suc
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &bufferInfo;// Buffer data
			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = internals.descriptorSets[i];
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].pImageInfo = &imageInfo;// Optional Image data
			vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}

		return true;
	}



	GraphicsVerticesStorage::GraphicsVerticesStorage() = default;
	GraphicsVerticesStorage::~GraphicsVerticesStorage()
	{
		for (auto& mapping : textureMapping) {
			mapping.second.Free();
		}
	}
	void GraphicsVerticesStorage::clear()
	{
		for (auto& mapping : textureMapping) {
			mapping.second.Free();
		}
		textureMapping.clear();
	}

	std::vector<GraphicsVerticesStorage::Id> GraphicsVerticesStorage::getIds() const
	{
		std::vector<Id> ids;
		for (auto& [id, _] : textureMapping) ids.push_back(id);
		return ids;
	}
	const VerticesHandle& GraphicsVerticesStorage::getHandle(const Id& id) const {
		return textureMapping.at(id);
	}
	VerticesHandle& GraphicsVerticesStorage::getHandle(const Id& id) {
		return textureMapping.at(id);
	}
	bool GraphicsVerticesStorage::contains(const Id& id)
	{
		return textureMapping.find(id) != textureMapping.end();
	}

	void GraphicsVerticesStorage::init(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue graphicsQueue, VkCommandPool commandPool, VkDescriptorSetLayout descriptorSetLayout)
	{
		this->device = device;
		this->physicalDevice = physicalDevice;
		this->graphicsQueue = graphicsQueue;
		this->commandPool = commandPool;
		this->descriptorSetLayout = descriptorSetLayout;
	}

	void GraphicsVerticesStorage::createTexture(const Id& id, std::string_view filePathName)
	{
		VerticesHandle handle;
		bool state = handle.init(filePathName, device, physicalDevice, graphicsQueue, commandPool, descriptorSetLayout);
		if (!state) {
			std::cout << "ERROR with creating texture: " << handle.getError() << std::endl;
			return;
		}

		if (auto iter = textureMapping.find(id); iter != textureMapping.end()) {
			iter->second.Free();
		}
		textureMapping[id] = handle;

	}

	void GraphicsVerticesStorage::createVertice(const Id& id, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
	{
		VerticesHandle handle;
		bool state = handle.init(vertices, indices, device, physicalDevice, graphicsQueue, commandPool, descriptorSetLayout);
		if (!state) {
			std::cout << "ERROR with creating texture: " << handle.getError() << std::endl;
			return;
		}

		if (auto iter = textureMapping.find(id); iter != textureMapping.end()) {
			iter->second.Free();
		}
		textureMapping[id] = handle;
	}



	GraphicsTextureStorage::GraphicsTextureStorage() = default;
	GraphicsTextureStorage::~GraphicsTextureStorage()
	{
		for (auto& mapping : textureMapping) {
			mapping.second.Free();
		}
	}
	void GraphicsTextureStorage::clear()
	{
		for (auto& mapping : textureMapping) {
			mapping.second.Free();
		}
		textureMapping.clear();
	}

	std::vector<GraphicsTextureStorage::Id> GraphicsTextureStorage::getIds() const
	{
		std::vector<Id> ids;
		for (auto& [id, _] : textureMapping) ids.push_back(id);
		return ids;
	}
	const GraphicsTextureHandle& GraphicsTextureStorage::getTextureHandle(const Id&id) const {
		return textureMapping.at(id);
	}
	GraphicsTextureHandle& GraphicsTextureStorage::getTextureHandle(const Id& id) {
		return textureMapping.at(id);
	}
	bool GraphicsTextureStorage::contains(const Id&id)
	{
		return textureMapping.find(id) != textureMapping.end();
	}

	void GraphicsTextureStorage::init(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue graphicsQueue, VkCommandPool commandPool, VkDescriptorSetLayout descriptorSetLayout)
	{
		this->device = device;
		this->physicalDevice = physicalDevice;
		this->graphicsQueue = graphicsQueue;
		this->commandPool = commandPool;
		this->descriptorSetLayout = descriptorSetLayout;
	}

	void GraphicsTextureStorage::createTexture(const Id& id, std::string_view filePathName)
	{
		GraphicsTextureHandle handle;
		bool state = handle.init(filePathName, device, physicalDevice, graphicsQueue, commandPool, descriptorSetLayout);
		if (!state) {
			std::cout << "ERROR with creating texture: " << handle.getError() << std::endl;
			return;
		}

		if (auto iter = textureMapping.find(id); iter != textureMapping.end()) {
			iter->second.Free();
		}
		textureMapping[id] = handle;

	}

}