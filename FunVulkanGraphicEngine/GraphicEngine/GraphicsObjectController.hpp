#pragma once

#include "GraphicEngine/GraphicsVertex.hpp"
#include <unordered_map>
#include <mutex>
#include <memory>

namespace GE
{

	class GraphicObject {
	public:
		VerticesHandle verticesHandle;
		GraphicsTextureHandle textureHandle;

		UniformBufferObject getUBO() const;
		void setUBO(const UniformBufferObject&);

	private:
		UniformBufferObject uniformObject{};
		mutable std::mutex mutex;
	};
	using GraphObjPtr = std::shared_ptr<GraphicObject>;


	class GraphicsObjectController
	{
	public:
		GraphicsObjectController() = default;
		void init(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue graphicsQueue, VkCommandPool commandPool, VkDescriptorSetLayout descriptorSetLayout);

		void clear();
		void remove(const std::string& id);
		bool contains(const std::string& id)const;
		void createObject(const std::string& id, bool doOverride = false);
		GraphObjPtr retrieveObject(const std::string& id);

		std::vector<std::string> getIds() const;

	private:
		std::unordered_map<std::string, GraphObjPtr> objectList;


		VkDevice device{ nullptr };
		VkPhysicalDevice physicalDevice{ nullptr };
		VkQueue graphicsQueue{ nullptr };
		VkCommandPool commandPool{ nullptr };
		VkDescriptorSetLayout descriptorSetLayout{ nullptr };
	};





}