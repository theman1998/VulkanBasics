#pragma once

#include "GraphicEngine/ConstDefines.hpp"

#include <map>
#include <memory>
#include <array>
#include <vector>
#include <type_traits>


#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/glm.hpp>


namespace GE
{
	struct Vertex {
		glm::vec3 pos;
		glm::vec3 color;
		glm::vec2 texCoord;

		static VkVertexInputBindingDescription getBindingDescription();

		static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();


		bool operator==(const Vertex& other) const;
	};


	// Alignment is important. Things need to be multiple of 16
	// Can for alignments with #define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES 
	// Avoid GLM_FORCE_DEFAULT_ALIGNED_GENTYPES and be explicit with the alignas
	struct UniformBufferObject
	{
		alignas(16) glm::mat4 model;
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 proj;
	};



	struct VerticesInternal {
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		VkBuffer vertexBuffer;
		VkDeviceMemory vertexBufferMemory; // allocated memory for gpu
		VkBuffer indexBuffer;
		VkDeviceMemory indexBufferMemory;
	};

	class VerticesHandle {
	public:
		VerticesHandle();
		~VerticesHandle();
		void Free();
		std::string_view getError() const;
		const VerticesInternal& Internals()const;

		bool init(std::string_view filePath, VkDevice device, VkPhysicalDevice physicalDevice, VkQueue graphicsQueue, VkCommandPool commandPool, VkDescriptorSetLayout descriptorSetLayout);
		bool init(std::vector<Vertex>, std::vector<uint32_t>, VkDevice device, VkPhysicalDevice physicalDevice, VkQueue graphicsQueue, VkCommandPool commandPool, VkDescriptorSetLayout descriptorSetLayout);


	private:
		VerticesInternal internals;
		std::string currentError;
		VkDevice device{nullptr};
	};


	class GraphicsVerticesStorage {
		using Id = std::string;
		GraphicsVerticesStorage& operator=(GraphicsVerticesStorage&) = delete;
		GraphicsVerticesStorage(const GraphicsVerticesStorage&) = delete;
	public:
		GraphicsVerticesStorage();
		/// @brief Handles the cleaning of textures
		~GraphicsVerticesStorage();
		void clear();
		void init(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue graphicsQueue, VkCommandPool commandPool, VkDescriptorSetLayout descriptorSetLayout);
		std::vector<Id> getIds() const;
		const VerticesHandle& getHandle(const Id&) const;
		VerticesHandle& getHandle(const Id&);
		bool contains(const Id&);

		void createTexture(const Id& id, std::string_view filePathName);
		void createVertice(const Id& id, const std::vector<Vertex>&, const std::vector<uint32_t>&);

	private:
		std::map<Id, VerticesHandle> textureMapping;
		VkDevice device{ nullptr };
		VkPhysicalDevice physicalDevice{ nullptr };
		VkQueue graphicsQueue{ nullptr };
		VkCommandPool commandPool{ nullptr };
		VkDescriptorSetLayout descriptorSetLayout{ nullptr };
	};


	struct TextureInternal {
		uint32_t mipLevels;
		VkImage textureImage;
		VkDeviceMemory textureImageMemory;
		VkImageView textureImageView;
		VkSampler textureSampler; // Distinct from the image. Can be used to extra pixels from any image
		VkDescriptorImageInfo descriptor;
		std::string textureFile;
	};
	struct UBOInternal {
		// We might have multiple frames in flight at a single time, hence we must use dynamic memory
		std::vector<VkBuffer> uniformBuffers;
		std::vector<VkDeviceMemory> uniformBuffersMemory;
		std::vector<void*> uniformBuffersMapped;
		// Used coasside with the uniform buffers
		VkDescriptorPool descriptorPool;
	};
	struct UniformTextureInternals {
		TextureInternal texture;
		UBOInternal ubo;
		std::vector<VkDescriptorSet> descriptorSets;
	};


	struct TextureMetaData
	{
		unsigned char *imageData{nullptr};
		uint16_t pixelSize{ 4 };
		int pictureWidth{ 0 };
		int pictureHeight{ 0 };
		uint32_t mipLevel{ 0 };
	};

	class GraphicsTextureHandle
	{
	public:
		GraphicsTextureHandle();
		~GraphicsTextureHandle();
		void Free();
		std::string_view getError() const;
		const UniformTextureInternals& Internals()const;

		bool init(std::string_view filePath, VkDevice device, VkPhysicalDevice physicalDevice, VkQueue graphicsQueue, VkCommandPool commandPool, VkDescriptorSetLayout descriptorSetLayout);
		bool init(const TextureMetaData&, VkDevice device, VkPhysicalDevice physicalDevice, VkQueue graphicsQueue, VkCommandPool commandPool, VkDescriptorSetLayout descriptorSetLayout);

	private:

		UniformTextureInternals internals;
		std::string currentError;
		VkDevice device;


	};


	class GraphicsTextureStorage {
		using Id = std::string;
		GraphicsTextureStorage& operator=(GraphicsTextureStorage&) = delete;
		GraphicsTextureStorage(const GraphicsTextureStorage&) = delete;
	public:
		GraphicsTextureStorage();
		/// @brief Handles the cleaning of textures
		~GraphicsTextureStorage();
		void clear();

		void init(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue graphicsQueue, VkCommandPool commandPool, VkDescriptorSetLayout descriptorSetLayout);

		std::vector<Id> getIds() const;
		const GraphicsTextureHandle& getTextureHandle(const Id&) const;
		GraphicsTextureHandle& getTextureHandle(const Id&);
		bool contains(const Id&);

		void createTexture(const Id& id, std::string_view filePathName);

	private:
		std::map<Id, GraphicsTextureHandle> textureMapping;

		VkDevice device{ nullptr };
		VkPhysicalDevice physicalDevice{ nullptr };
		VkQueue graphicsQueue{ nullptr };
		VkCommandPool commandPool{ nullptr };
		VkDescriptorSetLayout descriptorSetLayout{nullptr};
	};

}









namespace std {
	template<> struct hash<GE::Vertex> {
		size_t operator()(GE::Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
}